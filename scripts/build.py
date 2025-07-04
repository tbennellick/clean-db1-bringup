#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

# Add the parent directory to sys.path to allow importing from scripts
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from scripts.setup_env import setup_virtual_environment

if __name__ == "__main__":
    # Setup the build environment
    project_dir = Path(__file__).parent.parent.absolute()
    venv_dir = project_dir / ".venv"
    setup_virtual_environment(venv_dir)

    build_cmd = ["west", "build", "-b", "db1/mcxn947/cpu0", "app/"]
    if any(arg in ["-p", "--pristine"] for arg in sys.argv[1:]):
        build_cmd.append("-p")
    subprocess.run(build_cmd, check=True)


    if len(sys.argv) > 1 and any(arg in ["flash", "-f"] for arg in sys.argv[1:]):
        subprocess.run(["west", "flash", "-d", "build/db1/mcxn947/cpu0/app", "--runner=jlink"], check=True)
        #TODO, add debugger select with --snr=50000364
