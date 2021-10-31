#!/usr/bin/env python

"""
Save a Chromium snapshot to the internal storage.
"""

import argparse
import os
import shutil

from lib.args import boolean_string
from lib.canonical_platform import CanonicalPlatform
from lib.chromium_version import ChromiumVersion, ChromiumVersionWithSuffix
from lib.filesystem import get_temp_folder
from lib.git import Repository, bypass_cred_scan_hook
from lib.versions import get_chromium_version_from_version_file

REPO_URL = 'https://domoreexp.visualstudio.com/DefaultCollection/' \
           'Teamspace/_git/electron-chromium'


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform',
                        required=False, choices=CanonicalPlatform.get_all(),
                        default=CanonicalPlatform.from_system())
    parser.add_argument('--replace',
                        type=boolean_string, nargs='?',
                        default=False, const=True,
                        help="replace existing snapshot")
    parser.add_argument('--snapshot-dir',
                        required=True,
                        help="path to a directory with a Chromium snapshot")
    return parser.parse_args()


def get_tags(repo, filter_function):
    repo.git('fetch', 'origin', 'refs/tags/*:refs/tags/*')
    tags_string = repo.git('tag', '--list')
    tags = tags_string.split('\n')
    filtered_tags = filter(filter_function, tags)
    return filtered_tags


def reset_parent_version(repo, version, platform):
    repo.git('remote', 'add', 'origin', REPO_URL)

    # Find the closest parent revision for this platform.
    tags = get_tags(repo, lambda t: t.endswith(platform))
    tags_as_versions = map(ChromiumVersionWithSuffix.parse, tags)
    parent_tag = version.get_closest_parent(tags_as_versions)

    if parent_tag is None:
        message = "no parent revision found for version {0} on {1}".format(
            version, platform)
        raise Exception(message)

    repo.git('reset', '--soft', str(parent_tag))


def commit_and_push(repo, tag, description, replace=False):
    # Commit the snapshot contents.
    repo.git('add', '.')
    repo.git('commit', '--message', description)

    bypass_cred_scan_hook(repo)

    # Tag the snapshot and push it. There are no branches in that repo.
    maybe_force = '--force' if replace else '--no-force'
    repo.git('tag', maybe_force,
             '--annotate', '--message', description,
             tag, 'HEAD')
    repo.git('push', maybe_force, 'origin', tag)


def main():
    script_args = parse_args()

    platform = script_args.platform
    snapshot_dir = script_args.snapshot_dir

    chromium_version_string = get_chromium_version_from_version_file(
        src_directory=snapshot_dir)
    chromium_version = ChromiumVersion(chromium_version_string)

    # Reset to the closest revision in the snapshot dir.
    snapshot_repo = Repository(snapshot_dir, init=True)
    reset_parent_version(snapshot_repo,
                         version=chromium_version,
                         platform=platform)
    chromium_version_with_platform_suffix = ChromiumVersionWithSuffix(
        chromium_version_string, suffix=platform)
    commit_and_push(snapshot_repo,
                    tag=str(chromium_version_with_platform_suffix),
                    description=chromium_version_string,
                    replace=script_args.replace)


if __name__ == '__main__':
    main()
