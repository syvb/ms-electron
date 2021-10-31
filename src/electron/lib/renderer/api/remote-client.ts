/* global FinalizationRegistry, WeakRef */

import { EventEmitter } from 'events';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

import { CallbacksRegistry } from '@microsoft/lib/renderer/callbacks-registry';
import * as ipcMessages from '@microsoft/lib/renderer/ipc-messages';
import * as utils from '@microsoft/lib/renderer/utils';

import type {
  MetaType,
  MetaTypeFromRenderer,
  MetaTypeObjectFunction,
  ObjectMember,
  ObjProtoDescriptor
} from '@microsoft/lib/renderer/types';

const v8Util = process.electronBinding('v8_util');

// Unique ID that can represent the current context.
const contextId = v8Util.getHiddenValue(global, 'contextId');

export interface RemoteClientMetrics {
  duration: number;
  durationInHost?: number;
  async?: boolean;
}

export interface RemoteClientHooks {
  incomingMessage?(message: string): void;
  outgoingMessage?(message: string): void;
  outgoingMessageSync?(message: string): void;

  require?(module: string, metrics: RemoteClientMetrics): void;

  functionConstructor?(name: string, metrics: RemoteClientMetrics): void;
  functionCall?(name: string, metrics: RemoteClientMetrics): void;

  memberConstructor?(target: Object, name: string, metrics: RemoteClientMetrics): void;
  memberCall?(target: Object, name: string, metrics: RemoteClientMetrics): void;
  memberGet?(target: Object, name: string, metrics: RemoteClientMetrics): void;
  memberSet?(target: Object, name: string, metrics: RemoteClientMetrics): void;
}

export interface RemoteClientOptions {
  enableDirectChannel?: boolean;
  enableTracing?: boolean;
}

class RemoteClient extends EventEmitter {
  public enableTracing = false;
  public enableOptimizationConstant = true;
  public enableOptimizationPromise = true;
  public enableOptimizationVoid = true;
  public enableOptimizationSimpleArgs = true;
  public usingDirectChannel = false;
  public hooks: RemoteClientHooks = {};

  private callbacksRegistry = new CallbacksRegistry();
  private remoteObjectCache = new Map();
  private electronIds = new WeakMap<Object, number>();
  private finalizationRegistry: FinalizationRegistry;

  public constructor (private hostId: number, options: RemoteClientOptions) {
    super();

    this.enableTracing = !!options.enableTracing;

    this.finalizationRegistry = new FinalizationRegistry((id) => {
      const ref = this.remoteObjectCache.get(id);
      if (ref !== undefined && ref.deref() === undefined) {
        this.remoteObjectCache.delete(id);
        this.send(ipcMessages.remoteServerDereference, id);
      }
    });

    if (options.enableDirectChannel) {
      try {
        ipcRendererInternal.on('ms-connect-event', ({ senderId }, connected: boolean) => {
          // make sure we only handle messages from our host
          if (senderId !== this.hostId) {
            return;
          }

          if (this.enableTracing) {
            console.log(`ms-connect-event: ${connected}`);
          }

          this.usingDirectChannel = connected;
          this.emit('using-direct-channel-changed', this.usingDirectChannel);
        });

        ipcRendererInternal.connectToRenderer(this.hostId);
        ipcRendererInternal.sendTo(this.hostId, ipcMessages.remoteServerConnect);
      } catch (error) {
        if (this.enableTracing) {
          console.log(`connectToRenderer failed: ${error}`);
        }
      }
    }

    // Host calls a callback in renderer.
    this.handleMessage(ipcMessages.remoteClientCallbackInvoke, (id: number, args: any) => {
      this.callbacksRegistry.apply(id, this.metaToValue(args));
    });

    // A callback in host is released.
    this.handleMessage(ipcMessages.remoteClientCallbackRelease, (id: number) => {
      this.callbacksRegistry.remove(id);
    });

    process.on('exit', () => {
      this.send(ipcMessages.remoteServerContextRelease);
    });
  }

  public require (module: string) {
    const api = `remote-client: require() / ${module}`;
    const [meta, metrics] = this.sendSync(ipcMessages.remoteServerRequire, [module], api);

    if (this.hooks && this.hooks.require) {
      this.hooks.require(module, metrics);
    }

    return this.metaToValue(meta);
  }

  private getCachedRemoteObject (id: number) {
    const ref = this.remoteObjectCache.get(id);
    if (ref !== undefined) {
      const deref = ref.deref();
      if (deref !== undefined) return deref;
    }
  }

  private setCachedRemoteObject (id: number, value: object) {
    const wr = new WeakRef(value);
    this.remoteObjectCache.set(id, wr);
    this.finalizationRegistry.register(value, id);
    return value;
  }

  private handleMessage (channel: string, handler: (...args: any[]) => void) {
    ipcRendererInternal.on(channel, (event, passedContextId, ...args) => {
      const { senderId, isDirectChannel } = event;

      // make sure we only handle messages from our host
      if (senderId !== this.hostId) {
        return;
      }

      if (this.hooks.incomingMessage) {
        this.hooks.incomingMessage(channel);
      }

      if (this.enableTracing) {
        console.group('handleMessage', channel);
        console.log('hostId:', this.hostId, 'isDirectChannel:', isDirectChannel, 'contextId:', passedContextId, 'args:', ...args);
      }

      if (passedContextId === contextId) {
        handler(...args);
      }

      if (this.enableTracing) {
        console.groupEnd();
      }
    });
  }

  private send (command: string, ...args: any[]) {
    if (this.hooks.outgoingMessage) {
      this.hooks.outgoingMessage(command);
    }

    if (this.enableTracing) {
      console.group('sendTo', command);
      console.log('hostId:', this.hostId, 'contextId:', contextId, 'args:', ...args);
    }

    const timeStart = utils.now();
    ipcRendererInternal.sendToRenderer(this.hostId, command, [contextId, ...args]);
    const duration = utils.now() - timeStart;

    if (this.enableTracing) {
      console.log('duration:', utils.precise(duration));
      console.groupEnd();
    }

    return { duration, async: true };
  }

  private sendSync (command: string, args: any[], api: string) {
    if (this.hooks.outgoingMessageSync) {
      this.hooks.outgoingMessageSync(command);
    }

    if (this.enableTracing) {
      console.group('sendToSync', command);
      console.log('hostId:', this.hostId, 'contextId:', contextId, 'args:', ...args);
    }

    const timeStart = utils.now();
    const [result, durationInHost] = ipcRendererInternal.sendToRendererSync(this.hostId, command, [contextId, ...args], api);
    const duration = utils.now() - timeStart;

    if (this.enableTracing) {
      console.log('duration:', utils.precise(duration), 'durationInHost:', utils.precise(durationInHost), 'result:', result);
      console.groupEnd();
    }

    return [result, { duration, durationInHost }];
  }

  // Convert the arguments object into an array of meta data.
  private wrapArgs (args: any[], simple = false, visited = new Set()): MetaTypeFromRenderer[] {
    if (simple) {
      return args.map(value => ({
        type: 'value',
        value: value
      }));
    }

    const valueToMeta = (value: any): MetaTypeFromRenderer => {
      // Check for circular reference.
      if (visited.has(value)) {
        return {
          type: 'value',
          value: null
        };
      }

      if (Array.isArray(value)) {
        visited.add(value);
        const meta: MetaTypeFromRenderer = {
          type: 'array',
          value: this.wrapArgs(value, simple, visited)
        };
        visited.delete(value);
        return meta;
      } else if (value instanceof Buffer) {
        return {
          type: 'buffer',
          value
        };
      } else if (utils.isSerializableObject(value)) {
        return {
          type: 'value',
          value
        };
      } else if (typeof value === 'object') {
        if (utils.isPromise(value)) {
          return {
            type: 'promise',
            then: valueToMeta((onFulfilled: Function, onRejected: Function) => {
              value.then(onFulfilled, onRejected);
            })
          };
        } else if (this.electronIds.has(value)) {
          return {
            type: 'remote-object',
            id: this.electronIds.get(value)!
          };
        }

        const meta: MetaTypeFromRenderer = {
          type: 'object',
          name: value.constructor ? value.constructor.name : '',
          members: []
        };
        visited.add(value);
        // eslint-disable-next-line guard-for-in
        for (const prop in value) {
          meta.members.push({
            name: prop,
            value: valueToMeta(value[prop])
          });
        }
        visited.delete(value);
        return meta;
      } else if (typeof value === 'function') {
        return {
          type: 'function',
          id: this.callbacksRegistry.add(value),
          length: value.length
        };
      } else {
        return {
          type: 'value',
          value
        };
      }
    };
    return args.map(valueToMeta);
  }

  // Populate object's members from descriptors.
  // The |ref| will be kept referenced by |members|.
  // This matches |getObjectMemebers| in rpc-server.
  private setObjectMembers (ref: any, object: any, metaId: number, members: ObjectMember[]) {
    if (!Array.isArray(members)) return;

    for (const member of members) {
      if (Object.prototype.hasOwnProperty.call(object, member.name)) continue;

      const descriptor: PropertyDescriptor = { enumerable: member.enumerable };
      if (member.type === 'method') {
        const remoteMemberFunction = this.makeRemoteMemberFunction(ref, metaId, member);
        let descriptorFunction = this.proxyFunctionProperties(remoteMemberFunction, ref, metaId, member.name);

        descriptor.get = () => {
          descriptorFunction.ref = ref; // The member should reference its object.
          return descriptorFunction;
        };
        // Enable monkey-patch the method
        descriptor.set = (value) => {
          descriptorFunction = value;
          return value;
        };
        descriptor.configurable = true;
      } else if (member.type === 'get') {
        descriptor.get = this.makePropertyGetter(ref, metaId, member);
        if (member.writable) {
          descriptor.set = this.makePropertySetter(ref, metaId, member);
        }
      } else if (member.type === 'value') {
        descriptor.value = member.value;
        descriptor.writable = member.writable;
      }

      Object.defineProperty(object, member.name, descriptor);
    }
  }

  // Populate object's prototype from descriptor.
  // This matches |getObjectPrototype| in rpc-server.
  private setObjectPrototype (ref: any, object: any, metaId: number, descriptor: ObjProtoDescriptor) {
    if (descriptor === null) return;
    const proto = {};
    this.setObjectMembers(ref, proto, metaId, descriptor.members);
    this.setObjectPrototype(ref, proto, metaId, descriptor.proto);
    Object.setPrototypeOf(object, proto);
  }

  // Wrap function in Proxy for accessing remote properties
  private proxyFunctionProperties (remoteMemberFunction: Function, target: object, metaId: number, name: string) {
    let loaded = false;

    // Lazily load function properties
    const loadRemoteProperties = () => {
      if (loaded) return;
      loaded = true;

      const api = `remote-client: get ${getClassName(target)}.${name}`;
      const [meta, metrics] = this.sendSync(ipcMessages.remoteServerMemberGet, [metaId, name], api);

      if (this.hooks.memberGet) {
        this.hooks.memberGet(target, name, metrics);
      }

      this.setObjectMembers(remoteMemberFunction, remoteMemberFunction, meta.id, meta.members);
    };

    return new Proxy(remoteMemberFunction as any, {
      set: (target, property, value) => {
        if (property !== 'ref') loadRemoteProperties();
        target[property] = value;
        return true;
      },
      get: (target, property) => {
        if (!Object.prototype.hasOwnProperty.call(target, property)) loadRemoteProperties();
        const value = target[property];
        if (property === 'toString' && typeof value === 'function') {
          return value.bind(target);
        }
        return value;
      },
      ownKeys: (target) => {
        loadRemoteProperties();
        return Object.getOwnPropertyNames(target);
      },
      getOwnPropertyDescriptor: (target, property) => {
        const descriptor = Object.getOwnPropertyDescriptor(target, property);
        if (descriptor) return descriptor;
        loadRemoteProperties();
        return Object.getOwnPropertyDescriptor(target, property);
      }
    });
  }

  private makePropertyGetter (target: object, metaId: number, member: ObjectMember) {
    return () => {
      const api = `remote-client: get ${getClassName(target)}.${member.name}`;
      const [result, metrics] = this.sendSync(ipcMessages.remoteServerMemberGet, [metaId, member.name], api);

      if (this.hooks.memberGet) {
        this.hooks.memberGet(target, member.name, metrics);
      }

      return this.metaToValue(result);
    };
  }

  private makePropertySetter (target: object, metaId: number, member: ObjectMember) {
    return (value: any) => {
      const args = this.wrapArgs([value]);
      const api = `remote-client: set ${getClassName(target)}.${member.name}`;
      const [result, metrics] = this.sendSync(ipcMessages.remoteServerMemberSet, [metaId, member.name, args], api);

      if (this.hooks.memberSet) {
        this.hooks.memberSet(target, member.name, metrics);
      }

      if (result != null) this.metaToValue(result);
      return value;
    };
  }

  private makeRemoteMemberFunction (target: object, metaId: number, member: ObjectMember) {
    const self = this;
    const remoteMemberFunction = function (this: object, ...args: any[]) {
      const isConstructCall = this && this.constructor === remoteMemberFunction;
      const simpleArgs = self.enableOptimizationSimpleArgs && member.simpleArgs;

      if (!isConstructCall) {
        if (self.enableOptimizationVoid && member.returnType === 'void') {
          const metrics = self.send(ipcMessages.remoteServerMemberCallAsync, metaId, member.name, self.wrapArgs(args, simpleArgs));

          if (self.hooks.memberCall) {
            self.hooks.memberCall(target, member.name, metrics);
          }

          return;
        } else if (self.enableOptimizationPromise && member.returnType === 'promise') {
          return new Promise((resolve, reject) => {
            args.push((error: Error, result: any) => {
              if (error) {
                reject(error);
              } else {
                resolve(result);
              }
            });

            const metrics = self.send(ipcMessages.remoteServerMemberCallAsync, metaId, member.name, self.wrapArgs(args, simpleArgs));

            if (self.hooks.memberCall) {
              self.hooks.memberCall(target, member.name, metrics);
            }
          });
        }
      }

      const command = isConstructCall
        ? ipcMessages.remoteServerMemberConstructor
        : ipcMessages.remoteServerMemberCall;

      const api = `remote-client: ${isConstructCall ? 'new' : 'call'} ${getClassName(target)}.${member.name}()`;
      const [result, metrics] = self.sendSync(command, [metaId, member.name, self.wrapArgs(args, simpleArgs)], api);

      if (isConstructCall) {
        if (self.hooks.memberConstructor) {
          self.hooks.memberConstructor(target, member.name, metrics);
        }
      } else {
        if (self.hooks.memberCall) {
          self.hooks.memberCall(target, member.name, metrics);
        }
      }

      return self.metaToValue(result);
    };

    if (this.enableOptimizationConstant && member.returnType === 'constant') {
      return this.cacheReturnValue(remoteMemberFunction);
    }

    return remoteMemberFunction;
  }

  private makeRemoteFunction (meta: MetaTypeObjectFunction) {
    const self = this;
    const remoteFunction = function (this: object, ...args: any[]) {
      const isConstructCall = this && this.constructor === remoteFunction;
      const simpleArgs = self.enableOptimizationSimpleArgs && meta.simpleArgs;

      if (!isConstructCall) {
        if (self.enableOptimizationVoid && meta.returnType === 'void') {
          const metrics = self.send(ipcMessages.remoteServerFunctionCallAsync, meta.id, self.wrapArgs(args, simpleArgs));

          if (self.hooks.functionCall) {
            self.hooks.functionCall(meta.name, metrics);
          }

          return;
        } else if (self.enableOptimizationPromise && meta.returnType === 'promise') {
          return new Promise((resolve, reject) => {
            args.push((error: Error, result: any) => {
              if (error) {
                reject(error);
              } else {
                resolve(result);
              }
            });

            const metrics = self.send(ipcMessages.remoteServerFunctionCallAsync, meta.id, self.wrapArgs(args, simpleArgs));

            if (self.hooks.functionCall) {
              self.hooks.functionCall(meta.name, metrics);
            }
          });
        }
      }

      const command = isConstructCall
        ? ipcMessages.remoteServerConstructor
        : ipcMessages.remoteServerFunctionCall;

      const api = `remote-client: ${isConstructCall ? 'new' : 'call'} ${meta.name}()`;
      const [result, metrics] = self.sendSync(command, [meta.id, self.wrapArgs(args, simpleArgs)], api);

      if (isConstructCall) {
        if (self.hooks.functionConstructor) {
          self.hooks.functionConstructor(meta.name, metrics);
        }
      } else {
        if (self.hooks.functionCall) {
          self.hooks.functionCall(meta.name, metrics);
        }
      }

      return self.metaToValue(result);
    };

    if (this.enableOptimizationConstant && meta.returnType === 'constant') {
      return this.cacheReturnValue(remoteFunction);
    }

    return remoteFunction;
  }

  private cacheReturnValue<T> (func: (...args: any[]) => T) {
    let value: T;
    let cached = false;

    return function (this: any, ...args: any[]) {
      if (!cached) {
        value = func.apply(this, args);
        cached = true;
      }

      return value;
    };
  }

  // Convert meta data from host into real value.
  private metaToValue (meta: MetaType): any {
    if (meta.type === 'value') {
      return meta.value;
    } else if (meta.type === 'array') {
      return meta.members.map((member) => this.metaToValue(member));
    } else if (meta.type === 'buffer') {
      return Buffer.from(meta.value.buffer, meta.value.byteOffset, meta.value.byteLength);
    } else if (meta.type === 'promise') {
      return Promise.resolve({ then: this.metaToValue(meta.then) });
    } else if (meta.type === 'exception') {
      throw this.metaToValue(meta.value);
    }

    let ret;
    const cached = this.getCachedRemoteObject(meta.id);
    if (cached !== undefined) { return cached; }

    // A shadow class to represent the remote function object.
    if (meta.type === 'function') {
      ret = this.makeRemoteFunction(meta);
    } else {
      ret = {};
    }

    this.setObjectMembers(ret, ret, meta.id, meta.members);
    this.setObjectPrototype(ret, ret, meta.id, meta.proto);
    Object.defineProperty(ret.constructor, 'name', { value: meta.constructorName });

    // Track delegate obj's lifetime & tell host to clean up when object is GCed.
    this.electronIds.set(ret, meta.id);
    this.setCachedRemoteObject(meta.id, ret);
    return ret;
  }
}

function getClassName (obj: object) {
  return obj.constructor ? obj.constructor.name : '<object>';
}

const clients = new Map<number, RemoteClient>();

export function get (hostId: number, options: RemoteClientOptions = {}) {
  if (clients.has(hostId)) {
    return clients.get(hostId);
  }

  const client = new RemoteClient(hostId, options);
  clients.set(hostId, client);

  return client;
}
