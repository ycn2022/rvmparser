#!/bin/bash

# Cross-platform build script for rvmparser

set -e  # Exit on any error

# Detect platform
PLATFORM=$(uname -s)
case $PLATFORM in
    Linux*)     PLATFORM_NAME="Linux";;
    Darwin*)    PLATFORM_NAME="macOS";;
    CYGWIN*)    PLATFORM_NAME="Windows (Cygwin)";;
    MINGW*)     PLATFORM_NAME="Windows (MinGW)";;
    *)          PLATFORM_NAME="Unknown";;
esac

echo "=== Building rvmparser for $PLATFORM_NAME ==="

# Create build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
if [[ "$PLATFORM" == MINGW* ]] || [[ "$PLATFORM" == CYGWIN* ]]; then
    # Windows with MinGW/Cygwin
    cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
else
    # Unix-like systems
    cmake .. -DCMAKE_BUILD_TYPE=Release
fi

# Build
echo "Building..."
if command -v nproc >/dev/null 2>&1; then
    JOBS=$(nproc)
elif command -v sysctl >/dev/null 2>&1; then
    JOBS=$(sysctl -n hw.ncpu)
else
    JOBS=4
fi

if [[ "$PLATFORM" == MINGW* ]] || [[ "$PLATFORM" == CYGWIN* ]]; then
    cmake --build . --config Release -j $JOBS
else
    make -j$JOBS
fi

echo "=== Build completed successfully ==="
echo "Executable location: $BUILD_DIR/rvmparser"

# Test if executable was created
EXECUTABLE="rvmparser"
if [[ "$PLATFORM" == MINGW* ]] || [[ "$PLATFORM" == CYGWIN* ]]; then
    EXECUTABLE="rvmparser.exe"
fi

if [ -f "$EXECUTABLE" ]; then
    echo "Testing executable..."
    ./$EXECUTABLE --help 2>/dev/null || echo "Note: Executable created successfully"
    echo "Build verification: OK"
else
    echo "Error: Executable not found!"
    exit 1
fi