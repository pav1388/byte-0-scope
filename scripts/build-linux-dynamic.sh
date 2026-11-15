#!/usr/bin/env bash
# =====================================================================
#  build-linux-dynamic.sh [clean]
#
#  Linux, system (dynamic) Qt 5 + system GCC.
#  Output: build-linux-dynamic/byte0scope
#  "clean" as the argument deletes build-linux-dynamic/ first.
#
#  Needs (Debian/Ubuntu):
#    sudo apt install build-essential cmake qtbase5-dev qttools5-dev \
#                     qttools5-dev-tools zlib1g-dev qttranslations5-l10n
# =====================================================================
set -euo pipefail

# ---------------------- SETTINGS: edit for your PC -------------------
# Parallel jobs.
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"

# Optional: a working Qt5 lrelease binary, used when the Qt5LinguistTools
# CMake files are missing (e.g. broken/missing qttools5-dev). Empty = auto.
LRELEASE_EXE="${LRELEASE_EXE:-}"
# ---------------------------------------------------------------------

PROFILE="native"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-linux-dynamic"
EXE_BUILT="$BUILD_DIR/byte0scope"

fail() { echo; echo "ERROR: $*" >&2; exit 1; }

command -v cmake >/dev/null 2>&1 || fail "cmake not found. Install it: sudo apt install cmake"
command -v g++   >/dev/null 2>&1 || fail "g++ not found. Install it: sudo apt install build-essential"
[ -f "$PROJECT_ROOT/CMakeLists.txt" ] || fail "CMakeLists.txt not found in $PROJECT_ROOT"

# Fallback lrelease: the one unpacked by hand (dpkg -x qttools5-dev-tools ...)
if [ -z "$LRELEASE_EXE" ] && [ -x "$HOME/qt5tools/usr/lib/qt5/bin/lrelease" ]; then
    LRELEASE_EXE="$HOME/qt5tools/usr/lib/qt5/bin/lrelease"
fi

echo
echo "==== Byte-0-scope build: linux-dynamic ===="
echo "g++:    $(g++ -dumpversion) ($(g++ -dumpmachine))"
echo "cmake:  $(cmake --version | head -n1)"
echo "Build:  $BUILD_DIR"
[ -n "$LRELEASE_EXE" ] && echo "lrelease: $LRELEASE_EXE"
echo

if [ "${1:-}" = "clean" ]; then
    echo "Removing $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

CMAKE_ARGS=(
    -S "$PROJECT_ROOT" -B "$BUILD_DIR"
    -DBS_PROFILE="$PROFILE"
    -DBS_STATIC_QT=OFF
    -DCMAKE_BUILD_TYPE=Release
)
[ -n "$LRELEASE_EXE" ] && CMAKE_ARGS+=(-DBS_LRELEASE_EXE="$LRELEASE_EXE")

cmake "${CMAKE_ARGS[@]}"  || fail "CMake configuration failed."
cmake --build "$BUILD_DIR" -j"$JOBS" || fail "Build failed."
[ -f "$EXE_BUILT" ] || fail "byte0scope was not produced."

echo
echo "BUILD SUCCESSFUL"
echo "Output: $EXE_BUILT ($(stat -c %s "$EXE_BUILT") bytes)"
file "$EXE_BUILT" 2>/dev/null || true
