#!/usr/bin/env python

import argparse
import os
import shutil

import lib.filesystem as fs


def parse_args():
    parser = argparse.ArgumentParser(
        description="Create a clean copy of a Chromium tree")
    parser.add_argument('--chromium-path',
                        type=os.path.abspath, required=True,
                        help="Chromium checkout path")
    parser.add_argument('--copy-path', required=True,
                        type=os.path.abspath,
                        help="path to a clean copy folder (must not exist)")
    return parser.parse_args()


def main():
    script_args = parse_args()
    clean_copy(script_args.chromium_path,
               script_args.copy_path)


def clean_copy(chromium_path, copy_path):
    # Avoid copying some files and folders.
    ignore = shutil.ignore_patterns(*[
        # We don't need Git stuff.
        '.git',
        '.gitignore',

        # Those are heavy and we aren't going to use them anyway.
        'LayoutTests',

        # Some files have weird Unicode filenames
        # and `shutil.copytree()` fails on them.
        'android_webview',  # //src/android_webview
        'perf_tests',   # //src/third_party/blink/perf_tests
        'swarming_client',  # //src/tools/swarming_client/
        'web_tests',  # //src/third_party/blink/web_tests/
        'webvr_info',  # //src/chrome/test/data/xr/webvr_info
    ])
    shutil.copytree(chromium_path, copy_path,
                    ignore=ignore, symlinks=True)

    # We don't want to modify the original tree,
    # so all renames should happen in the copy.

    # We don't want DEPS to be processed by gclient.
    # It's OK to rename only the top-level file.
    fs.rename_file(copy_path,
                   src='DEPS',
                   dst='DEPS.disabled')


if __name__ == '__main__':
    main()
