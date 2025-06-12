#!/usr/bin/env bash

set -e

export CC="clang"
export CXX="clang++"
export CPM_SOURCE_CACHE=".cache/cpm"

DINT_SOURCE_DIR=`git rev-parse --show-toplevel`
BUILD_PRESET=${1:-default}
case $BUILD_PRESET in
    "default")
        BUILD_DIR="${DINT_SOURCE_DIR}/build/release"
        ;;
    "Debug")
        BUILD_DIR="${DINT_SOURCE_DIR}/build/debug"
        ;;
    *)
        echo Unknown build preset: $BUILD_PRESET >&2
        exit 1
        ;;
esac

CMAKE_ARGS=()
cmake --preset "$BUILD_PRESET" "${CMAKE_ARGS[@]}" -S "$DINT_SOURCE_DIR" -B "$BUILD_DIR"
ninja -C $BUILD_DIR
