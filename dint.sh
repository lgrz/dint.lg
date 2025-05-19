#!/usr/bin/env bash

set -e

BUILD_PRESET="default"
BUILD_DIR="build"
DINT_SOURCE_DIR=`git rev-parse --show-toplevel`
CMAKE_ARGS=()

cmake --preset "$BUILD_PRESET" "${CMAKE_ARGS[@]}" -S "$DINT_SOURCE_DIR" -B "$BUILD_DIR"
ninja -C build
./build/test_dint
