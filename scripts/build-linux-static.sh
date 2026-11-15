#!/usr/bin/env bash
# =====================================================================
#  build-linux-static.sh [clean]
#
#  Linux, STATIC Qt 5.15 (Qt + qxcb plugin + libgcc/libstdc++ linked in).
#  glibc, X11/xcb and fontconfig stay dynamic, so the executable runs on
#  any desktop Linux with a glibc not older than the one it was built on
#  (build on the oldest distro you want to support) and needs no Qt.
#  Output: build-linux-static/byte0scope
#  "clean" as the argument deletes build-linux-static/ first.
#
#  Needs a static Qt SDK - see scripts/BUILD-QT-5.15.2-static-linux.md
# =====================================================================
set -euo pipefail

# ---------------------- SETTINGS: edit for your PC -------------------
# Static Qt SDK folder (contains bin, lib, plugins).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QT_DIR="${QT_DIR:-$SCRIPT_DIR/../../_libs/qt-5.15.2-static-linux}"

# Parallel jobs.
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"
# ---------------------------------------------------------------------

PROFILE="native"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-linux-static"
EXE_BUILT="$BUILD_DIR/byte0scope"

fail() { echo; echo "ERROR: $*" >&2; exit 1; }

command -v cmake >/dev/null 2>&1 || fail "cmake not found. Install it: sudo apt install cmake"
command -v g++   >/dev/null 2>&1 || fail "g++ not found. Install it: sudo apt install build-essential"
[ -f "$PROJECT_ROOT/CMakeLists.txt" ] || fail "CMakeLists.txt not found in $PROJECT_ROOT"

[ -d "$QT_DIR" ] || fail "Static Qt SDK not found at $QT_DIR. Set QT_DIR (env or at the top of this script)."
QT_DIR="$(cd "$QT_DIR" && pwd)"
for f in lib/libQt5Core.a lib/libQt5Gui.a lib/libQt5Widgets.a plugins/platforms/libqxcb.a; do
    [ -f "$QT_DIR/$f" ] || fail "$QT_DIR/$f not found. Is this a static Qt SDK built with xcb? (see scripts/BUILD-QT-5.15.2-static-linux.md)"
done

echo
echo "==== Byte-0-scope build: linux-static ===="
echo "g++:    $(g++ -dumpversion) ($(g++ -dumpmachine))"
echo "cmake:  $(cmake --version | head -n1)"
echo "Qt:     $QT_DIR"
echo "Build:  $BUILD_DIR"
echo

if [ "${1:-}" = "clean" ]; then
    echo "Removing $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

# A static Qt SDK usually has its own lrelease in bin/ (used for the
# embedded Russian translation); CMake finds Qt5LinguistTools there.
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
      -DBS_PROFILE="$PROFILE" \
      -DCMAKE_PREFIX_PATH="$QT_DIR" \
      -DBS_QT_DIR="$QT_DIR" \
      -DBS_STATIC_QT=ON \
      -DBS_STATIC_RUNTIME=ON \
      -DBS_NO_OPENGL=ON \
      -DCMAKE_BUILD_TYPE=Release \
    || fail "CMake configuration failed."

cmake --build "$BUILD_DIR" -j"$JOBS" || fail "Build failed."
[ -f "$EXE_BUILT" ] || fail "byte0scope was not produced."

echo
echo "BUILD SUCCESSFUL"
echo "Output: $EXE_BUILT ($(stat -c %s "$EXE_BUILT") bytes)"
file "$EXE_BUILT" 2>/dev/null || true
echo
echo "Dynamic dependencies (must not include libQt5*):"
ldd "$EXE_BUILT" 2>/dev/null | awk '{print "  " $1}' || true
if ldd "$EXE_BUILT" 2>/dev/null | grep -q "libQt5"; then
    echo "WARNING: the executable still links against libQt5*.so - Qt is not static!" >&2
fi
