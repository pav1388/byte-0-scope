@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM =====================================================================
REM  release-xp32.bat
REM
REM  Packs releases\byte-0-scope-vVERSION-win32-static\ and the matching .zip
REM  from build-xp32\byte0scope.exe  (Windows XP+, 32-bit).
REM  Build first: scripts\build-xp32.bat
REM =====================================================================

REM ---------------------- SETTINGS: edit if needed ---------------------
REM Release name suffix: byte-0-scope-vVERSION-TAG
set "TAG=win32-static"

REM 1 = compress byte0scope.exe with UPX (upx must be in PATH), 0 = do not.
REM (UPX is on for the XP build, as before.)
set "USE_UPX=1"
REM ---------------------------------------------------------------------

set "ERR="
set "PROFILE=xp32"
for %%i in ("%~dp0..") do set "PROJECT_ROOT=%%~fi"

set "SRC_EXE=%PROJECT_ROOT%\build-%PROFILE%\byte0scope.exe"
set "SRC_README=%PROJECT_ROOT%\README.md"
set "SRC_README_RU=%PROJECT_ROOT%\README.ru.md"
set "SRC_SAMPLES=%PROJECT_ROOT%\sample-bin"
set "SRC_SAMPLES_ZIP=%SRC_SAMPLES%\sample-bin.zip"
set "RELEASE_ROOT=%PROJECT_ROOT%\releases"

REM --- read version (first line, spaces stripped) ---
if not exist "%PROJECT_ROOT%\version.txt" set "ERR=version.txt not found at %PROJECT_ROOT%" & goto :fail
set "VERSION="
set /p VERSION=<"%PROJECT_ROOT%\version.txt"
set "VERSION=%VERSION: =%"
if "%VERSION%"=="" set "ERR=version.txt is empty." & goto :fail

set "NAME=byte-0-scope-v%VERSION%-%TAG%"
set "DEST=%RELEASE_ROOT%\%NAME%"
set "ZIP_PATH=%RELEASE_ROOT%\%NAME%.zip"
set "DEST_SAMPLES=%DEST%\sample-bin"

REM --- sanity ---
if not exist "%SRC_EXE%" set "ERR=%SRC_EXE% not found. Build first: scripts\build-%PROFILE%.bat" & goto :fail
if not exist "%SRC_README%" set "ERR=%SRC_README% not found." & goto :fail

echo.
echo ==== Byte-0-scope release: %PROFILE% ====

REM --- create release dir ---
if not exist "%RELEASE_ROOT%" mkdir "%RELEASE_ROOT%"
if exist "%DEST%" rmdir /S /Q "%DEST%"
if not exist "%DEST%" mkdir "%DEST%"

REM --- README.md (always overwrite) ---
copy /Y "%SRC_README%" "%DEST%\README.md" >nul
if errorlevel 1 set "ERR=failed to copy README.md" & goto :fail
echo [ok] README.md

if exist "%SRC_README_RU%" (
    copy /Y "%SRC_README_RU%" "%DEST%\README.ru.md" >nul
    if errorlevel 1 set "ERR=failed to copy README.ru.md" & goto :fail
    echo [ok] README.ru.md
) else (
    echo [--] README.ru.md not found, skipped
)

REM --- sample-tim (extract sample-tim.zip into release sample-tim) ---
REM if exist "%SRC_SAMPLES%" (
    REM if not exist "%SRC_SAMPLES_ZIP%" (
        REM echo ERROR: %SRC_SAMPLES_ZIP% not found.
        REM pause & exit /b 1
    REM )

    REM if exist "%DEST_SAMPLES%" rmdir /S /Q "%DEST_SAMPLES%"
    REM mkdir "%DEST_SAMPLES%"
    REM if not exist "%DEST_SAMPLES%" (
        REM echo ERROR: failed to create "%DEST_SAMPLES%"
        REM pause & exit /b 1
    REM )

    REM where tar >nul 2>&1
    REM if errorlevel 1 (
        REM echo [--] tar not found, trying PowerShell Expand-Archive...
        REM powershell -NoProfile -ExecutionPolicy Bypass -Command ^
            REM "Expand-Archive -LiteralPath '%SRC_SAMPLES_ZIP%' -DestinationPath '%DEST_SAMPLES%' -Force"
        REM if errorlevel 1 (
            REM echo ERROR: failed to extract sample-tim.zip with PowerShell
            REM pause & exit /b 1
        REM )
    REM ) else (
        REM tar -xf "%SRC_SAMPLES_ZIP%" -C "%DEST_SAMPLES%"
        REM if errorlevel 1 (
            REM echo ERROR: failed to extract sample-tim.zip with tar
            REM pause & exit /b 1
        REM )
    REM )
    REM echo [ok] sample-tim extracted
REM ) else (
    REM echo [--] sample-tim folder not found, skipped
REM )


REM --- byte0scope.exe (copy + optional UPX) ---
copy /Y "%SRC_EXE%" "%DEST%\byte0scope.exe" >nul
if errorlevel 1 set "ERR=failed to copy byte0scope.exe" & goto :fail
for %%F in ("%DEST%\byte0scope.exe") do set "EXE_SIZE_RAW=%%~zF"
echo [ok] byte0scope.exe copied ^(!EXE_SIZE_RAW! bytes^)

if not "%USE_UPX%"=="1" goto :upx_off
where upx >nul 2>&1
if errorlevel 1 goto :upx_missing

REM Decompress if already packed (from a previous UPX=ON build).
REM Errors are ignored: if the file isn't packed, upx -d exits non-zero.
upx -d "%DEST%\byte0scope.exe" >nul 2>&1
upx --best --force "%DEST%\byte0scope.exe"
if errorlevel 1 set "ERR=upx failed." & goto :fail
for %%F in ("%DEST%\byte0scope.exe") do set "EXE_SIZE_PACKED=%%~zF"
echo [ok] byte0scope.exe compressed ^(!EXE_SIZE_PACKED! bytes, was !EXE_SIZE_RAW!^)
goto :upx_done

:upx_off
echo [--] UPX disabled ^(USE_UPX=0^), skipping compression
goto :upx_done

:upx_missing
echo [--] upx not found in PATH, skipping compression

:upx_done

REM --- zip packaging ---
if exist "%ZIP_PATH%" del /Q "%ZIP_PATH%"

set "ZIP_RC=1"

where tar >nul 2>&1
if errorlevel 1 goto :zip_powershell

pushd "%RELEASE_ROOT%"
tar -a -c -f "%NAME%.zip" "%NAME%"
set "ZIP_RC=%ERRORLEVEL%"
popd
goto :zip_check

:zip_powershell
echo [--] tar not found, trying PowerShell Compress-Archive...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "Compress-Archive -Path '%RELEASE_ROOT%\%NAME%' -DestinationPath '%ZIP_PATH%' -Force"
set "ZIP_RC=%ERRORLEVEL%"

:zip_check
if not "%ZIP_RC%"=="0" (
    echo WARNING: zip packaging failed ^(exit %ZIP_RC%^)
    echo [WARN] zip skipped
) else (
    for %%F in ("%ZIP_PATH%") do set "ZIP_SIZE=%%~zF"
    echo [ok] %NAME%.zip ^(!ZIP_SIZE! bytes^)
)

echo.
echo RELEASE READY ^(%PROFILE%^)
echo   folder: %DEST%
if exist "%ZIP_PATH%" echo   zip:    %ZIP_PATH%
echo.
echo Contents:
dir /b "%DEST%"

REM Uncomment to open the folder in Explorer after release:
REM start "" "%DEST%"

REM pause
exit /b 0


:fail
echo.
echo ERROR: %ERR%
REM pause
exit /b 1
