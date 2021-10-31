#!/usr/bin/env python

"""
Generate a wiki page for a Microsoft Electron release.
https://microsoft.sharepoint.com/teams/ElectronInMicrosoft/Wiki/Home.aspx

Uses Jinja2 template language, see http://jinja.pocoo.org/docs/2.10/.
"""

# Examples:
#
# python generate_release_wiki.py -v 4.2.7 \
# -b media=3165963 electron=3168414 vscode=3168415 teams=3173003
#
# python generate_release_wiki.py -o output.html -v 4.2.7 \
# -b media=3165963 electron=3168414 vscode=3168415 teams=3173003
#
# python generate_release_wiki.py -v 3.1.12 \
# -b electron=17641712 vscode=17730073 teams=17730003

import argparse
import os

from jinja2 import Environment, FileSystemLoader
from lib.args import KeyValuePairs
from lib.build_flavour import BuildFlavour
from lib.filesystem import write_to_file
from lib.project_paths import REPO_ROOT_DIR
from lib.versions import get_electron_version_from_microsoft_deps


TEMPLATE_DIRECTORY = os.path.join(REPO_ROOT_DIR, 'configs', 'templates')
TEMPLATE_FILE_NAME = 'release_wiki_page.html'


def get_short_version(electron_version_string):
    version_components = electron_version_string.split('.')
    return '{0}.{1}'.format(version_components[0], version_components[1])


def get_template_data(electron_version, build_ids):
    # Preserve flavours' order in the resulting list.
    flavours = sorted(build_ids.keys())

    builds_data = map(lambda flavour: {
        'description': BuildFlavour.get_description(flavour),
        'flavour': flavour,
        'id': build_ids[flavour],
    }, flavours)

    template_data = {
        'version': electron_version,
        'short_version': get_short_version(electron_version),
        'base_url': 'https://electron.blob.core.windows.net/builds',
        'builds': builds_data,
    }
    return template_data


def process_template(template_data):
    env = Environment(loader=FileSystemLoader(TEMPLATE_DIRECTORY))
    template = env.get_template(TEMPLATE_FILE_NAME)
    return template.render(electron=template_data).encode('utf-8')


def generate_wiki_page(electron_version, build_ids):
    template_data = get_template_data(electron_version, build_ids)
    return process_template(template_data)


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(
        '-v', '--version', required=False,
        default=get_electron_version_from_microsoft_deps(strip_leading_v=True),
        help="Electron version")
    parser.add_argument(
        '-o', '--output-file', required=False, type=os.path.abspath,
        help="optional output file path")
    all_flavours = BuildFlavour.get_all()
    parser.add_argument(
        '-b', '--build', required=True,
        nargs=len(all_flavours), action=KeyValuePairs,
        help="build flavours and ids in the 'flavour_name=build_id' format. "
             "Current flavours are [{0}]".format(', '.join(all_flavours)))

    return parser.parse_args()


def main():
    script_args = parse_args()

    wiki_page_data = generate_wiki_page(
        script_args.version, script_args.build)

    if script_args.output_file is None:
        print wiki_page_data
    else:
        write_to_file(wiki_page_data, script_args.output_file,
            replace_contents=True)


if __name__ == '__main__':
    main()
