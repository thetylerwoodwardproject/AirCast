#!/usr/bin/env sh
# Builds packaging/macos/dist/AirCast-<version>-macOS.pkg from an already-built
# Release artefact tree. Does NOT build the plugin - run this after a normal
# Release build (see INSTALL.md). Produces one unsigned .pkg with two
# selectable components (VST3, AU); the user picks "install for me" or
# "install for all users" in Installer.app.
set -eu

usage() {
    echo "Usage: $0 [--build-dir <path-to-Release-artefacts>]"
    echo ""
    echo "  --build-dir   Path to the Release artefact root, i.e. the directory"
    echo "                containing VST3/AirCast.vst3 and AU/AirCast.component."
    echo "                Default: build-release/AirCast_artefacts/Release"
    exit "${1:-0}"
}

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
ARTEFACTS="$REPO_ROOT/build-release/AirCast_artefacts/Release"

while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir) ARTEFACTS="$2"; shift 2 ;;
        -h|--help) usage 0 ;;
        *) echo "Unknown option: $1" >&2; usage 1 ;;
    esac
done

VST3_SRC="$ARTEFACTS/VST3/AirCast.vst3"
AU_SRC="$ARTEFACTS/AU/AirCast.component"

if [ ! -d "$VST3_SRC" ] || [ ! -d "$AU_SRC" ]; then
    echo "error: couldn't find a built VST3 and AU under:" >&2
    echo "  $ARTEFACTS" >&2
    echo "Build a Release configuration first - see INSTALL.md - or pass --build-dir." >&2
    exit 1
fi

VERSION=$("$SCRIPT_DIR/../common/get-version.sh")
echo "Packaging AirCast $VERSION..."

WORK="$SCRIPT_DIR/.build"
rm -rf "$WORK"
mkdir -p "$WORK/Resources"

# --- VST3 component ---
pkgbuild --analyze --root "$VST3_SRC" "$WORK/vst3.plist" >/dev/null
plutil -replace BundleIsRelocatable -bool NO "$WORK/vst3.plist"
plutil -replace BundleIsVersionChecked -bool NO "$WORK/vst3.plist"
pkgbuild \
    --root "$VST3_SRC" \
    --component-plist "$WORK/vst3.plist" \
    --identifier "com.fullymodulated.aircast.vst3.pkg" \
    --version "$VERSION" \
    --install-location "Library/Audio/Plug-Ins/VST3/AirCast.vst3" \
    "$WORK/AirCast-vst3.pkg" >/dev/null

# --- AU component ---
pkgbuild --analyze --root "$AU_SRC" "$WORK/au.plist" >/dev/null
plutil -replace BundleIsRelocatable -bool NO "$WORK/au.plist"
plutil -replace BundleIsVersionChecked -bool NO "$WORK/au.plist"
pkgbuild \
    --root "$AU_SRC" \
    --component-plist "$WORK/au.plist" \
    --identifier "com.fullymodulated.aircast.au.pkg" \
    --version "$VERSION" \
    --install-location "Library/Audio/Plug-Ins/Components/AirCast.component" \
    "$WORK/AirCast-au.pkg" >/dev/null

# --- License pane: only if a LICENSE file actually exists at the repo root ---
LICENSE_LINE=""
if [ -f "$REPO_ROOT/LICENSE" ]; then
    cp "$REPO_ROOT/LICENSE" "$WORK/Resources/License.txt"
    LICENSE_LINE='<license file="License.txt" mime-type="text/plain"/>'
else
    echo "note: no LICENSE file at repo root - installer will skip the license pane."
fi

sed \
    -e "s/@VERSION@/$VERSION/g" \
    -e "s#@LICENSE_LINE@#$LICENSE_LINE#" \
    "$SCRIPT_DIR/distribution.xml.in" > "$WORK/distribution.xml"

mkdir -p "$SCRIPT_DIR/dist"
OUT="$SCRIPT_DIR/dist/AirCast-$VERSION-macOS.pkg"
rm -f "$OUT"

productbuild \
    --distribution "$WORK/distribution.xml" \
    --resources "$WORK/Resources" \
    --package-path "$WORK" \
    "$OUT"

echo ""
echo "Built: $OUT"
echo ""
echo "NOTE: this package is unsigned (no Apple Developer ID Installer certificate"
echo "configured). End users will need to right-click the .pkg and choose Open,"
echo "or approve it in System Settings > Privacy & Security, to run it."
