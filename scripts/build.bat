@echo off
setlocal

cd /d "%~dp0"

for %%i in ("%~dp0..") do set PROJECT_ROOT=%%~fi
set BUILD_DIR=%PROJECT_ROOT%\build
set QT_DIR=%PROJECT_ROOT%\../_libs\qt-5.6.3-static
set EXE_OUT=%PROJECT_ROOT%\byte0scope.exe

where cmake >nul 2>&1 || (echo ERROR: cmake not in PATH. & pause & exit /b 1)
where gcc   >nul 2>&1 || (echo ERROR: gcc not in PATH. & pause & exit /b 1)
where mingw32-make >nul 2>&1 || (echo ERROR: mingw32-make not in PATH. & pause & exit /b 1)
if not exist "%QT_DIR%" (echo ERROR: Qt SDK not found at "%QT_DIR%" & pause & exit /b 1)
if not exist "%PROJECT_ROOT%\CMakeLists.txt" (echo ERROR: CMakeLists.txt not found. & pause & exit /b 1)

echo Toolchain:
where gcc
gcc --version | findstr /R "^gcc"
echo.

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

cmake -G "MinGW Makefiles" ^
      -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
      -DBS_QT_DIR="%QT_DIR%" ^
      -DBS_UPX=OFF ^
      "%PROJECT_ROOT%"
if errorlevel 1 (echo ERROR: CMake configuration failed. & pause & exit /b 1)

mingw32-make -j4
if errorlevel 1 (echo ERROR: Build failed. & pause & exit /b 1)

if not exist "byte0scope.exe" (echo WARNING: byteoscope.exe not produced. & pause & exit /b 1)

echo.
echo BUILD SUCCESSFUL
echo Output: %EXE_OUT%
pause
exit /b 0