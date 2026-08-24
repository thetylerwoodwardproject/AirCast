#!/usr/bin/env sh
# Maintainer script: stages an already-built Linux Release VST3 plus
# install.sh into packaging/linux/dist/AirCast-<version>-Linux.tar.gz.
#
# This must be run against a build produced ON LINUX (there's no cross-compile
# here) - it cannot be exercised on macOS. Not executed/verified this session;
# written to spec. See PACKAGING.md.
set -eu

usage() {
    echo "Usage: $0 [--build-dir <path-to-Release-artefacts>]"
    echo ""
    echo "  --build-dir   Path to the Release artefact root, i.e. the directory"
    echo "                containing VST3/AirCast.vst3."
    echo "                Default: build/AirCast_artefacts/Release"
    exit "${1:-0}"
}

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
ARTEFACTS="$REPO_ROOT/build/AirCast_artefacts/Release"

while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir) ARTEFACTS="$2"; shift 2 ;;
        -h|--help) usage 0 ;;
        *) echo "Unknown option: $1" >&2; usage 1 ;;
    esac
done

VST3_SRC="$ARTEFACTS/VST3/AirCast.vst3"
if [ ! -d "$VST3_SRC" ]; then
    echo "error: couldn't find a built VST3 at $VST3_SRC" >&2
    echo "Build a Release configuration first (see INSTALL.md), or pass --build-dir." >&2
    exit 1
fi

VERSION=$("$SCRIPT_DIR/../common/get-version.sh")
echo "Packaging AirCast $VERSION..."

STAGE_NAME="AirCast-$VERSION-Linux"
WORK="$SCRIPT_DIR/.build"
STAGE="$WORK/$STAGE_NAME"
rm -rf "$WORK"
mkdir -p "$STAGE"

cp -R "$VST3_SRC" "$STAGE/AirCast.vst3"
cp "$SCRIPT_DIR/install.sh" "$STAGE/"
chmod +x "$STAGE/install.sh"

mkdir -p "$SCRIPT_DIR/dist"
OUT="$SCRIPT_DIR/dist/$STAGE_NAME.tar.gz"
tar -czf "$OUT" -C "$WORK" "$STAGE_NAME"

echo ""
echo "Built: $OUT"
echo "Contents: AirCast.vst3/ + install.sh"
