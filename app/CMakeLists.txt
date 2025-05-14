cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/debug_leds.c)
target_sources(app PRIVATE src/handle_fatal.c)
target_sources(app PRIVATE src/led_manager.c)
target_sources(app PRIVATE src/power.c)
target_sources(app PRIVATE src/imu.c)
target_sources(app PRIVATE src/rip.c)
target_sources(app PRIVATE src/pressure.c)
