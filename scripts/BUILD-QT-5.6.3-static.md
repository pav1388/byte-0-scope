================================================================
  Qt 5.6.3 static (Windows XP+, 32-bit) — build notes
================================================================

DOWNLOAD
--------
MinGW-w64 4.9.2 (i686, posix, dwarf, msvcrt):
  https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/4.9.2/threads-posix/dwarf/i686-4.9.2-release-posix-dwarf-rt_v4-rev4.7z/download

Qt sources:
  https://download.qt.io/archive/qt/5.6/5.6.3/single/qt-everywhere-opensource-src-5.6.3.zip


LAYOUT
------
  C:\mingw32\bin\                          compiler (4.9.2)
  R:\qt-everywhere-opensource-src-5.6.3\   sources
  R:\qt-5.6.3-static\                      install prefix


BUILD
-----
  set PATH=C:\mingw32\bin;%PATH%

  cd /d R:\qt-everywhere-opensource-src-5.6.3

  configure -release -static -target xp -platform win32-g++ -opensource -confirm-license -prefix R:\qt-5.6.3-static -opengl desktop -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -make libs -nomake tools -nomake examples -nomake tests -skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtconnectivity -skip qtdeclarative -skip qtdoc -skip qtlocation -skip qtmultimedia -skip qtscript -skip qtsensors -skip qtsvg -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebview

  mingw32-make -j4
  mingw32-make install


VERIFY
------
  dir R:\qt-5.6.3-static\lib\libQt5Core.a
  dir R:\qt-5.6.3-static\lib\libQt5Gui.a
  dir R:\qt-5.6.3-static\lib\libQt5Widgets.a
  dir R:\qt-5.6.3-static\plugins\platforms\libqwindows.a

All four must exist.


NOTES
-----
- compiler: MinGW-w64 4.9.2 (i686, posix, dwarf, rt).
  Do NOT use 8.1.0 / 12.x / 16.x for Qt 5.6.3: 8.1.0+ needs patches,
  12+ and 16+ fail without code changes.
- msvcrt runtime: required for XP. ucrt builds do not work on XP.
- -static -static-runtime: required, else no libqwindows.a.
- -target xp: required for XP; add -no-directwrite if DirectWrite errors.
- -opengl desktop: no ANGLE / DirectX SDK.
- -no-sse4.1 -no-sse4.2 -no-avx -no-avx2: for old CPUs. All four flags
  are supported by GCC 4.9.2.
- LTO: MinGW 4.9.2 ar/gcc-ar is built WITHOUT plugin support.
  Building Qt 5.6.3 with LTO on this toolchain fails at link time
  ("sorry - this program has been built without plugin support").
  Do not enable LTO for xp32; in the project CMakeLists BS_LTO
  defaults to OFF for this profile.
