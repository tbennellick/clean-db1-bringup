#!/usr/bin/env python3
import os, subprocess, sys, platform

if __name__ == "__main__":
    tc_path = "../../toolchain/tc/" if platform.system() != "Windows" else "../toolchain/sdk/"
    os.environ["ZEPHYR_SDK_INSTALL_DIR"] = tc_path

    build_cmd = ["west", "build", "-b", "db1/mcxn947/cpu0"]
    if any(arg in ["-p", "--pristine"] for arg in sys.argv[1:]):
        build_cmd.append("-p")
    subprocess.run(build_cmd, check=True)


    if len(sys.argv) > 1 and any(arg in ["flash", "-f"] for arg in sys.argv[1:]):
        subprocess.run(["west", "flash", "--runner=jlink"], check=True)
        #TODO, add debugger select with --snr=50000364
