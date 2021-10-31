type ReturnType = 'constant' | 'promise' | 'void';

export type Annotations = {
  returnType?: ReturnType;
  simpleArgs?: boolean;
};

export type ObjectMember = {
  name: string;
  value?: any;
  enumerable?: boolean;
  writable?: boolean;
  type?: 'method' | 'get' | 'value';
} & Annotations;

export type ObjProtoDescriptor = {
  members: ObjectMember[];
  proto: ObjProtoDescriptor;
} | null;

export type MetaTypeObjectFunction = {
  type: 'object' | 'function';
  name: string;
  constructorName: string;
  members: ObjectMember[];
  proto: ObjProtoDescriptor;
  id: number;
} & Annotations;

export type MetaType = MetaTypeObjectFunction | {
  type: 'value';
  value: any;
} | {
  type: 'buffer';
  value: Uint8Array;
} | {
  type: 'array';
  members: MetaType[];
} | {
  type: 'exception';
  value: MetaType;
} | {
  type: 'promise';
  then: MetaType;
};

export type MetaTypeFromRenderer = {
  type: 'value';
  value: any;
} | {
  type: 'remote-object';
  id: number;
} | {
  type: 'array';
  value: MetaTypeFromRenderer[];
} | {
  type: 'buffer';
  value: Uint8Array;
} | {
  type: 'promise';
  then: MetaTypeFromRenderer;
} | {
  type: 'object';
  name: string;
  members: {
    name: string;
    value: MetaTypeFromRenderer;
  }[];
} | {
  type: 'function';
  id: number;
  length: number;
};
