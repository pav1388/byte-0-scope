#!/usr/bin/env bash
# =====================================================================
#  build-linux-static.sh [clean] [release]
#
#  Linux, STATIC Qt 5.15 (Qt + qxcb plugin + libgcc/libstdc++ linked in).
#  glibc, X11/xcb and fontconfig stay dynamic, so the executable runs on
#  any desktop Linux with a glibc not older than the one it was built on
#  (build on the oldest distro you want to support) and needs no Qt.
#  Output: build-linux-static/byte0scope
#  clean    deletes build-linux-static/ first (full rebuild).
#  release  after the build, makes releases/byte-0-scope-v<ver>-linux-<arch>-static/
#           (byte0scope, README.md, README.ru.md, LICENSE) and the same .tar.gz
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

# --- release: releases/<NAME>/ + releases/<NAME>.tar.gz ----------------
make_release() {
    local version arch name release_root dest archive f
    [ -f "$PROJECT_ROOT/version.txt" ] || fail "version.txt not found."
    version="$(tr -d '[:space:]' < "$PROJECT_ROOT/version.txt")"
    [ -n "$version" ] || fail "version.txt is empty."
    arch="$(uname -m)"
    name="byte-0-scope-v${version}-linux-${arch}-static"
    release_root="$PROJECT_ROOT/releases"
    dest="$release_root/$name"
    archive="$release_root/$name.tar.gz"

    echo
    echo "==== Byte-0-scope release: $name ===="
    mkdir -p "$release_root"
    rm -rf "$dest" "$archive"
    mkdir -p "$dest"

    install -m 755 "$EXE_BUILT" "$dest/byte0scope" || fail "failed to copy byte0scope"
    echo "[ok] byte0scope ($(stat -c %s "$dest/byte0scope") bytes)"

    [ -f "$PROJECT_ROOT/README.md" ] || fail "README.md not found."
    install -m 644 "$PROJECT_ROOT/README.md" "$dest/README.md"
    echo "[ok] README.md"

    if [ -f "$PROJECT_ROOT/README.ru.md" ]; then
        install -m 644 "$PROJECT_ROOT/README.ru.md" "$dest/README.ru.md"
        echo "[ok] README.ru.md"
    else
        echo "[--] README.ru.md not found, skipped"
    fi

    local found_license=0
    for f in LICENSE LICENSE.md LICENSE.txt; do
        if [ -f "$PROJECT_ROOT/$f" ]; then
            install -m 644 "$PROJECT_ROOT/$f" "$dest/$f"
            echo "[ok] $f"
            found_license=1
            break
        fi
    done
    [ "$found_license" = 1 ] || echo "[--] LICENSE not found, skipped"

    tar --owner=0 --group=0 --numeric-owner -czf "$archive" -C "$release_root" "$name" \
        || fail "failed to create the archive."
    echo "[ok] $(basename "$archive") ($(stat -c %s "$archive") bytes)"

    echo
    echo "RELEASE READY"
    echo "Folder:  $dest"
    echo "Archive: $archive"
    (cd "$release_root" && sha256sum "$name.tar.gz")
}

DO_CLEAN=0
DO_RELEASE=0
for arg in "$@"; do
    case "$arg" in
        clean)   DO_CLEAN=1 ;;
        release) DO_RELEASE=1 ;;
        -h|--help|help)
            echo "Usage: $0 [clean] [release]"
            echo "  clean    delete $(basename "$BUILD_DIR")/ first (full rebuild)"
            echo "  release  after the build: make releases/<name>/ and the .tar.gz"
            exit 0 ;;
        *) echo "Unknown argument: $arg" >&2; echo "Usage: $0 [clean] [release]" >&2; exit 2 ;;
    esac
done

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

if [ "$DO_CLEAN" = 1 ]; then
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

[ "$DO_RELEASE" = 1 ] && make_release
exit 0
