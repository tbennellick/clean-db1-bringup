#!/usr/bin/env python3
"""
Environment setup script for Zephyr development
"""

import configparser
import os
from pathlib import Path
import platform
import subprocess


def setup_virtual_environment(venv_dir: Path) -> bool:
    if platform.system() == "Windows":
        venv_bin = venv_dir / "Scripts"
        path_separator = ";"
    else:
        venv_bin = venv_dir / "bin"
        path_separator = ":"

    # Check if we're already in a virtual environment
    current_venv = os.environ.get("VIRTUAL_ENV")
    if current_venv:
        current_venv_path = Path(current_venv).resolve()
        target_venv_path = venv_dir.resolve()

        if current_venv_path == target_venv_path:
            print("Already in the correct virtual environment")
            return True
        else:
            print("Error: Already in a different virtual environment:")
            print(f"Current: {current_venv_path}")
            print(f"Expected: {target_venv_path}")
            print("Please deactivate the current virtual environment first")
            return False

    if not venv_bin.exists():
        print(f"Initialising venv at: {venv_dir}")
        subprocess.run(f"python -m venv {venv_dir} --prompt db1-bringup", shell=True, check=True)

    # set venv environment vars so the rest of the script can use it
    if not current_venv:
        print(f"Activating venv at: {venv_dir}")
        os.environ["VIRTUAL_ENV"] = str(venv_dir)
        os.environ["PATH"] = f"{venv_bin}{path_separator}{os.environ.get('PATH', '')}"

    return True


def install_requirements(requirements_file: Path):
    print(f"Installing requirements from {requirements_file}")
    subprocess.run(f"pip install -r {requirements_file}", shell=True, check=True)


def setup_west(project_dir: Path) -> bool:
    print("Setting up west and zephyr sdk/toolchain...")

    west_config_path = project_dir / ".." / ".west" / "config"
    if not west_config_path.exists():
        subprocess.run("west init -l .", shell=True, check=True)

        config = configparser.ConfigParser()
        config.read(west_config_path)

        if "build" not in config:
            config["build"] = {}
        config["build"]["dir-fmt"] = "build/{board}/{source_dir}"
        config["build"]["board"] = "db1/mcxn947/cpu0"

        with open(west_config_path, "w") as configfile:
            config.write(configfile)

        print("Updated .west/config with build directory and board configuration")

    subprocess.run("west update --fetch-opt=--filter=tree:0", shell=True, check=True)
    subprocess.run("west setup-toolchain", shell=True, check=True)


def git_commit_hook(project_dir: Path):
    hook_path = project_dir / ".git" / "hooks" / "pre-commit"

    if not hook_path.exists():
        print("Installing pre-commit hook...")
        hook_path.symlink_to(project_dir / "scripts" / "lint.py")


def main():
    project_dir = Path(__file__).parent.parent.absolute()
    venv_dir = project_dir / ".venv"

    if not setup_virtual_environment(venv_dir):
        exit(1)

    requirements_file = project_dir / "scripts" / "requirements.txt"
    install_requirements(requirements_file)

    setup_west(project_dir)

    git_commit_hook(project_dir)

    print("Environment setup completed successfully!")


if __name__ == "__main__":
    main()
