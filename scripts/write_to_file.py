#!/usr/bin/env python

import argparse
import os

from lib.filesystem import write_to_file


def parse_args():
    parser = argparse.ArgumentParser(description="Writes text to a file")
    parser.add_argument('--text', required=True)
    parser.add_argument('--file-path',
                        type=os.path.abspath, required=True,
                        help="path to a file")
    return parser.parse_args()


def main():
    script_args = parse_args()
    write_to_file(text=script_args.text,
                  file_path=script_args.file_path,
                  replace_contents=True)


if __name__ == '__main__':
    main()
