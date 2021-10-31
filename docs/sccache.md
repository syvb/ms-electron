# sccache – Shared Compiler Cache

<https://github.com/mozilla/sccache#readme>

Please note that now it only works for Electron builds on Linux and Mac. 

## TL;DR

*The following code snippets assume that the current working dir is `src/`.*

Stop the sccache server if it's already running. 
```
./electron/external_binaries/sccache --stop-server
```
    
Set the following environment variables to use our Azure-backed shared cache.
You'll need to get a connection string for an [Azure storage account][] first
```
export SCCACHE_AZURE_BLOB_CONTAINER="sccache-mac"  # or "sccache-linux"
export SCCACHE_AZURE_CONNECTION_STRING="<connection-string>"
```

Then configure the Electron build to use the `sccache`. To edit a GN config run
```
    gn args out/default
```

Then add a new line in the opened editor
```
    cc_wrapper="<absolute_path>/src/electron/external_binaries/sccache"
```

You all set, now run a build as usual:
```
    ninja -C out/default electron
```

If it didn't work keep reading.

## Long and boring version

We can use sccache to speed up Electron builds on Mac and Linux.
sccache on Windows currently doesn't work.

### Using the cache

#### Configure the build

Chromium build configuration exposes a `cc_wrapper` build time argument
to use various tools instead of a default compiler.  
To use sccache set that argument value to an absolute path to a sccache binary shipped with Electron.  
Right now the binary is stored as "…/src/electron/external_binaries/sccache".  
It is important to use the binary shipped with Electron because caching depends on a binary version.  
To update a build config run
```
    gn args out/default
```

an editor will open, add a new line `cc_wrapper = "/absolute/path/to/sccache"`, save and close the file.

#### Configure sccache

Set new environment variables to make sccache use our Azure storage.
You'll need to get a connection string for an [Azure storage account][] first
```
    export SCCACHE_AZURE_BLOB_CONTAINER="sccache-mac"  # or "sccache-linux"
    export SCCACHE_AZURE_CONNECTION_STRING="<connection-string>
```

If you have used sccache before stop the server to force sccache to respect the newly set variables.
You will see the `error: couldn't connect to server` error if the server is not running, it's ok.
```
    ./path/to/sccache --stop-server
```

#### Check if everything works

Start ninja build as usual, and kill it after a few seconds. Now check sccache stats to find out if it was used
```
    ./path/to/sccache --show-stats
```

It should show you something like this:
```
Compile requests                21
Compile requests executed       16
Cache hits                      13
Cache misses                     3
Cache timeouts                   0
Cache read errors                0
Forced recaches                  0
Cache write errors               0
Compilation failures             0
Cache errors                     0
Non-cacheable compilations       0
Non-cacheable calls              5
Non-compilation calls            0
Unsupported compiler calls       0
Average cache write          0.172 s
Average cache read miss      5.489 s
Average cache read hit       0.158 s
Cache location             Azure, container: BlobContainer(url=https://<...>/sccache-mac/)
```

If the "cache location" is "Azure" and there are cache hits, everything works fine.

#### Possible issues

1. **Cache location is different.**  
   Stop the sccache server. Make sure that the SCCACHE_* environment variables are defined and have correct values.
   If you have different environment variables set for sccache, unset them. Then run ninja again.

[Azure storage account]: https://ms.portal.azure.com/#@microsoft.onmicrosoft.com/resource/subscriptions/1a3c1edb-1a92-4a1e-ab91-251fdc9e6680/resourceGroups/electron-devops-rg/providers/Microsoft.Storage/storageAccounts/electroncache/keys
