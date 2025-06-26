cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(codec)

target_include_directories(app PRIVATE ../../drivers/audio/max9867/)
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE ../../drivers/audio/max9867/max9867_utils.c)
#target_sources(app PRIVATE src/codec_integration.c)
