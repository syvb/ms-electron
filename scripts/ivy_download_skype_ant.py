#!/usr/bin/env python

# Download skype-ant to defined folder (arg "--destination")

import argparse
import os
import sys
import zipfile

from lib.filesystem import get_temp_folder
from lib.two_to_three import download_file


skype_ant_version = '1.9.3.397'


def parse_args():
    parser = argparse.ArgumentParser(
        description="Download Skype Ant {0}".format(skype_ant_version))

    parser.add_argument('--destination',
                        required=True,
                        help="directory where to unzip skype-ant")
    parser.add_argument('--username',
                        required=True,
                        help="username to access Skype IVY")
    parser.add_argument('--password',
                        required=True,
                        help="password to access Skype IVY")

    return parser.parse_args()


def main():
    script_args = parse_args()

    full_url = 'https://{username}:{password}@skype.pkgs.visualstudio.com/' \
               '_packaging/csc/ivy/v1/skype/' \
               'tools_build/skype-ant/master/anyos-anycpu/' \
               '{version}/skype-ant-{version}.zip'.format(**{
        'username': script_args.username,
        'password': script_args.password,
        'version': skype_ant_version,
    })

    with get_temp_folder() as temp_folder:
        destination_path = os.path.join(temp_folder, 'skype-ant.zip')
        download_file(full_url, destination_path)

        z = zipfile.ZipFile(destination_path)
        z.extractall(path=script_args.destination)


if __name__ == '__main__':
    main()
