#!/usr/bin/env python3
import os
import subprocess
import sys

# Add the parent directory to sys.path to allow importing from scripts
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from scripts.setup_env import main as setup_main

if __name__ == "__main__":
    # Setup the build environment
    setup_main()

    build_cmd = ["west", "build", "-b", "db1/mcxn947/cpu0", "app/"]
    if any(arg in ["-p", "--pristine"] for arg in sys.argv[1:]):
        build_cmd.append("-p")
    subprocess.run(build_cmd, check=True)


    if len(sys.argv) > 1 and any(arg in ["flash", "-f"] for arg in sys.argv[1:]):
        subprocess.run(["west", "flash", "--runner=jlink"], check=True)
        #TODO, add debugger select with --snr=50000364
