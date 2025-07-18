#!/usr/bin/env python3
import os
import subprocess
import sys
import argparse
from pathlib import Path

# Add the parent directory to sys.path to allow importing from scripts
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from scripts.setup_env import setup_virtual_environment

def build_specific_board_variant(board_spec, pristine=False):
    build_cmd = ["west", "build", "-b", board_spec, "app/"]
    if pristine:
        build_cmd.append("-p")
    print(f"Building {board_spec}...")
    subprocess.run(build_cmd, check=True)
    print(f"Successfully built {board_spec}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Build firmware for db1 board')
    parser.add_argument('--revision', '-r', default='62.2.1', 
                        help='Board revision to build (default: 62.2.1)')
    parser.add_argument('--pristine', '-p', action='store_true',
                        help='Do pristine build')
    parser.add_argument('--flash', '-f', action='store_true',
                        help='Flash after build')
    
    args = parser.parse_args()
    
    # Setup the build environment
    project_dir = Path(__file__).parent.parent.absolute()
    venv_dir = project_dir / ".venv"
    setup_virtual_environment(venv_dir)

    # Build the specified revision
    board_spec = f"db1@{args.revision}/mcxn947/cpu0"
    build_specific_board_variant(board_spec, args.pristine)

    if args.flash:
        subprocess.run(["west", "flash", "-d", f"build/{board_spec}/app", "--runner=jlink"], check=True)
        #TODO, add debugger select with --snr=50000364
