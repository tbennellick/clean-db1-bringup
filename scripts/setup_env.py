#!/usr/bin/env python3
"""
Environment setup script for Zephyr development
"""

import configparser
import os
from pathlib import Path
import subprocess


def run_command(cmd) -> bool:
    """Run a command and handle errors"""
    print(f"Running command: {cmd}")

    try:
        subprocess.run(cmd, shell=True, check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {cmd}: {e}")
        return False


def setup_virtual_environment(venv_dir: Path):
    """Setup and activate virtual environment"""
    venv_bin = venv_dir / "bin"

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

    # Check if venv directory exists
    if not venv_bin.exists():
        print(f"Initialising venv at: {venv_dir}")
        if not run_command(f"python -m venv {venv_dir} --prompt db1-bringup"):
            print("Failed to create virtual environment")
            return False

    # # Activate virtual environment (set environment variables)
    if not current_venv:
        print(f"Activating venv at: {venv_dir}")
        os.environ["VIRTUAL_ENV"] = str(venv_dir)
        os.environ["PATH"] = f"{venv_bin}:{os.environ.get('PATH', '')}"

    return True


def install_requirements(requirements_file: Path):
    """Install Python requirements"""
    if not requirements_file.exists():
        print(f"Requirements file not found: {requirements_file}")
        return False

    print(f"Installing requirements from {requirements_file}")
    return run_command(f"pip install -r {requirements_file}")


def setup_west(project_dir: Path):
    print("Setting up west and zephyr sdk/toolchain...")

    west_config_path = project_dir / ".." / ".west" / "config"
    if not west_config_path.exists():
        if not run_command("west init -l ."):
            print("Failed to run west init")
            return False

        config = configparser.ConfigParser()
        config.read(west_config_path)

        # Add build directory configuration
        if "build" not in config:
            config["build"] = {}
        config["build"]["dir-fmt"] = "build/{board}/{source_dir}"
        config["build"]["board"] = "db1/mcxn947/cpu0"

        # Write the updated configuration back to file
        with open(west_config_path, "w") as configfile:
            config.write(configfile)

        print("Updated .west/config with build directory and board configuration")

    # West update
    if not run_command("west update --fetch-opt=--filter=tree:0"):
        print("Failed to run west update")
        return False

    # West setup toolchain
    if not run_command("west setup-toolchain"):
        print("Failed to setup west toolchain")
        return False

    return True


def main():
    """Main function"""
    # Get script directory and venv directory
    project_dir = Path(__file__).parent.parent.absolute()
    venv_dir = project_dir / ".venv"

    # Setup virtual environment
    if not setup_virtual_environment(venv_dir):
        exit(1)

    # Install main requirements
    requirements_file = project_dir / "requirements.txt"

    if not install_requirements(requirements_file):
        print("Failed to install requirements")
        exit(1)

    # Setup west toolchain
    if not setup_west(project_dir):
        exit(1)

    print("Environment setup completed successfully!")


if __name__ == "__main__":
    main()
