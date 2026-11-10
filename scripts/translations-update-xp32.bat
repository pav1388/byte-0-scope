@echo off
setlocal

set "PATH=C:\mingw32\bin;%PATH%"

cd /d "%~dp0"

if not exist "..\build-xp32" (
    echo Build folder not found: ..\build-xp32
    pause
    exit /b 1
)
cd /d "..\build-xp32"

mingw32-make update_translations
REM pause