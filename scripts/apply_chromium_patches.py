#!/usr/bin/env python

import argparse
import json
import os
import sys

from lib.args import boolean_string
from lib.git import fake_user
from lib.project_paths import ELECTRON_DIR

SCRIPTS_LIB_DIR = os.path.join(ELECTRON_DIR, 'script', 'lib')
sys.path.append(SCRIPTS_LIB_DIR)

import git  # pylint: disable=wrong-import-position
from patches import patch_from_dir  # pylint: disable=wrong-import-position


def parse_args():
    parser = argparse.ArgumentParser(
        description="Apply patches to Chromium and friends")

    parser.add_argument('config', nargs='+',
                        type=argparse.FileType('r'),
                        help="patches' config(s) in the JSON format")
    parser.add_argument('--to-a-chromium-snapshot',
                        type=boolean_string, nargs='?',
                        default=False, const=True,
                        help="if the target repo is a Chromium snapshot")

    return parser.parse_args()


def apply_patches(config, is_snapshot=False):
    # A Chromium snapshot misses some non-essential parts of the tree.
    # Keys of the dict are real repos paths,
    # e.g. 'src/v8', 'src/third_party/ffmpeg', etc.
    excluded_paths_for_repos = {
      'src': [
        'third_party/WebKit/LayoutTests/*',
      ],
    }

    for patch_dir, repo_path in config.items():
        repo, directory = get_target_repo_and_directory(repo_path, is_snapshot)
        excluded_paths = (excluded_paths_for_repos.get(repo_path, None)
                          if is_snapshot else None)

        git.am(repo=repo, directory=directory,
               exclude=excluded_paths,
               patch_data=patch_from_dir(patch_dir),
               committer_name=fake_user.name,
               committer_email=fake_user.email)


def is_snapshot_part(repo_path):
    # The Node.js repo is not a part of Chromium.
    if repo_path == 'src/third_party/electron_node':
        return False

    return True


def get_target_repo_and_directory(repo_path, is_snapshot):
    if not is_snapshot or not is_snapshot_part(repo_path):
        return repo_path, ''

    # In a Chromium snapshot all dependencies are stored
    # in the top-level repo not in nested repos.
    src_repo = 'src'

    # Let's exclude "src/" from a path,
    # e.g. "src/third_party/ffmpeg" -> "third_party/ffmpeg".
    directory, top_level_dir = exclude_top_level_dir(repo_path)
    assert(top_level_dir == src_repo)

    return src_repo, directory


def exclude_top_level_dir(some_path):
    # Don't use a system path separator.
    path_separator = '/'

    path_parts = some_path.split(path_separator)
    top_level_dir = path_parts[0]
    path_parts = path_parts[1:]
    path_without_top_level_dir = path_separator.join(path_parts)

    return path_without_top_level_dir, top_level_dir


def main():
    script_args = parse_args()

    is_snapshot = script_args.to_a_chromium_snapshot

    for config_json in script_args.config:
        apply_patches(config=json.load(config_json),
                      is_snapshot=is_snapshot)


if __name__ == '__main__':
    main()
