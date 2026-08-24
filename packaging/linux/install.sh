#!/usr/bin/env sh
# Installs AirCast.vst3 (expected next to this script, i.e. inside the
# extracted tarball) into the standard VST3 location.
#
#   ./install.sh            installs to ~/.vst3           (no root, default)
#   ./install.sh --system   installs to /usr/lib/vst3      (needs sudo)
set -eu

SCOPE="user"
while [ $# -gt 0 ]; do
    case "$1" in
        --system) SCOPE="system"; shift ;;
        --user)   SCOPE="user";   shift ;;
        -h|--help)
            echo "Usage: $0 [--system|--user]"
            echo "  (default) --user    installs to ~/.vst3, no root required"
            echo "            --system  installs to /usr/lib/vst3, requires sudo"
            exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
SRC="$SCRIPT_DIR/AirCast.vst3"

if [ ! -d "$SRC" ]; then
    echo "error: AirCast.vst3 not found next to install.sh (expected at $SRC)" >&2
    exit 1
fi

if [ "$SCOPE" = "system" ]; then
    if [ "$(id -u)" -ne 0 ]; then
        echo "error: --system requires root. Re-run as: sudo $0 --system" >&2
        exit 1
    fi
    DEST="/usr/lib/vst3"
else
    if [ "$(id -u)" -eq 0 ]; then
        echo "error: refusing to install a per-user plugin as root - this would" >&2
        echo "root-own files under a home directory. Run without sudo, or pass --system" >&2
        echo "to install system-wide to /usr/lib/vst3 instead." >&2
        exit 1
    fi
    DEST="$HOME/.vst3"
fi

mkdir -p "$DEST"
rm -rf "${DEST:?}/AirCast.vst3"
cp -R "$SRC" "$DEST/"

echo "Installed AirCast.vst3 to $DEST"
echo "Rescan plugins in your DAW to pick it up."
