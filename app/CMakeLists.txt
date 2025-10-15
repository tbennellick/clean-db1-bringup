cmake_minimum_required(VERSION 3.13.1)

if(NOT DEFINED BOARD)
    set(BOARD db1/mcxn947/cpu0) # Must be before find_package()
    #set(BOARD frdm_mcxn947/mcxn947/cpu0) # Must be before find_package()
endif()
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/debug_leds.c)
target_sources(app PRIVATE src/handle_fatal.c)
target_sources(app PRIVATE src/led_manager.c)
target_sources(app PRIVATE src/modem.c)
target_sources(app PRIVATE src/power.c)
target_sources(app PRIVATE src/imu.c)
target_sources(app PRIVATE src/rip.c)
target_sources(app PRIVATE src/pressure.c)
#target_sources(app PRIVATE src/exg.c)
target_sources(app PRIVATE src/gpio_init.c)
target_sources(app PRIVATE src/fuel_gauge.c)
target_sources(app PRIVATE src/als.c)
target_sources(app PRIVATE src/audio.c)
target_sources(app PRIVATE src/display.c)
target_sources(app PRIVATE src/temperature.c)
target_sources(app PRIVATE src/storage.c)
target_sources(app PRIVATE src/uuid_lib.c)
target_sources(app PRIVATE src/boot_id.c)
target_sources(app PRIVATE src/ui.c)
target_sources(app PRIVATE src/usb.c)

#TODO This needs to be replaced with stowood specific info
include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)


#Logo can be optionally removed from the build.
zephyr_sources_ifdef(CONFIG_DISPLAY_LOGO ../artefacts/test_logo.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../artefacts)

# This exposes the audio codec routing enum to the app,
# it seems that there is not a nice way to handle this.
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../drivers/audio/max9867)
