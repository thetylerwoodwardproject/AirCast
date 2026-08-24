#!/usr/bin/env sh
# Prints AirCast's version (e.g. "0.1.0"), parsed from the single source of
# truth: CMakeLists.txt's project(AirCast VERSION x.y.z) call. Never hardcode
# the version anywhere else in the packaging scripts - always call this.
set -eu

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
CMAKE_LISTS="$SCRIPT_DIR/../../CMakeLists.txt"

if [ ! -f "$CMAKE_LISTS" ]; then
    echo "error: $CMAKE_LISTS not found" >&2
    exit 1
fi

VERSION=$(sed -nE 's/.*project\(AirCast VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$CMAKE_LISTS" | head -n1)

if [ -z "$VERSION" ]; then
    echo "error: could not parse a version out of project(AirCast VERSION x.y.z) in $CMAKE_LISTS" >&2
    exit 1
fi

echo "$VERSION"
