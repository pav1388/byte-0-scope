@echo off
setlocal EnableExtensions

REM =====================================================================
REM  build-win64.bat [clean]
REM
REM  Windows 7 and newer, 64-bit: static Qt 5.15 + MinGW-w64 (x86_64)
REM  Output: build-win64\byte0scope.exe
REM  "clean" as the argument deletes build-win64\ first (full rebuild).
REM =====================================================================

REM ---------------------- SETTINGS: edit for your PC -------------------
REM MinGW "bin" folder (64-bit MinGW-w64, msvcrt build for Windows 7, for example C:\mingw64\bin).
REM Leave empty to use the compiler that is already first in PATH.
set "MINGW_BIN=C:\mingw64\bin"

REM Static Qt SDK folder (contains bin, lib, plugins).
set "QT_DIR=%~dp0..\..\_libs\qt-5.15.2-static"

REM Parallel jobs for mingw32-make.
set "JOBS=2"
REM ---------------------------------------------------------------------

set "ERR="
set "PROFILE=win64"
for %%i in ("%~dp0..") do set "PROJECT_ROOT=%%~fi"
for %%i in ("%QT_DIR%") do set "QT_DIR=%%~fi"
set "QT_DIR_CMAKE=%QT_DIR:\=/%"
set "BUILD_DIR=%PROJECT_ROOT%\build-%PROFILE%"
set "EXE_BUILT=%BUILD_DIR%\byte0scope.exe"

if defined MINGW_BIN set "PATH=%MINGW_BIN%;%PATH%"

where cmake >nul 2>&1
if errorlevel 1 set "ERR=cmake not found in PATH." & goto :fail
where gcc >nul 2>&1
if errorlevel 1 set "ERR=gcc not found. Set MINGW_BIN at the top of this script." & goto :fail
where mingw32-make >nul 2>&1
if errorlevel 1 set "ERR=mingw32-make not found in PATH." & goto :fail
if not exist "%QT_DIR%" set "ERR=Qt SDK not found at %QT_DIR%. Edit QT_DIR at the top of this script." & goto :fail
if not exist "%PROJECT_ROOT%\CMakeLists.txt" set "ERR=CMakeLists.txt not found in %PROJECT_ROOT%" & goto :fail

REM --- the compiler must match this build (64-bit) ---
for /f "delims=" %%v in ('gcc -dumpversion') do set "GCC_VER=%%v"
for /f "delims=" %%m in ('gcc -dumpmachine') do set "GCC_MACHINE=%%m"
set "GCC_IS64=0"
echo %GCC_MACHINE%| findstr /i "x86_64 amd64" >nul && set "GCC_IS64=1"
if "%GCC_IS64%"=="0" set "ERR=This build needs a 64-bit MinGW-w64, but gcc is %GCC_MACHINE%. Set MINGW_BIN at the top of this script." & goto :fail

echo.
echo ==== Byte-0-scope build: %PROFILE% ====
echo gcc:   %GCC_VER% ^(%GCC_MACHINE%^)
where gcc
echo Qt:    %QT_DIR%
echo Build: %BUILD_DIR%
echo.

if /i not "%~1"=="clean" goto :no_clean
if exist "%BUILD_DIR%" echo Removing %BUILD_DIR%
if exist "%BUILD_DIR%" rmdir /S /Q "%BUILD_DIR%"
:no_clean

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

cmake -G "MinGW Makefiles" ^
      -DBS_PROFILE=%PROFILE% ^
      -DCMAKE_PREFIX_PATH="%QT_DIR_CMAKE%" ^
      -DBS_QT_DIR="%QT_DIR_CMAKE%" ^
      -DBS_UPX=OFF ^
      "%PROJECT_ROOT%"
if errorlevel 1 set "ERR=CMake configuration failed." & goto :fail

mingw32-make -j%JOBS%
if errorlevel 1 set "ERR=Build failed." & goto :fail

if not exist "%EXE_BUILT%" set "ERR=byte0scope.exe was not produced." & goto :fail

for %%F in ("%EXE_BUILT%") do set "EXE_SIZE=%%~zF"
echo.
echo BUILD SUCCESSFUL
echo Output: %EXE_BUILT% ^(%EXE_SIZE% bytes^)
REM pause
exit /b 0

:fail
echo.
echo ERROR: %ERR%
REM pause
exit /b 1
