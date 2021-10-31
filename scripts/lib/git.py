import subprocess


def git(*git_args):
    call_args = ['git'] + list(git_args)
    return subprocess.check_output(call_args)


class GitUser(object):
    def __init__(self, name, email):
        self._name = name
        self._email = email

    @property
    def name(self):
        return self._name

    @property
    def email(self):
        return self._email

    @property
    def full_name(self):
        return "{} <{}>".format(self._name, self._email)


fake_user = GitUser(name="Electron at Microsoft",
                    email="electron@microsoft.com")


class Repository(object):
    def __init__(self, directory, user=fake_user, init=False):
        self._directory = directory
        self._user = user

        if init:
            self.git('init')

    @property
    def user(self):
        return self._user

    def git(self, *git_args):
        additional_args = [
            '-C', self._directory,
            '-c', 'user.name="{}"'.format(self._user.name),
            '-c', 'user.email="{}"'.format(self._user.email),
        ]
        all_args = additional_args + list(git_args)
        return git(*all_args)


def bypass_cred_scan_hook(repo):
    # This magic disables the CredScan hook.
    commit_message = "**DISABLE_SECRET_SCANNING**"
    repo.git('commit', '--allow-empty', '--message', commit_message)
