# Bash wrapper for Python environment setup
# Usage: source setup_env.sh

# Ensure script is sourced
if ! $(return 0 >/dev/null 2>&1); then
    echo "Error: Script must be sourced. Run: source $0"
    exit 1
fi

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
VENV_DIR=$(readlink -f "${SCRIPT_DIR}/../.venv")

echo "Setting up development environment..."

# Run the Python setup script
echo "Running Python environment setup..."
if ! python3 "${SCRIPT_DIR}/setup_env.py"; then
    echo "Error: Python setup failed"
    return 1
fi

# Manually activate the virtual environment (since Python can't do this for us)
if [[ -d "${VENV_DIR}" ]]; then
    # Check if we're already in the correct virtual environment
    if [[ -n "${VIRTUAL_ENV}" ]]; then
        CURRENT_VENV="$(readlink -f "${VIRTUAL_ENV}")"
        if [[ "${CURRENT_VENV}" != "${VENV_DIR}" ]]; then
            echo "Error: Already in a different virtual environment"
            echo "Current: ${CURRENT_VENV}"
            echo "Expected: ${VENV_DIR}"
            echo "Please deactivate first and try again"
            return 1
        fi
        echo "Already in correct virtual environment"
    else
        echo "Activating virtual environment..."
        source "${VENV_DIR}/bin/activate"
    fi
else
    echo "Error: Virtual environment directory not found: ${VENV_DIR}"
    return 1
fi

# Source Zephyr environment if it exists
ZEPHYR_ENV_SCRIPT=$(readlink -f "${SCRIPT_DIR}/../../zephyr/zephyr-env.sh")
if [[ -f "${ZEPHYR_ENV_SCRIPT}" ]]; then
    echo "Sourcing Zephyr environment..."
    source "${ZEPHYR_ENV_SCRIPT}"
else
    echo "Warning: Zephyr environment script not found"
fi

# Set Zephyr environment variables
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=$(readlink -f "${SCRIPT_DIR}/../../toolchain/tc/")

# Final status
echo ""
echo "=== Environment Setup Complete ==="
echo "Virtual Environment: ${VIRTUAL_ENV:-"Not set"}"
echo "Zephyr Toolchain: ${ZEPHYR_TOOLCHAIN_VARIANT:-"Not set"}"
echo "SDK Install Dir: ${ZEPHYR_SDK_INSTALL_DIR:-"Not set"}"
echo ""
echo "Development environment is ready!"
