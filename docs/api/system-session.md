# systemSession

> Monitor and control user´s operating system session.

Process: [Main](../glossary.md#main-process)

You cannot require or use this module until `app` module emits `ready` event.

## Events

The `systemSession` module emits the following events:

### Event: 'locked'  _Windows_, _macOS_, _Linux_

Emitted when the system session has been locked.

### Event: 'unlocked'  _Windows_, _macOS_, _Linux_

Emitted when the system session has been unlocked.

### Event: 'loggedon'  _Windows_

Emitted when the user is loggedon to the session.

### Event: 'loggedoff'  _Windows_

Emitted when the user is loggedoff from the session.


## Methods

### `systemSession.lock()` _Windows_, _Linux_

Locks the system session.

Returns `Boolean` - Indicates if the operation has been initiated. It does not indicate whether the system session has been successfully locked.
