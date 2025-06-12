#!/bin/bash

export ZEPHYR_SDK_INSTALL_DIR=../../../toolchain/tc/

if [ "$1" == "sim" ]; then
  west build -b qemu_cortex_m3
  west build -t run

elif [ "$1" == "dvk" ]; then
  west build -b frdm_mcxn947/mcxn947/cpu0
  west flash --runner=jlink
else
  west build -b db1/mcxn947/cpu0
  west flash --runner=jlink
fi


