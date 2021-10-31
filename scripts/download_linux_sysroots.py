#!/usr/bin/env python

import argparse
import json
import os

from lib.filesystem import mkdir_p
from lib.two_to_three import download_file


def parse_args():
    parser = argparse.ArgumentParser(
        description="Download Linux sysroots used by Electron")
    parser.add_argument('--config',
                        type=argparse.FileType('r'), required=True,
                        help='sysroots\' config in the JSON format')
    parser.add_argument('--save-to',
                        type=os.path.abspath, required=True,
                        help="directory to save sysroots to")
    return parser.parse_args()


def download_sysroot(tarball_sha1sum, tarball_filename, output_dir):
    # Those URL parts are defined in "patches/common/chromium/sysroot.patch"
    # file in the Electron repo. We can't reuse them from there.
    URL_PREFIX = 'http://s3.amazonaws.com'
    URL_PATH = 'electronjs-sysroots/toolchain'

    # The pattern below is defined in the Chromium's file
    # "build/linux/sysroot_scripts/install-sysroot.py".
    url = '%s/%s/%s/%s' % (URL_PREFIX, URL_PATH,
                           tarball_sha1sum, tarball_filename)

    # Append `tarball_sha1sum` to the path. Make sure the folder exists.
    output_path = os.path.join(output_dir, tarball_sha1sum, tarball_filename)
    mkdir_p(os.path.dirname(output_path))

    download_file(url, output_path)


def download_sysroots(config, output_dir):
    for sysroot_data in config.values():
        tarball_sha1sum = sysroot_data['Sha1Sum']
        tarball_filename = sysroot_data['Tarball']
        download_sysroot(tarball_sha1sum, tarball_filename, output_dir)


def main():
    script_args = parse_args()

    config = json.load(script_args.config)
    download_sysroots(config=config,
                      output_dir=script_args.save_to)


if __name__ == '__main__':
    main()
