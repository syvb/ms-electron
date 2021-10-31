#!/usr/bin/env python

from __future__ import print_function

import argparse
import json
import os

from lib.filesystem import mkdir_p
from lib.two_to_three import download_file


def parse_args():
    parser = argparse.ArgumentParser(
        description="Mirror external binaries for Electron")

    parser.add_argument('-c', '--config', dest='config_file',
                        type=argparse.FileType('r'), required=True,
                        help="a path to a config file in JSON format")
    parser.add_argument('--save-to',
                        type=os.path.abspath, required=True,
                        help="a path to a directory to save files to")

    return parser.parse_args()


def get_download_url(base_url, file_info):
    return "{}/{}/{}".format(base_url, file_info['sha'], file_info['name'])


def get_destination_path(output_dir, file_info):
    return os.path.join(output_dir, file_info['sha'], file_info['name'])


def get_binaries_to_download(config, output_dir):
    """Create a list of [{'url': 'save_path'}], ...]."""

    base_url = config['baseUrl']
    files = config['files']

    downloads = list(map(lambda file_info: {
        'url': get_download_url(base_url, file_info),
        'dest_path': get_destination_path(output_dir, file_info)
    }, files))
    return downloads


def download_binaries(config, output_dir):
    downloads = get_binaries_to_download(config, output_dir)
    for download in downloads:
        base_dir = os.path.dirname(download['dest_path'])
        mkdir_p(base_dir)
        download_file(download['url'], download['dest_path'])


def main():
    script_args = parse_args()

    config = json.load(script_args.config_file)
    download_binaries(config, script_args.save_to)


if __name__ == '__main__':
    main()
