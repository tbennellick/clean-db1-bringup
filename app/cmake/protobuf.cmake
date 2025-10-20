# Protobuf (nanopb) configuration

include(${ZEPHYR_NANOPB_MODULE_DIR}/cmake/nanopb.cmake)

# Use protobuf definitions from bfp_common_sw module
set(BFP_COMMON_SW_DIR ${ZEPHYR_BASE}/../bfp_common_sw)
zephyr_nanopb_sources(app ${BFP_COMMON_SW_DIR}/bfp_proto/BFP.proto)

# Add proto directory to include paths for the proto revision header
target_include_directories(app PRIVATE ${BFP_COMMON_SW_DIR}/bfp_proto)