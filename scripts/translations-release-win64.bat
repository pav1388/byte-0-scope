@echo off
setlocal

set "PATH=C:\mingw64\bin;%PATH%"

cd /d "%~dp0"

if not exist "..\build-win64" (
    echo Build folder not found: ..\build-win64
    pause
    exit /b 1
)
cd /d "..\build-win64"

mingw32-make release_translations
REM pause