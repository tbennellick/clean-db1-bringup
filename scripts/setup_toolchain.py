from west.commands import WestCommand  # your extension must subclass this
from west import log  # use this for user output

import wget
from filehash import FileHash
from os.path import isdir, isfile
from os import mkdir, chdir, symlink, remove
import tarfile
from pathlib import Path
import tempfile

# This command is doing a very similar thing to the zephyr setup.sh in the SDK bundles. This adds hash checking.

## How to update:
## go here: https://github.com/zephyrproject-rtos/sdk-ng/tags
## select the version.
# Go to downloads
# Copy the link for SDK Bundle > minimal > Linux/x86-64
# Paste here:
SDK_MIN_URL = "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.1-rc1/zephyr-sdk-0.17.1-rc1_linux-x86_64_minimal.tar.xz"
# Find the actual toolchain eg # toolchain_linux-x86_64_arm-zephyr-eabi.tar.xz
# Paste here:
TOOLCHAIN_URL = "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.1-rc1/toolchain_linux-x86_64_arm-zephyr-eabi.tar.xz"

# Look down the page in Assets, open file sha256.sum, find the hash for the files above, paste here:
SDK_MIN_HASH = "bd9c7c916a37e8e305a7203d7cce90d7acfa0d7d798c7244c70a6dbc04949b1d"
TOOLCHAIN_HASH = "99af1bdcbc01fd451b180d0a546a773cfec6a5ea797caab64c4316717f8ccaa4"

# Update the gcc path
gcc_path = "zephyr-sdk-0.17.1-rc1/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc"


class SetupToolchain(WestCommand):
    def __init__(self):
        super().__init__(
            "setup-toolchain",  # gets stored as self.name
            "Setup the toolchain",  # self.help
            # self.description:
            """\
            This installs a specific version of the toolchain to build the project.""",
        )
        self.toolchain_dir = Path(__file__).parent.parent.parent / "toolchain"
        self.sdk_dir = self.toolchain_dir / "sdk"

    def do_add_parser(self, parser_adder):
        # This is a bit of boilerplate, which allows you full control over the
        # type of argparse handling you want. The "parser_adder" argument is
        # the return value of an argparse.ArgumentParser.add_subparsers() call.
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        # Add some example options using the standard argparse module API.
        parser.add_argument("-o", "--optional", help="an optional argument")
        #       parser.add_argument('required', help='a required argument')

        return parser  # gets stored as self.parser

    def check_toolchain(self):
        pass

    def _check_hash(self, filename, hash):
        log.inf("Checking Hash")
        hasher = FileHash("sha256")
        file_hash = hasher.hash_file(filename)
        if not file_hash == hash:
            log.err("Downloaded file does not match expected hash. Stopping")
            raise ValueError

    def download_toolchain(self):
        mkdir(self.sdk_dir)
        chdir(self.sdk_dir)
        log.inf("Downloading minimal SDK")
        name = wget.download(SDK_MIN_URL, out=tempfile.gettempdir())
        sdk_name = ".".join(Path(name).stem.split("_")[:-3])
        log.inf("")  # wget does not produce a newline on completion
        self._check_hash(name, SDK_MIN_HASH)

        log.inf(f"Extracting minimal SDK {sdk_name}")
        with tarfile.open(name) as f:
            f.extractall(".")
        remove(name)

        chdir(sdk_name)

        log.inf("Downloading toolchain")
        name = wget.download(TOOLCHAIN_URL, out=tempfile.gettempdir())
        log.inf("")  # wget does not produce a newline on completion
        self._check_hash(name, TOOLCHAIN_HASH)

        log.inf("Extracting toolchain")
        with tarfile.open(name) as f:
            f.extractall(".")
        remove(name)
        chdir("..")

    def do_run(self, args, unknown_args):
        # This gets called when the user runs the command

        ci_sdk_path = "/opt/zephyr-sdk/"
        # check if sdk already present eg in CI container
        if isfile(ci_sdk_path + gcc_path) and not isdir(
            self.toolchain_dir
        ):  # And toolchain isnt already setup
            log.inf("Linking to toolchain found in /opt/")
            mkdir(self.toolchain_dir)
            chdir(self.toolchain_dir)
            symlink(ci_sdk_path, "tc")
            return

        if isdir(self.toolchain_dir):
            log.inf("Toolchain already setup")
            self.check_toolchain()
            return
        else:
            mkdir(self.toolchain_dir)
            chdir(self.toolchain_dir)
            self.download_toolchain()
            chdir("..")
            symlink(self.sdk_dir, "tc")
