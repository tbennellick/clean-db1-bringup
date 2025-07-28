# Claude Instructions for DB1 Bringup Project

## Project Context
This is a Zephyr RTOS project for the DB1 development board based on the MCXN947 microcontroller.

## Build Instructions
- To build: `source scripts/setup_env.sh && scripts/build.py -p`
- Use the `-p` flag for pristine builds when needed
- Build script handles environment setup automatically

## Project Structure
- Main application code in `app/src/`
- Board definition in `boards/stowood/db1/`
- Device tree: `db1_mcxn947_cpu0.dts`
- Configuration: `app/prj.conf`

## Coding Standards
- Follow existing project conventions
- Use Zephyr logging (LOG_INF, LOG_ERR, etc.)
- Include proper error handling
- Keep comments concise and relevant
- Use #pragma once for header files

## Hardware
- The board scehematic is available in `/home/tom/project/stowood/client-hardware/dev-board1/board/'
