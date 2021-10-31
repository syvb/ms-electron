#!/usr/bin/env python

"""
Generate Ninja.
"""

import argparse
import os

from lib.args import boolean_string
from lib.build_flavour import BuildFlavour
from lib.canonical_platform import CanonicalPlatform
from lib.filesystem import print_file, read_file
from lib.gn import gn
from lib.project_paths import REPO_ROOT_DIR


class BuildConfig:
    FFMPEG = 'ffmpeg'
    TESTING = 'testing'
    RELEASE = 'release'

    @staticmethod
    def get_all():
        return (
            BuildConfig.FFMPEG,
            BuildConfig.TESTING,
            BuildConfig.RELEASE,
        )

    @staticmethod
    def get_import_path(name):
        return '//electron/build/args/{}.gn'.format(name)


def get_build_flavour_config_path(flavour):
    path = os.path.join(REPO_ROOT_DIR, 'build', 'config',
                        '{}.args.gni'.format(flavour))
    return path


class GetBuildFlavourConfigPath(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        config_path = get_build_flavour_config_path(flavour=values)
        setattr(namespace, self.dest, config_path)


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument('out_dir', type=os.path.abspath,
                        help="output dir path, e.g. 'out/testing'")
    parser.add_argument('--args', nargs='*',
                        help="build arguments in 'key=value' format "
                             "to override values in configs. "
                             "Escape double quotes for string values, "
                             "e.g. target_cpu=\\\"x64\\\"")
    parser.add_argument('--for', dest='build_flavour_config', required=True,
                        choices=BuildFlavour.get_all(),
                        action=GetBuildFlavourConfigPath,
                        help="a build flavour")
    parser.add_argument('--print',
                        dest='print_the_config',  # Can't use `args.print`.
                        action='store_true', default=False,
                        help='print the config created')
    # https://chromium.googlesource.com/chromium/src/+/HEAD/docs/asan.md
    parser.add_argument('--asan',
                        type=boolean_string, nargs='?',
                        default=False, const=True,
                        help="build with ASan")
    parser.add_argument('--mas',
                        type=boolean_string, nargs='?',
                        default=False, const=True,
                        help="build for Mac App Store")

    config = parser.add_mutually_exclusive_group(required=True)
    config.add_argument('--build-config',
                        choices=BuildConfig.get_all(),
                        help="a build config type")
    for build_config in BuildConfig.get_all():
        config.add_argument('--{}'.format(build_config), dest='build_config',
                            action='store_const', const=build_config,
                            help="use the {} build config".format(build_config))

    args = parser.parse_args()

    # Additional rules.
    if args.mas and not CanonicalPlatform.is_current(CanonicalPlatform.MAC):
        raise parser.error("Build for Mac App Store is available only on macOS")

    if args.build_config is not None:
        args.build_config = BuildConfig.get_import_path(args.build_config)

    return args


def gn_gen(build_dir, build_args=None, print_the_config=False):
    gn_args = ['gen', build_dir]
    if build_args:
        build_args_string = '--args=' + ' '.join(build_args)
        gn_args.append(build_args_string)
    gn(*gn_args)

    if print_the_config:
        config_path = os.path.join(build_dir, 'args.gn')
        print_file(config_path)


def get_build_args(configs_to_import=None, configs_to_inline=None,
                   other_args=None):
    build_args = []

    # Import configs first.
    if configs_to_import:
        build_args += generate_gn_import_statements(configs_to_import)

    # Then inline values from other configs.
    if configs_to_inline:
        for config_path in configs_to_inline:
            build_args += list_values(config_path)

    # And the other args should go last to override configs' values.
    if other_args:
        build_args += other_args

    return build_args


def generate_gn_import_statements(paths):
    return ['import("{}")'.format(p)
            for p in paths if p is not None]


def list_values(file_path=None):
    if file_path is None:
        return []

    lines = read_file(file_path, as_lines=True)
    values = [extract_key_value_pair(line) for line in lines]
    return values


def extract_key_value_pair(line):
    # Strip a comment.
    if '#' in line:
        index = line.index('#')
        line = line[:index]

    # Strip whitespace.
    line = line.strip()

    return line


def main():
    script_args = parse_args()

    build_args = get_build_args(configs_to_import=[
        script_args.build_config,
    ], configs_to_inline=[
        script_args.build_flavour_config,
        (get_build_flavour_config_path('asan') if script_args.asan else None),
        (get_build_flavour_config_path('mas') if script_args.mas else None),
    ], other_args=script_args.args)

    gn_gen(build_dir=script_args.out_dir,
           build_args=build_args,
           print_the_config=script_args.print_the_config)


if __name__ == '__main__':
    main()
