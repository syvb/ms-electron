#!/usr/bin/env python

import argparse
import json
import os
import shutil

from lib.git import Repository


def parse_args():
    parser = argparse.ArgumentParser(
        description="Create a copy of an existing build config of FFmpeg "
                    "with a different name")

    parser.add_argument('--build-dir',
                        type=os.path.abspath, required=True,
                        help="FFmpeg build configs directory path")
    parser.add_argument('--config', nargs='+',
                        type=argparse.FileType('r'), required=True,
                        help="a custom config's config path")

    return parser.parse_args()


def copy_and_commit_config(configs_dir, base_config, new_config):
    new_config_dir = create_config_copy(configs_dir, base_config, new_config)

    repo = Repository(new_config_dir)
    repo.git('add', '.')

    commit_message = "build: copy FFmpeg's {} config to {}".format(
        base_config, new_config)
    repo.git('commit',
             '--author', repo.user.full_name,
             '--message', commit_message)


def create_config_copy(configs_dir, base_config, new_config):
    base_config_dir = os.path.join(configs_dir, base_config)
    new_config_dir = os.path.join(configs_dir, new_config)

    # The new dir must not exist before we create a copy.
    shutil.rmtree(new_config_dir, ignore_errors=True)
    shutil.copytree(base_config_dir, new_config_dir)

    return new_config_dir


def main():
    script_args = parse_args()

    for config_file in script_args.config:
        config = json.load(config_file)
        copy_and_commit_config(configs_dir=script_args.build_dir,
                               base_config=config['base_config'],
                               new_config=config['name'])


if __name__ == '__main__':
    main()
