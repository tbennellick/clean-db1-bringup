#!/bin/bash

export ZEPHYR_SDK_INSTALL_DIR=../../../toolchain/tc/

west build -b db1/mcxn947/cpu0
west -v flash --runner=jlink


