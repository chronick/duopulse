#!/bin/bash
# Setup script for DaisySP Patch.Init firmware project

set -e

echo "Setting up DaisySP Patch.Init firmware project..."

# Check for git
if ! command -v git &> /dev/null; then
    echo "Error: git is not installed"
    exit 1
fi

# Initialize and update git submodules
echo "Initializing DaisySP submodule..."
git submodule update --init --recursive

# Check if DaisySP directory exists
if [ ! -d "DaisySP" ]; then
    echo "Error: DaisySP submodule not found"
    exit 1
fi

# Build DaisySP library
echo "Building DaisySP library..."
cd DaisySP
if [ ! -f "Makefile" ]; then
    echo "Error: DaisySP Makefile not found"
    exit 1
fi
make
cd ..

# Check for ARM toolchain
echo "Checking for ARM toolchain..."
if ! command -v arm-none-eabi-gcc &> /dev/null; then
    echo "Warning: ARM GCC toolchain not found in PATH"
    echo "Please install ARM GCC toolchain for STM32 development"
    echo "On macOS: brew install arm-none-eabi-gcc"
    echo "On Linux: sudo apt-get install gcc-arm-none-eabi"
fi

# Check for dfu-util (for flashing)
if ! command -v dfu-util &> /dev/null; then
    echo "Warning: dfu-util not found"
    echo "Install dfu-util for firmware deployment:"
    echo "On macOS: brew install dfu-util"
    echo "On Linux: sudo apt-get install dfu-util"
fi

echo ""
echo "Setup complete!"
echo ""
echo "Next steps:"
echo "  1. Review src/main.cpp and customize your firmware"
echo "  2. Run 'make' to build the firmware"
echo "  3. Run 'make test' to run unit tests (requires Catch2)"
echo "  4. Run 'make program' to flash firmware (requires DFU mode)"
echo ""

