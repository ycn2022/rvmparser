#!/bin/bash

# Test build script for rvmparser-linux
# This script tests the CMake build process

echo "=== Testing rvmparser-linux CMake build ==="

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed or not in PATH"
    exit 1
fi

# Check if make is available
if ! command -v make &> /dev/null; then
    echo "Error: make is not installed or not in PATH"
    exit 1
fi

# Check if required source files exist
if [ ! -f "src/main.cpp" ]; then
    echo "Error: main.cpp not found in src directory"
    exit 1
fi

if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found"
    exit 1
fi

# Check if dependencies exist
if [ ! -d "libs/rapidjson" ]; then
    echo "Error: rapidjson library not found"
    exit 1
fi

if [ ! -d "libs/libtess2" ]; then
    echo "Error: libtess2 library not found"
    exit 1
fi

echo "All prerequisites check passed!"
echo "Ready to build with: ./build.sh"
echo ""
echo "Manual build commands:"
echo "  mkdir build && cd build"
echo "  cmake .. -DCMAKE_BUILD_TYPE=Release"
echo "  make -j\$(nproc)"