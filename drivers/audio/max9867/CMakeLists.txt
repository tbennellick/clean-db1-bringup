zephyr_library()
zephyr_library_sources_ifdef(CONFIG_AUDIO_CODEC_MAX9867 max9867.c max9867_utils.c)
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../include/app/drivers)
# zephyr_library_sources_ifdef(CONFIG_AUDIO_CODEC_MAX9867 )