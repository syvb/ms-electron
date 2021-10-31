export class CallbacksRegistry {
  private nextId = 0;
  private callbacks = new Map<number, Function>();
  private callbackIds = new WeakMap<Function, number>();

  public add (callback: Function) {
    // The callback is already added.
    let id = this.callbackIds.get(callback);
    if (id != null) return id;

    id = this.nextId += 1;
    this.callbacks.set(id, callback);
    this.callbackIds.set(callback, id);
    return id;
  }

  public get (id: number) {
    return this.callbacks.get(id) || function () {};
  }

  public apply (id: number, ...args: any[]) {
    return this.get(id).apply(global, ...args);
  }

  public remove (id: number) {
    const callback = this.callbacks.get(id);
    if (callback) {
      this.callbackIds.delete(callback);
      this.callbacks.delete(id);
    }
  }
}
