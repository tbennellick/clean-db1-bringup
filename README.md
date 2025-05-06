# BFP2 Dev board 1 bring up firmware

This is a Zephyr "Workspace"[^1] project based on [example-application]. It is forked from a tag of a point release to ensure reliable builds. ( revision: v4.1.0 in west.yml)

[example-application]: https://github.com/zephyrproject-rtos/example-application
[^1]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-workspace-app

## Building and running the application
This project is intended to be built on a modern linux system. It will probably work on other host OS but that is not considered here. 

# Local Build
## Install prerequisites
```shell
sudo dnf install -y wget2-2.1.0 wget2-wget-2.1.0 git-2.46.0 cmake-3.28.2 ninja-build-1.12.1 xz-5.4.6 python3-devel-3.12.4 perl-Digest-SHA-1:6.04 protobuf-compiler-3.19.6
pip install --no-cache-dir wget==3.2 filehash==0.2.dev1 west==1.2.0 pyelftools==0.30 grpcio-tools==1.62.0 protobuf==4.25.3
# Later versions will probably work fine. These are the versions used in CI build.
```

## Get source, dependencies and toolchain

Note that the repository is not cloned directly. Instead, the zephyr tool west is used to manage dependencies.
```shell
west init -m ggit@github.com:Stowood/devboard1-bringup-zephyr.git --mr main db1-bringup
cd db1-bringup
west update
west setup-toolchain
```

### Building and running

To build, run the following command:

```shell
cd db1-bringup.git/
cd app/
./build.sh
./flash.sh
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
west build -b frdm_mcxn947/mcxn947/cpu0
west flash --runner=jlink

west boards
west debug
west build -t menuconfig
```
### Testing TBC

```shell
west twister -T tests --integration
```
