# Protobuf (nanopb) configuration

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
zephyr_nanopb_sources(app proto/BFP.proto)

# Add proto directory to include paths for the proto revision header
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/proto)