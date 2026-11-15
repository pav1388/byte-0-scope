@echo off
REM pav13 (2024)
REM Byte-0-scope: menu for the scripts
setlocal EnableExtensions
set mycurdir=%~dp0

set headline=Byte-0-scope build menu
set menu_option_1=[1]...build           (Win7+ 64, Qt 5.15.2, MinGW64 8.1.0)
set menu_option_2=[2]...clean + build
set menu_option_3=[3]...release + zip
set menu_option_4=[4]...translations update
set menu_option_5=[5]...translations release
set menu_option_6=   [6]...build        (WinXP+ 32, Qt 5.6.3, MinGW32 4.9.2)
set menu_option_7=   [7]...clean + build
set menu_option_8=   [8]...release + zip
set menu_option_9=   [9]...translations update
set menu_option_10=   [0]...translations release
set menu_option_11=[Q]...EXIT
set menu_option_max=11
set force_clear_screen=0

set option=
set error=0
title %headline%
for /F %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"

:start
cd /d "%mycurdir%"
if %force_clear_screen% neq 0 if %error% equ 0 cls
echo.
echo   %ESC%[36m%headline%%ESC%[0m
echo      ~ Available Options ~
echo.
set i=1
:cycle
set /a x=%i% %% 2
if %x% equ 0 (call echo   %ESC%[33m%%menu_option_%i%%%%ESC%[0m) else (call echo   %ESC%[32m%%menu_option_%i%%%%ESC%[0m)
if %i% neq %menu_option_max% set /a i=i+1& goto cycle
echo.
goto err

:input
choice /C 1234567890Q /N /M "Select Option: "
set option=%errorlevel%
echo.
echo %ESC%[7;45m                    ~ program start ~                    %ESC%[0m
goto option%option%

:err
if not defined option goto input
if %error% equ 0 call echo %ESC%[7;32m OK :) %%menu_option_%option%%%%ESC%[0m
if %error% neq 0 call echo %ESC%[7;31m ERROR %error% :( %%menu_option_%option%%%%ESC%[0m
goto input

:option1
call "%mycurdir%build-win64.bat"
goto end

:option2
call "%mycurdir%clean-build-win64.bat"
goto end

:option3
call "%mycurdir%release-win64.bat"
goto end

:option4
call "%mycurdir%translations-update-win64.bat"
goto end

:option5
call "%mycurdir%translations-release-win64.bat"
goto end

:option6
call "%mycurdir%build-xp32.bat"
goto end

:option7
call "%mycurdir%clean-build-xp32.bat"
goto end

:option8
call "%mycurdir%release-xp32.bat"
goto end

:option9
call "%mycurdir%translations-update-xp32.bat"
goto end

:option10
call "%mycurdir%translations-release-xp32.bat"
goto end

:option11
exit /b 0

:end
set error=%errorlevel%
echo %ESC%[7;45m                     ~ program end ~                     %ESC%[0m
goto start