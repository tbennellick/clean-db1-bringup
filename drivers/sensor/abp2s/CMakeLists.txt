zephyr_library()
file(GLOB SRC_FILES *.c)
zephyr_library_sources(${SRC_FILES})
