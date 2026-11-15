================================================================
  Qt 5.15.2 static (Linux, x86_64) — build notes
================================================================
(not verified on every distro; adjust package names to yours)

Used by scripts/build-linux-static.sh. "Static" here means: Qt, the
xcb platform plugin, libgcc and libstdc++ are linked into the executable.
glibc, X11/xcb and fontconfig stay dynamic (system libraries).


DEPENDENCIES (Debian/Ubuntu)
----------------------------
  sudo apt install build-essential perl python3 pkg-config \
      libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev \
      libxext-dev libxfixes-dev libxi-dev libxrender-dev \
      libxcb1-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev \
      libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev \
      libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev \
      libxcb-util-dev libxcb-xinerama0-dev libxcb-xkb-dev \
      libxkbcommon-dev libxkbcommon-x11-dev

Build on the OLDEST distro you want to support: the executable needs a
glibc not older than the one it was built against.


DOWNLOAD
--------
  https://download.qt.io/archive/qt/5.15/5.15.2/single/qt-everywhere-src-5.15.2.tar.xz


LAYOUT
------
  ~/src/qt-everywhere-src-5.15.2/        sources
  <repo>/../_libs/qt-5.15.2-static-linux/ install prefix (default QT_DIR
                                          of build-linux-static.sh)


BUILD
-----
  tar xf qt-everywhere-src-5.15.2.tar.xz
  mkdir qt-build && cd qt-build

  ../qt-everywhere-src-5.15.2/configure -release -static -opensource \
      -confirm-license -prefix /path/to/qt-5.15.2-static-linux \
      -no-opengl -no-icu -no-dbus -no-openssl \
      -qt-zlib -qt-libpng -qt-libjpeg -qt-pcre -qt-harfbuzz \
      -fontconfig -system-freetype -qt-xcb -xkbcommon \
      -nomake examples -nomake tests \
      -skip qtwebengine -skip qtwebview -skip qtwebglplugin -skip qtscript \
      -skip qtquick3d -skip qtquickcontrols2 -skip qtdoc -skip qtcharts \
      -skip qtdatavis3d -skip qtlottie -skip qtmacextras -skip qtwinextras \
      -skip qtdeclarative -skip qtmultimedia -skip qt3d

  make -j$(nproc)
  make install

If GCC 13+ fails on Qt 5.15.2 sources (missing <limits> includes etc.),
build with an older compiler (gcc-11/12) or use Qt 5.15.x from the KDE
Qt patch collection.


VERIFY
------
  ls /path/to/qt-5.15.2-static-linux/lib/libQt5Core.a
  ls /path/to/qt-5.15.2-static-linux/lib/libQt5Gui.a
  ls /path/to/qt-5.15.2-static-linux/lib/libQt5Widgets.a
  ls /path/to/qt-5.15.2-static-linux/plugins/platforms/libqxcb.a
  ls /path/to/qt-5.15.2-static-linux/bin/lrelease

All five must exist.


NOTES
-----
- -no-opengl: no libGL dependency; the app uses only QPainter/QImage.
- -qt-xcb: bundled xcb libraries are linked into the plugin; X11 itself
  (libX11, libXext, ...) and fontconfig are still system libraries.
- A static Qt on Linux does not make a fully static executable and does
  not use Wayland natively (it runs through XWayland).
- Check the result: build-linux-static.sh prints `ldd`; there must be no
  libQt5*.so in the list.
