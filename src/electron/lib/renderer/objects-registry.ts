class ObjectsRegistry {
  private nextId = 0;

  // Stores all objects by ref-counting.
  // (id) => {object, count}
  private storage: Record<number, { count: number, object: any }> = {};

  // Stores the IDs + refCounts of objects referenced by WebContents.
  // (ownerKey) => { id: refCount }
  private owners: Record<string, Set<number>> = {};

  private electronIds = new WeakMap<object, number>();

  // Register a new object and return its assigned ID. If the object is already
  // registered then the already assigned ID would be returned.
  public add (webContentsId: number, contextId: string, obj: object) {
    // Get or assign an ID to the object.
    const id = this.saveToStorage(obj);

    // Add object to the set of referenced objects.
    const webContentsContextId = `${webContentsId}-${contextId}`;
    let owner = this.owners[webContentsContextId];
    if (!owner) {
      owner = this.owners[webContentsContextId] = new Set();
    }
    if (!owner.has(id)) {
      owner.add(id);
      // Increase reference count if not referenced before.
      this.storage[id].count++;
    }
    return id;
  }

  // Get an object according to its ID.
  public get (id: number) {
    const pointer = this.storage[id];
    if (pointer != null) return pointer.object;
  }

  // Dereference an object according to its ID.
  // Note that an object may be double-freed (cleared when page is reloaded, and
  // then garbage collected in old page).
  public remove (webContentsId: number, contextId: string, id: number) {
    const webContentsContextId = `${webContentsId}-${contextId}`;
    const owner = this.owners[webContentsContextId];
    if (owner) {
      // Remove the reference in owner.
      owner.delete(id);
      // Dereference from the storage.
      this.dereference(id);
    }
  }

  // Clear all references to objects refrenced by the WebContents.
  public clear (webContentsId: number, contextId: string) {
    const webContentsContextId = `${webContentsId}-${contextId}`;
    const owner = this.owners[webContentsContextId];
    if (!owner) return;

    for (const id of owner) this.dereference(id);

    delete this.owners[webContentsContextId];
  }

  // Saves the object into storage and assigns an ID for it.
  private saveToStorage (object: object) {
    let id = this.electronIds.get(object);
    if (!id) {
      id = ++this.nextId;
      this.storage[id] = {
        count: 0,
        object: object
      };
      this.electronIds.set(object, id);
    }
    return id;
  }

  // Dereference the object from store.
  private dereference (id: number) {
    const pointer = this.storage[id];
    if (pointer == null) {
      return;
    }
    pointer.count -= 1;
    if (pointer.count === 0) {
      this.electronIds.delete(pointer.object);
      delete this.storage[id];
    }
  }
}

export const objectsRegistry = new ObjectsRegistry();
