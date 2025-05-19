#!/bin/bash

# Exit on any error
set -e

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
# Uncomment to run tests if you have them
# make test
sudo make install
sudo ldconfig