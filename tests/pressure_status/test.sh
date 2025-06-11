#!/bin/bash
export ZEPHYR_SDK_INSTALL_DIR=../../../toolchain/tc/
#west build -b frdm_mcxn947/mcxn947/cpu0
west build -b db1/mcxn947/cpu0
west flash --runner=jlink

