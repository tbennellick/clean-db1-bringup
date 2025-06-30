import platform

from west.commands import WestCommand  # your extension must subclass this
from west import log                   # use this for user output

import wget
from filehash import FileHash
from os.path import isdir, isfile
from os import mkdir, chdir, remove
import py7zr

#This command is doing a very similar thing to the zephyr setup.sh in the SDK bundles. This adds hash checking and is converted to windows

## How to update:
## go here: https://github.com/zephyrproject-rtos/sdk-ng/tags
## select the version.
# Go to downloads
# Copy the link for SDK Bundle > minimal > Windows
# Paste here:
SDK_MIN_URL = ("https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.1-rc1/zephyr-sdk-0.17.1-rc1_windows-x86_64_minimal.7z")
# Find the actual toolchain eg # toolchain_windows-x86_64_arm-zephyr-eabi.7z
# Paste here:
TOOLCHAIN_URL = 'https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.1-rc1/toolchain_windows-x86_64_arm-zephyr-eabi.7z'

#Look down the page in Assets, open file sha256.sum, find the hash for the files above, paste here:
SDK_MIN_HASH = 'cf66c9182468e6b25a918c19d134a6441ffe93f9cd2889890c2b3daf0b49157e'
TOOLCHAIN_HASH = '0d6da84628b86feb9166c6bbf7a50d8ffa9bb9497c37c937f50181b1664e28dc'

# Update the gcc path
gcc_path = 'zephyr-sdk-0.17.1-rc1/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc'

class WindowsSetupToolchain(WestCommand):

    def __init__(self):
        super().__init__(
            'windows-setup-toolchain',               # gets stored as self.name
            'Setup the toolchain',  # self.help
            # self.description:
            '''\
            This installs a specific version of the toolchain to build the project.''')
        self.toolchain_dir = 'toolchain'
        self.sdk_dir = 'sdk'

    def do_add_parser(self, parser_adder):
        # This is a bit of boilerplate, which allows you full control over the
        # type of argparse handling you want. The "parser_adder" argument is
        # the return value of an argparse.ArgumentParser.add_subparsers() call.
        parser = parser_adder.add_parser(self.name,
                                         help=self.help,
                                         description=self.description)

        # Add some example options using the standard argparse module API.
        parser.add_argument('-o', '--optional', help='an optional argument')
 #       parser.add_argument('required', help='a required argument')

        return parser           # gets stored as self.parser


    def _check_hash(self, filename, hash):
        log.inf("Checking Hash")
        hasher = FileHash('sha256')
        file_hash = hasher.hash_file(filename)
        if not file_hash == hash:
            log.err("Downloaded file does not match expected hash. Stopping")
            raise ValueError

    def download_toolchain(self):
        mkdir(self.sdk_dir)
        chdir(self.sdk_dir)

        log.inf("Downloading minimal SDK")
        name = wget.download(SDK_MIN_URL)
        log.inf("") #wget does not produce a newline on completion
        self._check_hash(name, SDK_MIN_HASH)

        log.inf("Extracting minimal SDK")
        with py7zr.SevenZipFile(name, mode='r') as archive:
            archive.extractall(path='.')
        remove(name)

        sdk_name = '.'.join(name.split('_')[:-3])
        chdir(sdk_name)

        log.inf("Downloading toolchain")
        name = wget.download(TOOLCHAIN_URL)
        log.inf("") #wget does not produce a newline on completion
        self._check_hash(name, TOOLCHAIN_HASH)

        log.inf("Extracting toolchain")
        with py7zr.SevenZipFile(name, mode='r') as archive:
            archive.extractall(path='.')
        remove(name)
        chdir('..')


    def do_run(self, args, unknown_args):
        current_platform = platform.system()
        assert current_platform == 'Windows'

        if (isdir(self.toolchain_dir)):
            log.inf("Toolchain dir already present {}".format(self.toolchain_dir))
            return

        mkdir(self.toolchain_dir)
        chdir(self.toolchain_dir)
        self.download_toolchain()
        chdir('..')

if __name__ == "__main__":
    s = WindowsSetupToolchain()
    s.do_run(None, None)