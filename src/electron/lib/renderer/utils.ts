export function isPromise (val: any) {
  return (
    val &&
    val.then &&
    val.then instanceof Function &&
    val.constructor &&
    val.constructor.reject &&
    val.constructor.reject instanceof Function &&
    val.constructor.resolve &&
    val.constructor.resolve instanceof Function
  );
}

const serializableTypes = [
  Boolean,
  Number,
  String,
  Date,
  Error,
  RegExp,
  ArrayBuffer
];

// https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm#Supported_types
export function isSerializableObject (value: any) {
  return value === null || ArrayBuffer.isView(value) || serializableTypes.some(type => value instanceof type);
}

export function precise (x: number) {
  return x.toPrecision(3);
}

const timeOrigin = process.hrtime && process.hrtime();

export function now () {
  if (process.hrtime) {
    return milliseconds(process.hrtime(timeOrigin));
  } else {
    return performance.now();
  }
}

function milliseconds (time: [number, number]) {
  return time[0] * 1000 + time[1] / 1e6;
}
