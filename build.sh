#!/usr/bin/env bash
set -euo pipefail

# Minimal build script: configure and build using CMake
mkdir -p build
cmake -S . -B build
cmake --build build -- -j"$(nproc)"

echo "Build complete. Run the program with: ./build/main"

