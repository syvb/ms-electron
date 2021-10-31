#!/usr/bin/env python

"""
Get Electron version.
"""

from __future__ import print_function

import argparse
import sys

from lib.versions import get_electron_version_from_microsoft_deps


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument('--from-microsoft-deps', action='store_true',
                        help="get Electron version "
                             "from the Microsoft DEPS file")

    return parser.parse_args()


def main():
    script_args = parse_args()

    if script_args.from_microsoft_deps:
        version = get_electron_version_from_microsoft_deps()
        print(version, end='')
        return 0

    return 0


if __name__ == '__main__':
    sys.exit(main())
