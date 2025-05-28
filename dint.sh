#!/usr/bin/env bash

set -e

BUILD_PRESET="default"
BUILD_DIR="build/release"
DINT_SOURCE_DIR=`git rev-parse --show-toplevel`
CMAKE_ARGS=()

export CC="clang"
export CXX="clang++"
export CPM_SOURCE_CACHE=".cache/cpm"

cmake --preset "$BUILD_PRESET" "${CMAKE_ARGS[@]}" -S "$DINT_SOURCE_DIR" -B "$BUILD_DIR"
ninja -C $BUILD_DIR
