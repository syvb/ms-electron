#!/usr/bin/env python

import sys

from lib.gclient import gclient


if __name__ == '__main__':
    gclient(*sys.argv[1:])
