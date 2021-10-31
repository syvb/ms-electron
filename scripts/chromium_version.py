#!/usr/bin/env python

"""
Get or validate Chromium version
"""

from __future__ import print_function

import argparse
import sys

from lib.versions import get_chromium_version_from_electron_deps, \
    get_chromium_version_from_version_file


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)

    action = parser.add_mutually_exclusive_group()
    action.add_argument('-v', '--validate', action='store_true',
                        help="check that versions in the Electron DEPS "
                             "and in the VERSION file match")
    action.add_argument('--from-electron-deps', action='store_true',
                        help="get version from the Electron DEPS file")
    action.add_argument('--from-sources', action='store_true',
                        help="get version from the VERSION file")

    return parser.parse_args()


def main():
    script_args = parse_args()

    if script_args.validate:
        version_from_deps = get_chromium_version_from_electron_deps()
        version_from_sources = get_chromium_version_from_version_file()
        versions_match = (version_from_deps == version_from_sources)

        if not versions_match:
            error_message = "expected Chromium {0}, got {1}".format(
                version_from_sources, version_from_deps)
            print(error_message, file=sys.stderr)

        exit_code = 0 if versions_match else 1
        return exit_code

    if script_args.from_electron_deps:
        version = get_chromium_version_from_electron_deps()
        print(version, end='')
        return 0

    if script_args.from_sources:
        version = get_chromium_version_from_version_file()
        print(version, end='')
        return 0

    return 0


if __name__ == '__main__':
    sys.exit(main())
