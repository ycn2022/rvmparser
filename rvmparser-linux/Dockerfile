# Multi-stage Dockerfile for cross-platform builds

# Stage 1: Build environment
FROM ubuntu:22.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    gcc-multilib \
    g++-multilib \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build for x86_64
RUN mkdir build-x64 && cd build-x64 && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Build for ARM64
RUN mkdir build-arm64 && cd build-arm64 && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-linux-arm64.cmake -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Stage 2: Runtime environment (x86_64)
FROM ubuntu:22.04 as runtime-x64
WORKDIR /app
COPY --from=builder /app/build-x64/rvmparser /usr/local/bin/rvmparser
ENTRYPOINT ["/usr/local/bin/rvmparser"]

# Stage 3: Runtime environment (ARM64)
FROM ubuntu:22.04 as runtime-arm64
WORKDIR /app
COPY --from=builder /app/build-arm64/rvmparser /usr/local/bin/rvmparser
ENTRYPOINT ["/usr/local/bin/rvmparser"]