#!/bin/bash

# Docker-based cross-platform build script

set -e

echo "=== Building rvmparser using Docker ==="

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed or not in PATH"
    exit 1
fi

# Build Docker image
echo "Building Docker image..."
docker build -t rvmparser-builder .

# Extract binaries from Docker image
echo "Extracting binaries..."

# Create output directory
mkdir -p dist

# Extract x86_64 binary
echo "Extracting x86_64 binary..."
docker create --name temp-x64 rvmparser-builder
docker cp temp-x64:/app/build-x64/rvmparser dist/rvmparser-linux-x64
docker rm temp-x64

# Extract ARM64 binary (if available)
echo "Extracting ARM64 binary..."
docker create --name temp-arm64 rvmparser-builder
docker cp temp-arm64:/app/build-arm64/rvmparser dist/rvmparser-linux-arm64 2>/dev/null || echo "ARM64 build not available"
docker rm temp-arm64 2>/dev/null || true

echo "=== Docker build completed ==="
echo "Binaries available in dist/ directory:"
ls -la dist/

# Make binaries executable
chmod +x dist/rvmparser-* 2>/dev/null || true

echo "Build completed successfully!"