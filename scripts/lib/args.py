"""
Various utils to work with Python script arguments.
"""

import argparse

from lib.two_to_three import is_string


def boolean_string(value):
    if is_string(value):
        lowercased_value = value.lower()

        if lowercased_value == 'true':
            return True
        if lowercased_value == 'false':
            return False

    raise Exception("got unexpected value {}".format(value))


def parse_key_value_pairs(key_value_pairs):
    if key_value_pairs is None:
        return None

    names_and_values = map(lambda s: s.split('='), key_value_pairs)
    result = dict(names_and_values)
    return result


class KeyValuePairs(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, parse_key_value_pairs(values))


def comma_separated_list(string):
    values = filter(lambda v: v != '', string.split(','))
    return values
