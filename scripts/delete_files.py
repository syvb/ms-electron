#!/usr/bin/env python

"""
Delete files and directories.
"""

import argparse
import os
import shutil


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument('--root-dir',
                        type=os.path.abspath, default=os.getcwd(),
                        help="root directory for all other paths, "
                             "defaults to the current directory")
    parser.add_argument('--file', nargs='*',
                        help="file to delete")
    parser.add_argument('--directory', nargs='*',
                        help="directory to delete")

    return parser.parse_args()


def main():
    script_args = parse_args()

    for directory_path in script_args.directory:
        full_path = os.path.join(script_args.root_dir, directory_path)
        shutil.rmtree(full_path, ignore_errors=True)

    for file_path in script_args.file:
        full_path = os.path.join(script_args.root_dir, file_path)

        if os.path.exists(full_path):
            os.remove(full_path)


if __name__ == '__main__':
    main()
