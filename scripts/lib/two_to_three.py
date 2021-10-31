"""Helpers for Python 2 to Python 3 migration.
"""

try:
    # Python 3
    from urllib.request import urlretrieve
except ImportError:
    # Python 2
    from urllib import urlretrieve


def is_string(value):
    """Should be replaced by `isinstance(value, str)` in Python 3 code."""
    return isinstance(value, ("".__class__, u"".__class__))


def download_file(url, dest_path):
    return urlretrieve(url, dest_path)
