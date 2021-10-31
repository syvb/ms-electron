/* global FinalizationRegistry, WeakRef */

import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

import { objectsRegistry } from '@microsoft/lib/renderer/objects-registry';
import * as ipcMessages from '@microsoft/lib/renderer/ipc-messages';
import * as utils from '@microsoft/lib/renderer/utils';

import type {
  Annotations,
  MetaType,
  MetaTypeFromRenderer,
  ObjectMember,
  ObjProtoDescriptor
} from '@microsoft/lib/renderer/types';

const v8Util = process.electronBinding('v8_util');

// The internal properties of Function.
const FUNCTION_PROPERTIES = [
  'length', 'name', 'arguments', 'caller', 'prototype'
];

let enableTracing = false;

interface IpcRendererEventSync extends Electron.IpcRendererEvent {
  sendReply(value: any): void;
}

type FinalizerInfo = {
  senderId: number;
  contextId: string;
  objectId: number;
};

type CallIntoRenderer = (...args: any[]) => void;

// The remote functions in renderer processes.
const rendererFunctionCache = new Map<string, WeakRef<CallIntoRenderer>>();
const finalizationRegistry = new FinalizationRegistry((fi: FinalizerInfo) => {
  const mapKey = `${fi.contextId}~${fi.objectId}`;
  const ref = rendererFunctionCache.get(mapKey);
  if (ref !== undefined && ref.deref() === undefined) {
    rendererFunctionCache.delete(mapKey);
    sendTo(fi.senderId, ipcMessages.remoteClientCallbackRelease, fi.contextId, fi.objectId);
  }
});

function getCachedRendererFunction (contextId: string, objectId: number) {
  const mapKey = `${contextId}~${objectId}`;
  const ref = rendererFunctionCache.get(mapKey);
  if (ref !== undefined) {
    const deref = ref.deref();
    if (deref !== undefined) return deref;
  }
  return undefined;
}

function setCachedRendererFunction (senderId: number, contextId: string, objectId: number, value: CallIntoRenderer) {
  const wr = new WeakRef<CallIntoRenderer>(value);
  const mapKey = `${contextId}~${objectId}`;
  rendererFunctionCache.set(mapKey, wr);
  finalizationRegistry.register(value, { senderId, contextId, objectId });
  return value;
}

function getAnnotations (object: object): Annotations {
  const returnType = v8Util.getHiddenValue<Annotations['returnType']>(object, 'returnType');
  const simpleArgs = v8Util.getHiddenValue<boolean>(object, 'simpleArgs');

  const result: Annotations = {};
  if (returnType) result.returnType = returnType;
  if (simpleArgs) result.simpleArgs = simpleArgs;

  return result;
}

// Return the description of object's members:
function getObjectMembers (object: any): ObjectMember[] {
  let names = Object.getOwnPropertyNames(object);
  // For Function, we should not override following properties even though they
  // are "own" properties.
  if (typeof object === 'function') {
    names = names.filter((name) => {
      return !FUNCTION_PROPERTIES.includes(name);
    });
  }
  // Map properties to descriptors.
  return names.map((name) => {
    const descriptor = Object.getOwnPropertyDescriptor(object, name)!;
    const member: ObjectMember = { name, enumerable: descriptor.enumerable, writable: false };
    if (descriptor.get === undefined && typeof object[name] === 'function') {
      member.type = 'method';
      Object.assign(member, getAnnotations(object[name]));
    } else {
      if (descriptor.set || descriptor.writable) member.writable = true;
      if (descriptor.get && v8Util.getHiddenValue(descriptor.get, 'returnType') === 'constant') {
        member.type = 'value';
        member.value = object[name];
      } else {
        member.type = 'get';
      }
    }
    return member;
  });
}

// Return the description of object's prototype.
function getObjectPrototype (object: any): ObjProtoDescriptor {
  const proto = Object.getPrototypeOf(object);
  if (proto === null || proto === Object.prototype) return null;
  return {
    members: getObjectMembers(proto),
    proto: getObjectPrototype(proto)
  };
}

// Convert a real value into meta data.
function valueToMeta (senderId: number, contextId: string, value: any): MetaType {
  // Determine the type of value.
  let type: MetaType['type'];

  switch (typeof value) {
    case 'object':
      // Recognize certain types of objects.
      if (value instanceof Buffer) {
        type = 'buffer';
      } else if (Array.isArray(value)) {
        type = 'array';
      } else if (utils.isSerializableObject(value)) {
        type = 'value';
      } else if (utils.isPromise(value)) {
        type = 'promise';
      } else if (Object.prototype.hasOwnProperty.call(value, 'callee') && value.length != null) {
        // Treat the arguments object as array.
        type = 'array';
      } else if (v8Util.getHiddenValue(value, 'simple')) {
        // Treat simple objects as value.
        type = 'value';
      } else {
        type = 'object';
      }
      break;
    case 'function':
      type = 'function';
      break;
    default:
      type = 'value';
      break;
  }

  // Fill the meta object according to value's type.
  if (type === 'array') {
    return {
      type,
      members: value.map((el: any) => valueToMeta(senderId, contextId, el))
    };
  } else if (type === 'object' || type === 'function') {
    return {
      type,
      name: value.name,
      constructorName: value.constructor ? value.constructor.name : '',
      // Reference the original value if it's an object, because when it's
      // passed to renderer we would assume the renderer keeps a reference of
      // it.
      id: objectsRegistry.add(senderId, contextId, value),
      members: getObjectMembers(value),
      proto: getObjectPrototype(value),
      ...getAnnotations(value)
    };
  } else if (type === 'buffer') {
    return {
      type,
      value
    };
  } else if (type === 'promise') {
    // Add default handler to prevent unhandled rejections in main process
    // Instead they should appear in the renderer process
    value.then(() => {}, () => {});

    return {
      type,
      then: valueToMeta(senderId, contextId, (onFulfilled: Function, onRejected: Function) => {
        value.then(onFulfilled, onRejected);
      })
    };
  } else {
    return {
      type: 'value',
      value
    };
  }
}

// Convert array of meta data from renderer into array of real values.
function unwrapArgs (senderId: number, contextId: string, args: any[]) {
  function metaToValue (meta: MetaTypeFromRenderer): any {
    switch (meta.type) {
      case 'value':
        return meta.value;
      case 'remote-object':
        return objectsRegistry.get(meta.id);
      case 'array':
        return unwrapArgs(senderId, contextId, meta.value);
      case 'buffer':
        return Buffer.from(meta.value.buffer, meta.value.byteOffset, meta.value.byteLength);
      case 'promise':
        return Promise.resolve({
          then: metaToValue(meta.then)
        });
      case 'object': {
        const ret: any = {};
        Object.defineProperty(ret.constructor, 'name', { value: meta.name });

        for (const { name, value } of meta.members) {
          ret[name] = metaToValue(value);
        }
        return ret;
      }
      case 'function': {
        // Cache the callbacks in renderer.
        const cachedFunction = getCachedRendererFunction(contextId, meta.id);
        if (cachedFunction !== undefined) { return cachedFunction; }

        const callIntoRenderer = (...args: any[]) => {
          sendTo(senderId, ipcMessages.remoteClientCallbackInvoke, contextId, meta.id, valueToMeta(senderId, contextId, args));
        };
        Object.defineProperty(callIntoRenderer, 'length', { value: meta.length });

        setCachedRendererFunction(senderId, contextId, meta.id, callIntoRenderer);
        return callIntoRenderer;
      }
      default:
        throw new TypeError(`Unknown type: ${(meta as any).type}`);
    }
  }
  return args.map(metaToValue);
}

export type RequireHandler = (senderId: number, module: string) => any;

let requireHandler: RequireHandler = () => {
  throw new Error('Server not initialized');
};

function sendTo (webContentsId: number, command: string, ...args: any[]) {
  if (enableTracing) {
    console.group('sendTo', webContentsId, command);
    console.log('args:', ...args);
  }

  const timeStart = utils.now();
  ipcRendererInternal.sendToRenderer(webContentsId, command, args);
  const duration = utils.now() - timeStart;

  if (enableTracing) {
    console.log('duration:', utils.precise(duration));
    console.groupEnd();
  }
}

function callbackify (func: Function) {
  return function (this: any, ...args: any[]) {
    const callback = args.pop();
    new Promise(resolve => {
      resolve(func.apply(this, args));
    }).then(result => {
      callback(null, result);
    }, error => {
      callback(error);
    });
  };
}

const callbackFunctions = new WeakMap<Function, Function>();

function getCallbackFunction (func: Function) {
  if (!callbackFunctions.has(func)) {
    callbackFunctions.set(func, callbackify(func));
  }
  return callbackFunctions.get(func)!;
}

function invoke (senderId: number, contextId: string, expression: () => any) {
  const timeStart = utils.now();
  const value = expression();
  const duration = utils.now() - timeStart;

  return [valueToMeta(senderId, contextId, value), duration];
}

function callFunctionAsync (func: Function, self: object | undefined, args: any[]) {
  if (v8Util.getHiddenValue(func, 'returnType') === 'promise') {
    func = getCallbackFunction(func);
  }

  func.apply(self, args);
}

function handleMessageSync (channel: string, handler: (senderId: number, contextId: string, ...args: any[]) => any) {
  ipcRendererInternal.on(channel, (event, contextId: string, ...args: any[]) => {
    const { senderId, isDirectChannel } = event;

    if (enableTracing) {
      console.group('handleMessageSync', channel);
      console.log('senderId:', senderId, 'isDirectChannel:', isDirectChannel, 'contextId:', contextId, 'args:', ...args);
    }

    let returnValue;
    try {
      returnValue = handler(senderId, contextId, ...args);
    } catch (error) {
      returnValue = [{
        type: 'exception',
        value: valueToMeta(senderId, contextId, error)
      }];
    }

    if (enableTracing) {
      console.log('returnValue:', returnValue);
    }

    (event as IpcRendererEventSync).sendReply(returnValue);

    if (enableTracing) {
      console.groupEnd();
    }
  });
}

function handleMessage (channel: string, handler: (senderId: number, contextId: string, ...args: any[]) => void) {
  ipcRendererInternal.on(channel, (event, contextId: string, ...args: any[]) => {
    const { senderId, isDirectChannel } = event;

    if (enableTracing) {
      console.group('handleMessage', channel);
      console.log('senderId:', senderId, 'isDirectChannel:', isDirectChannel, 'contextId:', contextId, 'args:', ...args);
    }

    try {
      handler(senderId, contextId, ...args);
    } catch (error) {
      console.error(error);
    }

    if (enableTracing) {
      console.groupEnd();
    }
  });
}

handleMessageSync(ipcMessages.remoteServerRequire, (senderId, contextId, module) => {
  return invoke(senderId, contextId, () => requireHandler(senderId, module));
});

handleMessageSync(ipcMessages.remoteServerConstructor, (senderId, contextId, id, args) => {
  args = unwrapArgs(senderId, contextId, args);
  const constructor = objectsRegistry.get(id);

  if (constructor == null) {
    throw new Error(`Cannot call constructor on missing remote object ${id}`);
  }

  return invoke(senderId, contextId, () => new constructor(...args));
});

handleMessageSync(ipcMessages.remoteServerFunctionCall, (senderId, contextId, id, args) => {
  args = unwrapArgs(senderId, contextId, args);
  const func = objectsRegistry.get(id);

  if (func == null) {
    throw new Error(`Cannot call function on missing remote object ${id}`);
  }

  return invoke(senderId, contextId, () => func(...args));
});

handleMessage(ipcMessages.remoteServerFunctionCallAsync, (senderId, contextId, id, args) => {
  args = unwrapArgs(senderId, contextId, args);
  const func = objectsRegistry.get(id);

  if (func == null) {
    throw new Error(`Cannot call function (async) on missing remote object ${id}`);
  }

  return callFunctionAsync(func, undefined, args);
});

handleMessageSync(ipcMessages.remoteServerMemberConstructor, (senderId, contextId, id, method, args) => {
  args = unwrapArgs(senderId, contextId, args);
  const object = objectsRegistry.get(id);

  if (object == null) {
    throw new Error(`Cannot call constructor '${method}' on missing remote object ${id}`);
  }

  return invoke(senderId, contextId, () => new object[method](...args));
});

handleMessageSync(ipcMessages.remoteServerMemberCall, (senderId, contextId, id, method, args) => {
  args = unwrapArgs(senderId, contextId, args);
  const obj = objectsRegistry.get(id);

  if (obj == null) {
    throw new Error(`Cannot call function '${method}' on missing remote object ${id}`);
  }

  return invoke(senderId, contextId, () => obj[method](...args));
});

handleMessage(ipcMessages.remoteServerMemberCallAsync, (senderId, contextId, id, method, args) => {
  args = unwrapArgs(senderId, contextId, args);
  const obj = objectsRegistry.get(id);

  if (obj == null) {
    throw new Error(`Cannot call function (async) '${method}' on missing remote object ${id}`);
  }

  return callFunctionAsync(obj[method], obj, args);
});

handleMessageSync(ipcMessages.remoteServerMemberSet, (senderId, contextId, id, name, args) => {
  args = unwrapArgs(senderId, contextId, args);
  const obj = objectsRegistry.get(id);

  if (obj == null) {
    throw new Error(`Cannot set property '${name}' on missing remote object ${id}`);
  }

  return invoke(senderId, contextId, () => {
    obj[name] = args[0];
    return undefined;
  });
});

handleMessageSync(ipcMessages.remoteServerMemberGet, (senderId, contextId, id, name) => {
  const obj = objectsRegistry.get(id);

  if (obj == null) {
    throw new Error(`Cannot get property '${name}' on missing remote object ${id}`);
  }

  return invoke(senderId, contextId, () => obj[name]);
});

handleMessage(ipcMessages.remoteServerDereference, (senderId, contextId, id) => {
  objectsRegistry.remove(senderId, contextId, id);
});

handleMessage(ipcMessages.remoteServerContextRelease, (senderId, contextId) => {
  objectsRegistry.clear(senderId, contextId);
});

ipcRendererInternal.on(ipcMessages.remoteServerConnect, ({ senderId }) => {
  try {
    ipcRendererInternal.on('ms-connect-event', (details) => {
      if (enableTracing) {
        console.log(`ms-connect-event: ${JSON.stringify(details)}`);
      }
    });
    ipcRendererInternal.connectToRenderer(senderId);
  } catch (error) {
    if (enableTracing) {
      console.log(`connectToRenderer failed: ${error}`);
    }
  }
});

export interface RemoteHostOptions {
  gcInterval?: number;
  enableTracing?: boolean;
}

export function init (handler: RequireHandler, options: RemoteHostOptions = {}) {
  requireHandler = handler;

  if (options.gcInterval) {
    setInterval(() => v8Util.lowMemoryNotification(), options.gcInterval);
  }

  enableTracing = !!options.enableTracing;
}
