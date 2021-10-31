import os
import subprocess
import sys

from .filesystem import print_file


def gclient(*gclient_args):
    # Convert it from a tuple.
    gclient_args = list(gclient_args)
    executable = __get_executable_name()

    env = os.environ.copy()

    # This tells depot_tools to use your locally
    # installed version of Visual Studio (by default,
    # depot_tools will try to use a google-internal version).
    env['DEPOT_TOOLS_WIN_TOOLCHAIN'] = '0'

    # Don't try to update depot tools to the latest version
    # before running gclient.
    # https://www.chromium.org/developers/how-tos/depottools#TOC-Disabling-auto-update
    env['DEPOT_TOOLS_UPDATE'] = '0'

    # There are some issues on Microsoft-hosted build agents right now.
    # Managed Python works ok.
    env['VPYTHON_BYPASS'] = "manually managed python not supported by chrome operations"

    call_args = [executable] + gclient_args
    subprocess.check_call(call_args, env=env)


def __get_executable_name():
    executable = 'gclient'
    if sys.platform == 'win32':
        executable += '.bat'
    return executable


def config(url, name=None, custom_vars=None, cache_dir=None,
           print_the_config=False):
    gclient_args = ['config', '--unmanaged']

    if name is not None:
        gclient_args += ['--name', name]
    if custom_vars is not None:
        gclient_args += map(lambda i: __format_custom_var_argument(*i),
                            custom_vars.items())

    # Always pass the '--cache-dir' option to ignore the `GIT_CACHE_PATH`
    # environment variable. If `cache_dir` is `None`, pass a "None" string.
    gclient_args += ['--cache-dir', str(cache_dir)]
    gclient_args.append(url)

    gclient(*gclient_args)

    if print_the_config:
        print_file('.gclient')


def __format_custom_var_argument(name, value):
    # Let's pass all arguments' values in quotes except booleans.
    string_value = value if isinstance(value, bool) else '"{}"'.format(value)
    return '--custom-var={}={}'.format(name, string_value)
