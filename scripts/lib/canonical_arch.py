class CanonicalArch:
    """Human friendly arch identifiers.
    The set intended to be used as expected values provided by a user,
    e.g. as script arguments.
    """
    ARM = 'arm'
    ARM64 = 'arm64'
    X64 = 'x64'
    X86 = 'x86'

    @staticmethod
    def get_all():
        return (
            CanonicalArch.ARM,
            CanonicalArch.ARM64,
            CanonicalArch.X64,
            CanonicalArch.X86,
        )
