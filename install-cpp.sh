#!/bin/bash

# Installation script for Conduit C++ Library

set -e

echo "=== Installing Conduit C++ Library ==="

# Check if we're in the right directory
if [ ! -f "CMakeLists-cpp.txt" ]; then
    echo "Error: CMakeLists-cpp.txt not found. Please run this script from the project root."
    exit 1
fi

# Check C++ compiler
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "Error: C++ compiler not found. Please install g++ or clang++."
    exit 1
fi

# Backup existing build directory if it exists
if [ -d "build-cpp" ]; then
    echo "Backing up existing build directory..."
    rm -rf build-cpp.backup
    mv build-cpp build-cpp.backup
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build-cpp
cd build-cpp

# Configure with CMake
echo "Configuring with CMake..."
cp ../CMakeLists-cpp.txt ../CMakeLists.txt.backup
cp ../CMakeLists-cpp.txt ../CMakeLists.txt
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_INSTALL_PREFIX=/usr/local

# Build
echo "Building..."
make -j$(nproc)

# Run tests if they exist
if [ -f "test_basic_cpp" ]; then
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

# Restore original CMakeLists.txt
cd ..
if [ -f "CMakeLists.txt.backup" ]; then
    mv CMakeLists.txt.backup CMakeLists.txt
fi
cd build-cpp

echo ""
echo "=== Installation Complete ==="
echo ""
echo "The Conduit C++ library has been installed successfully!"
echo ""
echo "Usage:"
echo "  #include <conduit.hpp>"
echo ""
echo "Compile with:"
echo "  g++ -std=c++17 -o your_app your_app.cpp -lconduit-cpp"
echo ""
echo "Or use pkg-config:"
echo "  g++ -std=c++17 -o your_app your_app.cpp \$(pkg-config --cflags --libs conduit-cpp)"
echo ""
echo "Or with CMake:"
echo "  find_package(conduit-cpp REQUIRED)"
echo "  target_link_libraries(your_target PRIVATE ConduitCpp::conduit-cpp)"
echo ""
echo "Examples are available in the examples/ directory."
