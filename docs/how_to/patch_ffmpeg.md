# How To: Patch FFmpeg

## Overview

Because of various (mostly legal) reasons we can't ship the FFmpeg libraries that generic Electron build produces.  
So we create new build configurations for FFmpeg and use them to build the library.

Our approach for creating a custom config has two steps:
1. Copy one the Chromium configs.
2. Amend the copied config and some other build-related files.

## Custom build configs info

All meta information for custom build configs are stored in the `configs/ffmpeg/` directory.  
Paths to files in that directory are intended to be passed to scripts dealing with custom configs.

## (Re-)Generate patches

1.  Set `microsoft_add_custom_ffmpeg_configs` to `False` in the `gclient` config.  
    (Re-)Run `gclient sync` to get a Chromium tree without custom FFmpeg configs.

1.  Create a copy of an existing FFmpeg build config, commit the new directory.  
    There's a script to do exactly that, check the `DEPS` file.

1.  Replace defines in the newly created build config.
    Use `/scripts/change_defines_in_ffmpeg_config.py` for that.  
    Optionally commit the changes the script made passing the `--commit` argument to it.

1.  Manually apply existing patches (`/patches/customization/ffmpeg/`) for FFmpeg build configs  
    (use `/scripts/apply_chromium_patches.py`), to make it possible to use the new configs to build Electron.  
    There might be errors when you do it, fix them.

1.  Set `ffmpeg_branding = "CustomName"` build arg to use a custom config and build Electron.  
    Fix any build errors by editing FFmpeg build configs, commit your changes.

1.  Export commits that amend the new configs as patches to the `/patches/customization/ffmpeg/` directory.  
    Do not export commits that just copied existing configs.

1.  Make sure that FFmpeg patches have paths relative to the FFmpeg repository (`//src/third_party/ffmpeg`)  
    and not the Chromium repository (`//src`).
