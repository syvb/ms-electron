#!/usr/bin/env python

import argparse

from lib.gclient import gclient


def gclient_sync(revision=None, run_hooks=True):
    gclient_args = [
      'sync',
      '--force',
      '--with_branch_heads',
      '--with_tags',
    ]
    if revision:
        gclient_args += ['--revision', revision]
    if not run_hooks:
        gclient_args.append('--nohooks')

    gclient(*gclient_args)


def main():
    parser = argparse.ArgumentParser(
        description="Sync project dependencies using gclient\n"
                    "https://www.chromium.org/developers/how-tos/depottools",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('-r', '--revision', required=False,
                        help='enforces revision/hash for the solution')
    parser.add_argument('--no-hooks', action='store_false', dest='run_hooks',
                        help="don't run hooks after the update is complete")
    args = parser.parse_args()

    gclient_sync(args.revision, run_hooks=args.run_hooks)


if __name__ == '__main__':
    main()
