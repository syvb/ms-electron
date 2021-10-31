#!/usr/bin/env python

"""Copies a msdia*.dll to a location where it can be found by a dump_syms.exe
binary when it's run to generate breakpad symbols on Windows.
See the "Hacks and Workarounds" doc for details.
"""

import argparse
import os
import shutil
import sys

from lib.canonical_arch import CanonicalArch
from lib.project_paths import SRC_DIR

SRC_BUILD_DIR = os.path.join(SRC_DIR, 'build')
sys.path.append(SRC_BUILD_DIR)

import vs_toolchain  # pylint: disable=wrong-import-position

# True for VS 2017, can be changed in the future.
DIA_DLL_NAME = 'msdia140.dll'


def parse_args():
    parser = argparse.ArgumentParser(
        description="Copy {} to a directory where Electron build scripts "
                    "expect it to be".format(DIA_DLL_NAME))

    parser.add_argument('--arch', choices=CanonicalArch.get_all(),
                        required=True,
                        help="Target CPU arch of an Electron build")

    return parser.parse_args()


def main():
    script_args = parse_args()

    src = get_source_file_path(arch=script_args.arch)
    dst = get_destination_dir()
    shutil.copy(src, dst)


def get_source_file_path(arch):
    # Should be set to make Visual Studio detection work properly.
    os.environ['DEPOT_TOOLS_WIN_TOOLCHAIN'] = '0'

    visual_studio_path = vs_toolchain.DetectVisualStudioPath()
    dia_sdk_bin_path = os.path.join(visual_studio_path, 'DIA SDK', 'bin')

    arch_dir = {
        CanonicalArch.ARM64: 'amd64',  # Use the x64 version.
        CanonicalArch.X64: 'amd64',
        CanonicalArch.X86: '.',
    }[arch]
    dll_path = os.path.join(dia_sdk_bin_path, arch_dir, DIA_DLL_NAME)

    return dll_path


def get_destination_dir():
    return os.path.join(
        SRC_DIR, 'third_party', 'llvm-build', 'Release+Asserts', 'bin')


if __name__ == '__main__':
    main()
