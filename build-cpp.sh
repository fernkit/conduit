#!/bin/bash

# Build script for Conduit C++ library

set -e

echo "=== Building Conduit C++ Library ==="

# Create build directory
mkdir -p build-cpp
cd build-cpp

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -f ../CMakeLists-cpp.txt \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_INSTALL_PREFIX=/usr/local

# Build
echo "Building..."
make -j$(nproc)

# Run tests if they exist
if [ -f "test" ]; then
    echo "Running tests..."
    make test
fi

echo "Build completed successfully!"
echo ""
echo "To install system-wide, run:"
echo "  sudo make install"
echo ""
echo "To run examples:"
echo "  ./simple_example_cpp"
echo "  ./json_example_cpp"
echo ""
echo "To use in your project:"
echo "  find_package(conduit-cpp REQUIRED)"
echo "  target_link_libraries(your_target PRIVATE ConduitCpp::conduit-cpp)"
