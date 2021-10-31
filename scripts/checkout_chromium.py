#!/usr/bin/env python

"""
Check out Chromium from the Google repo.
"""

import argparse
import os
import subprocess

from lib.canonical_platform import CanonicalPlatform
from lib.deps import get as get_deps
from lib.filesystem import mkdir_p
from lib.gclient import gclient, config as gclient_config
from lib.project_paths import ELECTRON_DIR, REPO_ROOT_DIR, SRC_DIR
from gclient_sync import gclient_sync

CHROMIUM_REPO_URL = 'https://chromium.googlesource.com/chromium/src.git'


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-r', '--revision',
                        required=True,
                        help="e.g. '69.0.3497.106'")
    parser.add_argument('-u', '--url',
                        required=False, default=CHROMIUM_REPO_URL,
                        help="the repository URL")
    parser.add_argument('-o', '--output-dir',
                        required=False, default=os.getcwd(),
                        help="defaults for the current directory")
    parser.add_argument('--git-cache',
                        required=False, default=None,
                        help="git cache directory")

    return parser.parse_args()


def install_linux_build_deps(checkout_dir):
    script_to_run = os.path.join(checkout_dir,
                                 'src', 'build', 'install-build-deps.sh')
    script_args = [
        '--arm',
        '--lib32',
        '--no-chromeos-fonts',
        '--no-prompt',
        '--syms',
    ]

    call_args = [script_to_run] + script_args
    subprocess.check_call(call_args)


def checkout_chromium(revision, repo_url=CHROMIUM_REPO_URL, git_cache=None):
    # See https://chromium.googlesource.com/chromium/src/+/HEAD/docs/linux_build_instructions.md  # pylint: disable=line-too-long
    is_linux = CanonicalPlatform.is_current(CanonicalPlatform.LINUX)
    run_hooks_with_sync = not is_linux

    # Create a gclient config in the current dir
    # passing all "vars" from the Electron DEPS as custom vars.
    custom_vars = get_deps(ELECTRON_DIR)['vars']
    gclient_config(url=repo_url,
                   cache_dir=git_cache,
                   custom_vars=custom_vars,
                   print_the_config=True)

    # Get dependencies.
    gclient_sync(revision=revision,
                 run_hooks=run_hooks_with_sync)

    if is_linux:
        install_linux_build_deps(checkout_dir='.')

    # Run hooks if they haven't been run already.
    if not run_hooks_with_sync:
        gclient('runhooks')


def main():
    script_args = parse_args()

    mkdir_p(script_args.output_dir)
    os.chdir(script_args.output_dir)

    checkout_chromium(revision=script_args.revision,
                      repo_url=script_args.url,
                      git_cache=script_args.git_cache)


if __name__ == '__main__':
    main()
