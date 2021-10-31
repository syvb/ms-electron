import os
import subprocess
import sys

from .project_paths import SRC_DIR


def gn(*gn_args):
    # Convert it from a tuple.
    gn_args = list(gn_args)
    executable = __get_executable_name()

    env = os.environ.copy()

    # To find the `gn` executable.
    env['CHROMIUM_BUILDTOOLS_PATH'] = __get_buildtools_path()

    # This tells depot_tools to use your locally
    # installed version of Visual Studio (by default,
    # depot_tools will try to use a google-internal version).
    env['DEPOT_TOOLS_WIN_TOOLCHAIN'] = '0'

    call_args = [executable] + gn_args
    subprocess.check_call(call_args, env=env)


def __get_executable_name():
    executable = 'gn'
    if sys.platform == 'win32':
        executable += '.bat'
    return executable


def __get_buildtools_path():
    return os.path.join(SRC_DIR, 'buildtools')


if __name__ == '__main__':
    gn(*sys.argv[1:])
