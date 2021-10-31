#!/usr/bin/env python

import argparse
import json

import lib.filesystem as fs
from lib.git import Repository, bypass_cred_scan_hook


def parse_args():
    parser = argparse.ArgumentParser(description="Mirror a Git repo")

    parser.add_argument('--config', dest='config_file',
                        type=argparse.FileType('r'), required=True,
                        help="a path to a config file in JSON format")

    return parser.parse_args()


def main():
    script_args = parse_args()

    config = json.load(script_args.config_file)
    mirror_git_repo(url_from=config['from'],
                    url_to=config['to'],
                    branches=config['branches'],
                    force_push=config.get('force_push', False),
                    bypass_cred_scan=config.get('bypass_cred_scan', False),
                    tags=config.get('tags', False))


def mirror_git_repo(url_from, url_to, branches, bypass_cred_scan=False,
                    force_push=False, tags=False):
    maybe_force = '--force' if force_push else '--no-force'
    maybe_fetch_tags = '--tags' if tags else '--no-tags'
    maybe_push_tags = '--follow-tags' if tags else '--no-tags'

    with fs.get_temp_folder() as temp_folder:
        # Could have used `--bare` repo
        # if we didn't have to bypass the CredScan hook.
        repo = Repository(temp_folder, init=True)

        repo.git('remote', 'add', 'upstream', url_from)
        repo.git('fetch', maybe_fetch_tags, 'upstream', *branches)

        if bypass_cred_scan:
            bypass_cred_scan_for(repo=repo, remote='upstream',
                                 branches=branches)

        # Push local modified branches if we're bypassing the CredScan hook.
        refs_to_push_template = '{0}:refs/heads/{0}' if bypass_cred_scan \
            else 'upstream/{0}:refs/heads/{0}'
        refs_to_push = [refs_to_push_template.format(b) for b in branches]

        repo.git('remote', 'add', 'downstream', url_to)
        repo.git('push', maybe_force, maybe_push_tags,
                 'downstream', *refs_to_push)


def bypass_cred_scan_for(repo, remote, branches):
    for branch in branches:
        repo.git('checkout', '--track', '{0}/{1}'.format(remote, branch))
        bypass_cred_scan_hook(repo)


if __name__ == '__main__':
    main()
