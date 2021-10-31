#!/usr/bin/env python

import argparse
import json
import os

import lib.filesystem as fs
from lib.git import Repository


def parse_args():
    parser = argparse.ArgumentParser(
        description="Change defines in a FFmpeg build config")

    parser.add_argument('-b', '--build-dir',
                        type=os.path.abspath, required=True,
                        help="FFmpeg build configs directory path")
    parser.add_argument('-c', '--config',
                        type=argparse.FileType('r'), required=True,
                        help="a custom config's config path")
    parser.add_argument('--commit',
                        required=False, action='store_true',
                        help="commit the changes made")

    return parser.parse_args()


def main():
    script_args = parse_args()

    config = json.load(script_args.config)
    custom_config_name = config['name']
    custom_config_path = os.path.join(script_args.build_dir,
                                      custom_config_name)
    defines = config['defines']

    change_defines_in_dir(custom_config_path, defines)

    if script_args.commit:
        commit_changes(custom_config_path, custom_config_name)


def change_defines_in_dir(dir_path, defines):
    lines_generator = fs.for_every_line(dir_path)

    changed_line = None  # None has to be sent to the first `send()` call.
    while True:
        try:
            line = lines_generator.send(changed_line)
            changed_line = change_defines_in_line(line, defines)
        except StopIteration:
            break


def change_defines_in_line(line, defines):
    changed_line = None

    for define, value in defines.items():
        pattern = 'define {} '.format(define)
        index = line.find(pattern)
        if index != -1:
            # Assuming there's nothing except a define value
            # at the end of the line.
            changed_line = line[:index] + pattern + value + '\n'

    return changed_line


def commit_changes(directory, config_name):
    repo = Repository(directory)
    commit_message = "build: patch the {} config of FFmpeg".format(config_name)
    repo.git('add', directory)
    repo.git('commit', '--message', commit_message)


if __name__ == '__main__':
    main()
