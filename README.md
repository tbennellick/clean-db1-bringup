# BFP2 Dev board 1 bring up firmware

This is a Zephyr "Workspace"[^1] project based on [example-application]. It is forked from a tag of a point release to ensure reliable builds. ( revision: v4.1.0 in west.yml)

[example-application]: https://github.com/zephyrproject-rtos/example-application
[^1]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-workspace-app

## Building and running the application
This project is intended to be built on a modern linux system. Some support has been added for windows [here](##Building and running on Windows). 

# Local Build
## Install prerequisites
* Fedora:
```shell
sudo dnf install -y wget2-2.1.0 wget2-wget-2.1.0 git-2.46.0 cmake-3.28.2 ninja-build-1.12.1 xz-5.4.6 python3-devel-3.12.4 perl-Digest-SHA-1:6.04 protobuf-compiler-3.19.6
```
* Ubuntu:
```shell
sudo apt install -y wget2=2.2.0+ds-1 wget=1.24.5-2ubuntu1 git=1:2.48.1-0ubuntu1 cmake=3.31.6-1ubuntu1 ninja-build=1.12.1-1 xz-utils=5.6.4-1ubuntu1 python3.12-dev=3.12.7-1ubuntu2 perl=5.40.1-2ubuntu0.1 libdigest-sha-perl=6.04-1build4 protobuf-compiler=3.21.12-10build2
```

## Get source, dependencies and toolchain

Note that the repository is not cloned directly. Instead, the zephyr tool west is used to manage dependencies.
```shell
git clone git@github.com:Stowood/devboard1-bringup-zephyr.git db1-bringup/devboard1-bringup-zephyr
cd db1-bringup/devboard1-bringup-zephyr
source scripts/setup_env.sh
```

### Building and running

To build, run the following command:

```shell
cd devboard1-bringup-zephyr.git/
scripts/build.py # or west build app/
```

To flash on the device:
```shell
scripts/build.py -f
```

--------------------------------------------------------

### Toolchain
The `setup-toolchain` command used above is a custom west extension that downloads and verifies the exact version of the toolchain.

The toolchain is passed to the build by exporting ZEPHYR_SDK_INSTALL_DIR in the build.sh script

If the correct version of the SDK is present in  /opt/zephyr-sdk/ the script will link to that instead of downloading it.

### More advanced building
The standard west commands are available as with any other zephyr/ncs build. Including:
```shell
# To use some of these, you will need need to export ZEPHYR_SDK_INSTALL_DIR eg:
export ZEPHYR_SDK_INSTALL_DIR=../../toolchain/tc/
# or 
export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk/zephyr-sdk-0.17.1-rc1/
west build -b frdm_mcxn947/mcxn947/cpu0 samples/usb-test
west flash --runner=jlink -d build/frdm_mcxn947/mcxn947/cpu0/samples/usb-test

west boards
west debug
west build -t menuconfig
```


## Building and running on Windows

### Prerequisites
There are some prerequisites for building on Windows. They were not captured during initial development.
If this is ever used again. They should be captured the next time this is built on a clean Windows machine. 

It is probably just to install python.

```bash
mkdir db1-windows-bringup
cd db1-windows-bringup
git clone git@github.com:Stowood/devboard1-bringup-zephyr.git
python -m venv .venv
.venv\Scripts\Activate.ps1
pip install -r devboard1-bringup-zephyr/scripts/requirements.txt
cd devboard1-bringup-zephyr
west init -l .
west update
west windows-setup-toolchain
python scripts\build.py
# To flash:
python scripts\build.py -f

```

### Testing TBC

```shell
west twister -T tests --integration
```

## Code Formatting and Linting

This project uses `clang-format` to ensure consistent code formatting, using the [run-clang-format.py](https://github.com/Sarcasm/run-clang-format/) wrapper script. The formatting of staged files is automatically checked on each commit via a pre-commit hook, installed when sourcing the `setup_env.sh` script.

### Running Linting Manually
* Check formatting of staged files without applying changes:
	```shell
	scripts/lint.py --check
	```
* Check formatting on all files (not just staged):
	```shell
	scripts/lint.py --check --all
	```
* Fix formatting of staged files:
	```shell
	scripts/lint.py --apply
	```
* Fix formatting on all files:
	```shell
	scripts/lint.py --apply --all
	```

### Skipping the Pre-commit Check
If you need to commit code without running the formatting check, you can skip the pre-commit hook using:

```shell
git commit --no-verify
```
**Note:** This should be used sparingly. It's better to fix formatting issues before committing.
