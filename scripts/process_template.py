#!/usr/bin/env python

import argparse
import os
import string

import lib.filesystem as fs


def parse_args():
    parser = argparse.ArgumentParser(
        description="Process a template text file")

    parser.add_argument('--template',
                        type=argparse.FileType('r'), required=True,
                        help="template to process")
    parser.add_argument('--data', nargs='+',
                        required=True,
                        help="key=value format")
    parser.add_argument('--output-dir',
                        required=True,
                        help="directory to write a resulting file to")
    parser.add_argument('--safe', action='store_true',
                        help="use the 'safe' substitute strategy")

    return parser.parse_args()


def process_template(template_string, substitutions, safe=False):
    template_object = string.Template(template_string)
    if safe:
        result = template_object.safe_substitute(substitutions)
    else:
        result = template_object.substitute(substitutions)
    return result


def parse_substitutions(data):
    return dict(pair.split('=') for pair in data)


def get_output_file_path(input_file, output_dir):
    base_name = os.path.basename(input_file)
    file_path = os.path.join(output_dir, base_name)
    return file_path


def main():
    script_args = parse_args()

    substitutions = parse_substitutions(script_args.data)
    template_contents = script_args.template.read()
    processed_template = process_template(template_contents,
                                          substitutions,
                                          script_args.safe)

    file_path = get_output_file_path(script_args.template.name,
                                     script_args.output_dir)
    fs.write_to_file(text=processed_template, file_path=file_path)


if __name__ == '__main__':
    main()
