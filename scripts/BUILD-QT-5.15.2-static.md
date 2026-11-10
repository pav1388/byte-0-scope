================================================================
  Qt 5.15.2 static (Windows 7+, 64-bit) — build notes
================================================================

DOWNLOAD
--------
MinGW-w64 8.1.0 (x86_64, posix, seh, msvcrt):
  https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/8.1.0/threads-posix/seh/x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z/download

Qt sources:
  https://download.qt.io/archive/qt/5.15/5.15.2/single/qt-everywhere-src-5.15.2.zip


LAYOUT
------
  C:\mingw64\bin\                        compiler (x86_64-8.1.0-posix-seh)
  R:\qt-everywhere-src-5.15.2\           sources
  R:\qt-5.15.2-static\                   install prefix


BUILD
-----
  set PATH=C:\mingw64\bin;%PATH%

  cd /d R:\qt-everywhere-src-5.15.2

  configure -release -static -static-runtime -platform win32-g++ -opensource -confirm-license -prefix R:\qt-5.15.2-static -opengl desktop -no-angle -no-icu -qt-zlib -qt-libpng -qt-libjpeg -qt-freetype -qt-pcre -qt-harfbuzz -no-openssl -no-dbus -nomake examples -nomake tests -skip qtwebengine -skip qtwebview -skip qtwebglplugin -skip qtscript -skip qtquick3d -skip qtquickcontrols2 -skip qtdoc -skip qtcharts -skip qtdatavis3d -skip qtlottie -skip qtmacextras -skip qtwinextras

  mingw32-make -j4
  mingw32-make install


VERIFY
------
  dir R:\qt-5.15.2-static\lib\libQt5Core.a
  dir R:\qt-5.15.2-static\lib\libQt5Gui.a
  dir R:\qt-5.15.2-static\lib\libQt5Widgets.a
  dir R:\qt-5.15.2-static\plugins\platforms\libqwindows.a

All four must exist.


NOTES
-----
- compiler: MinGW-w64 8.1.0 (x86_64, posix, seh). This is the exact
  toolchain used by Qt Company for official Qt 5.15.x Windows packages
  and confirmed to build Qt 5.15.2 from source without patches.
- Do NOT use GCC 12.x / 16.x: they need code patches (qcompare ABI,
  SFINAE in QAnyStringView/QRegularExpression/QChar/QBitArray).
- msvcrt runtime: 8.1.0 mingw-builds releases are msvcrt by default
  (the "ucrt" variants only appeared in later builds). No suffix in
  the archive name — that is normal.
- -static -static-runtime: required, else no libqwindows.a.
- -opengl desktop -no-angle: no ANGLE / DirectX SDK.
- -no-icu: ICU for MinGW is usually not built; disable it.
- -no-pch: optional safety; PCH works on 8.1.0 but skipping it avoids
  occasional stale-cache issues after source edits. Remove the flag
  if you want faster rebuilds and are okay with PCH on 8.1.0.
