#!/bin/bash

# Installation script for Conduit C Library

set -e

echo "=== Installing Conduit C Library ==="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run this script from the project root."
    exit 1
fi

# Backup existing build directory if it exists
if [ -d "build" ]; then
    echo "Backing up existing build directory..."
    rm -rf build.backup
    mv build build.backup
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_STANDARD=99 \
    -DCMAKE_INSTALL_PREFIX=/usr/local

# Build
echo "Building..."
make -j$(nproc)

# Run tests if they exist
if [ -f "test" ]; then
    echo "Running tests..."
    make test
fi

# Install
echo "Installing to system..."
sudo make install

# Update library cache on Linux
if command -v ldconfig &> /dev/null; then
    echo "Updating library cache..."
    sudo ldconfig
fi

echo ""
echo "=== Installation Complete ==="
echo ""
echo "The Conduit C library has been installed successfully!"
echo ""
echo "Usage:"
echo "  #include <conduit.h>"
echo ""
echo "Compile with:"
echo "  gcc -o your_app your_app.c -lconduit"
echo ""
echo "Or use pkg-config:"
echo "  gcc -o your_app your_app.c \$(pkg-config --cflags --libs conduit)"
echo ""
echo "Examples are available in the examples/ directory."
