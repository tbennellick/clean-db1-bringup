#!/bin/bash
export ZEPHYR_SDK_INSTALL_DIR=../../toolchain/tc/
west flash --runner=jlink
#--snr=50000364
