import sys


class CanonicalPlatform:
    """Human friendly platform identifiers.
    The set intended to be used as expected values provided by a user,
    e.g. as script arguments.
    """
    LINUX = 'linux'
    MAC = 'mac'
    MAS = 'mas'  # Mac App Store
    WINDOWS = 'windows'

    @staticmethod
    def get_all():
        return (
            CanonicalPlatform.LINUX,
            CanonicalPlatform.MAC,
            CanonicalPlatform.MAS,
            CanonicalPlatform.WINDOWS,
        )

    @staticmethod
    def is_current(platform):
        # When checking MAS, check MAC instead.
        if (platform == CanonicalPlatform.MAS):
            platform = CanonicalPlatform.MAC

        return platform == CanonicalPlatform.from_system()

    @staticmethod
    def from_system():
        """See https://docs.python.org/2/library/sys.html#sys.platform
        for a list of all possible values.
        """
        system_platform = sys.platform

        if system_platform.startswith('linux'):
            return CanonicalPlatform.LINUX

        if system_platform == 'darwin':
            return CanonicalPlatform.MAC

        if system_platform in ('cygwin', 'win32'):
            return CanonicalPlatform.WINDOWS

        raise Exception(
            'unexpected system platform "{}"'.format(system_platform))
