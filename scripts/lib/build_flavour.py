class BuildFlavour:
    ELECTRON = 'electron'
    MEDIA = 'media'
    TEAMS = 'teams'
    VSCODE = 'vscode'

    @staticmethod
    def get_all():
        return (
            BuildFlavour.ELECTRON,
            BuildFlavour.MEDIA,
            BuildFlavour.TEAMS,
            BuildFlavour.VSCODE,
        )

    @staticmethod
    def get_description(flavour):
        if flavour == BuildFlavour.ELECTRON:
            return "App Center support, no FFmpeg codecs"
        if flavour == BuildFlavour.MEDIA:
            return "App Center support, proprietary FFmpeg codecs H264/AAC"
        if flavour == BuildFlavour.TEAMS:
            return "App Center and Watson support, calling stack integration,"\
                   " proprietary FFmpeg codecs H264/AAC"
        if flavour == BuildFlavour.VSCODE:
            return "reduced size, App Center support, no FFmpeg codecs"
        return None
