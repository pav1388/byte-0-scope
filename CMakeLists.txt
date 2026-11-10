# Byte-0-scope - binary file visualizer
# File: CMakeLists.txt
# Copyright (C) 2026 pav13
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build profiles (BS_PROFILE)
# ---------------------------
#   xp32    Windows XP+ , 32-bit, static Qt 5.6.3, MinGW 4.9.2 (i686), -Os
#   win64   Windows 7+  , 64-bit, static Qt 5.15,  MinGW-w64 8.1.0 (x86_64), -O2
#   native  Linux / macOS / anything else, system (dynamic) Qt5
#   auto    (default) Windows 32-bit toolchain -> xp32,
#                     Windows 64-bit toolchain -> win64,
#                     other systems            -> native
#
# The compiler is selected by PATH / -G, NOT by this file. Use a separate
# build directory per profile, e.g.:
#
#   rem --- XP, 32-bit (MinGW 4.9.2 in PATH) ---
#   cmake -S . -B build-xp32 -G "MinGW Makefiles" ^
#         -DBS_PROFILE=xp32 -DBS_QT_DIR=qt-5.6.3-static
#   cmake --build build-xp32 -j4
#
#   rem --- Win7..11, 64-bit (MinGW-w64 in PATH) ---
#   cmake -S . -B build-win64 -G "MinGW Makefiles" ^
#         -DBS_PROFILE=win64 -DBS_QT_DIR=qt-5.15-static
#   cmake --build build-win64 -j4
#
#   # --- Linux ---
#   cmake -S . -B build-linux -DBS_PROFILE=native
#   cmake --build build-linux -j4

cmake_minimum_required(VERSION 3.10)
project(byte0scope VERSION 0.0 LANGUAGES CXX)

# Let AUTOMOC ignore GENERATED sources (rcc output for translations .qrc).
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()

if(WIN32)
    enable_language(RC)
endif()

# =========================================================================
# Profile selection
# =========================================================================
set(BS_PROFILE "auto" CACHE STRING "Build profile: auto | xp32 | win64 | native")
set_property(CACHE BS_PROFILE PROPERTY STRINGS auto xp32 win64 native)

set(_profile "${BS_PROFILE}")
if(_profile STREQUAL "auto")
    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(_profile xp32)
        else()
            set(_profile win64)
        endif()
    else()
        set(_profile native)
    endif()
endif()

if(NOT (_profile STREQUAL "xp32" OR _profile STREQUAL "win64"
        OR _profile STREQUAL "native"))
    message(FATAL_ERROR
        "Unknown BS_PROFILE='${BS_PROFILE}'. Use: auto | xp32 | win64 | native.")
endif()

# Guard against picking the wrong compiler for the profile.
if(_profile STREQUAL "xp32")
    if(NOT WIN32)
        message(FATAL_ERROR "Profile xp32 is Windows-only.")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR
            "Profile xp32 needs a 32-bit compiler (i686 MinGW), "
            "but a ${CMAKE_SIZEOF_VOID_P}-byte-pointer toolchain was found.")
    endif()
elseif(_profile STREQUAL "win64")
    if(NOT WIN32)
        message(FATAL_ERROR "Profile win64 is Windows-only.")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR
            "Profile win64 needs a 64-bit compiler (x86_64 MinGW-w64), "
            "but a ${CMAKE_SIZEOF_VOID_P}-byte-pointer toolchain was found.")
    endif()
endif()

# Per-profile defaults ----------------------------------------------------
if(_profile STREQUAL "xp32")
    set(_def_qt_dir "qt-5.6.3-static")
    set(_def_winver "0x0501")           # Windows XP
    set(_def_opt    "-Os")
    set(_def_lto    OFF)
elseif(_profile STREQUAL "win64")
    set(_def_qt_dir "qt-5.15-static")
    set(_def_winver "0x0601")           # Windows 7
    set(_def_opt    "-O2")
    set(_def_lto    OFF)
else()
    set(_def_qt_dir "")
    set(_def_winver "")
    set(_def_opt    "-O2")
    set(_def_lto    ON)
endif()

# =========================================================================
# Paths
# =========================================================================
set(BS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH
    "Directory containing project sources")
set(BS_QT_DIR "" CACHE PATH
    "Qt SDK root (contains bin/, lib/, plugins/). Empty = profile default")

if(BS_QT_DIR)
    set(_qt_dir "${BS_QT_DIR}")
else()
    set(_qt_dir "${_def_qt_dir}")
endif()
if(_qt_dir)
    # Relative paths are resolved against the project root.
    get_filename_component(_qt_dir "${_qt_dir}" ABSOLUTE
                           BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(_qt_dir AND EXISTS "${_qt_dir}")
    set(CMAKE_PREFIX_PATH "${_qt_dir};${CMAKE_PREFIX_PATH}")
    message(STATUS "Local Qt SDK:    ${_qt_dir}")
elseif(WIN32)
    message(FATAL_ERROR
        "Local Qt SDK not found at:\n  ${_qt_dir}\n"
        "Place it next to CMakeLists.txt or pass -DBS_QT_DIR=<path>.")
endif()
message(STATUS "Project sources: ${BS_SRC_DIR}")

# =========================================================================
# Version
# =========================================================================
set(_version_file "${CMAKE_CURRENT_SOURCE_DIR}/version.txt")
if(NOT EXISTS "${_version_file}")
    message(FATAL_ERROR "version.txt not found: ${_version_file}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_version_file}")
file(READ "${_version_file}" APP_VERSION_RAW)
string(STRIP "${APP_VERSION_RAW}" APP_VERSION)
add_definitions(-DAPP_VERSION="${APP_VERSION}")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# C++11 on purpose: the same sources must build with MinGW 4.9.2 (xp32).
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# =========================================================================
# Build options
# =========================================================================
if(WIN32)
    set(_def_static ON)
else()
    set(_def_static OFF)
endif()

option(BS_STATIC_QT      "Link against a STATIC Qt SDK (Windows: ON)"      ${_def_static})
option(BS_STATIC_RUNTIME "Statically link libgcc/libstdc++ (MinGW)"        ON)
option(BS_LTO            "Enable link-time optimization for project code"  ${_def_lto})
option(BS_STRIP          "Run strip --strip-all on the executable"         ON)
option(BS_UPX            "Pack the executable with UPX"                    OFF)
option(BS_NO_DEBUG       "Define QT_NO_DEBUG_OUTPUT and QT_NO_DEBUG"       ON)
option(BS_NO_OPENGL      "Set if the Qt SDK was built with -no-opengl"     OFF)
option(BS_DROP_QICO      "Do not link qico (use PNG icon instead)"         OFF)

# 32-bit exe: allow up to 4 GB of address space on 64-bit Windows (2 GB
# otherwise). Matters for mmap of large files. Ignored for 64-bit builds.
option(BS_LARGE_ADDRESS_AWARE
       "32-bit build: link with --large-address-aware"                       ON)

# Report uses of deprecated Qt API (QT_DEPRECATED_WARNINGS). Useful while
# porting; ON by default for everything newer than the xp32 profile.
if(_profile STREQUAL "xp32")
    set(_def_depr OFF)
else()
    set(_def_depr ON)
endif()
option(BS_DEPRECATION_WARNINGS
       "Define QT_DEPRECATED_WARNINGS"                                       ${_def_depr})

set(BS_UPX_MODE "best" CACHE STRING "UPX mode: fast | best | brute")
set_property(CACHE BS_UPX_MODE PROPERTY STRINGS fast best brute)

set(BS_OPT "" CACHE STRING
    "Optimization flag override for MinGW Release (e.g. -O2, -Os). Empty = profile default")
set(BS_WINVER "" CACHE STRING
    "Override _WIN32_WINNT/WINVER (e.g. 0x0501 XP, 0x0601 Win7). Empty = profile default")
set(BS_MARCH "" CACHE STRING
    "Optional -march=<value> for MinGW (e.g. i586 for old CPUs). Empty = compiler default")

if(BS_OPT)
    set(_opt "${BS_OPT}")
else()
    set(_opt "${_def_opt}")
endif()
if(BS_WINVER)
    set(_winver "${BS_WINVER}")
else()
    set(_winver "${_def_winver}")
endif()

if(WIN32 AND _winver)
    add_definitions(-D_WIN32_WINNT=${_winver} -DWINVER=${_winver})
endif()

# =========================================================================
# MinGW: section GC + LTO + optimization + static runtime
# =========================================================================
if(MINGW)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -finput-charset=UTF-8 -fexec-charset=UTF-8")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    if(BS_MARCH)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${BS_MARCH}")
    endif()

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${_opt} -DNDEBUG")

    if(BS_LTO)
        # LTO needs an archiver that can read GCC's slim-LTO bytecode.
        # MinGW 4.9.2 ships gcc-ar built WITHOUT plugin support, so
        # LTO cannot work on xp32 (BS_LTO defaults to OFF there).
        # On win64, gcc-ar works and this path is taken.
        find_program(_BS_GCC_AR     NAMES gcc-ar)
        find_program(_BS_GCC_RANLIB NAMES gcc-ranlib)
        if(_BS_GCC_AR AND _BS_GCC_RANLIB)
            set(CMAKE_AR     "${_BS_GCC_AR}"     CACHE FILEPATH "" FORCE)
            set(CMAKE_RANLIB "${_BS_GCC_RANLIB}" CACHE FILEPATH "" FORCE)
            message(STATUS "LTO archiver: ${CMAKE_AR}")
        else()
            message(WARNING
                "gcc-ar/gcc-ranlib not found; LTO on this MinGW may fail "
                "with 'plugin needed to handle lto object'. "
                "Consider -DBS_LTO=OFF.")
        endif()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
    endif()

    if(BS_STATIC_RUNTIME)
        set(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
    if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4 AND BS_LARGE_ADDRESS_AWARE)
        set(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} -Wl,--large-address-aware")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--gc-sections -Wl,-O2")
    if(BS_LTO)
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE
            "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
    endif()
endif()

if(BS_NO_DEBUG)
    add_definitions(-DQT_NO_DEBUG_OUTPUT -DQT_NO_DEBUG)
endif()

if(BS_DEPRECATION_WARNINGS)
    add_definitions(-DQT_DEPRECATED_WARNINGS)
endif()

# main.cpp checks this macro to skip Q_IMPORT_PLUGIN(QICOPlugin).
if(BS_DROP_QICO)
    add_definitions(-DBS_DROP_QICO)
endif()

# =========================================================================
# Qt5
# =========================================================================
# 5.6 is the minimum (xp32 profile); win64 is expected to use 5.15.
find_package(Qt5 5.6 REQUIRED COMPONENTS Core Gui Widgets)

# Qt root / lib dir (needed for static plugins and translations)
set(QT_LIB_DIR "")
set(QT_ROOT    "")
get_target_property(_qt_core_loc Qt5::Core LOCATION)
if(NOT _qt_core_loc)
    get_target_property(_qt_core_loc Qt5::Core LOCATION_RELEASE)
endif()
if(_qt_core_loc)
    get_filename_component(QT_LIB_DIR "${_qt_core_loc}" DIRECTORY)
    get_filename_component(QT_ROOT    "${QT_LIB_DIR}"   DIRECTORY)
    message(STATUS "Qt lib dir: ${QT_LIB_DIR}")
    message(STATUS "Qt root:    ${QT_ROOT}")
elseif(WIN32)
    message(FATAL_ERROR "Could not locate Qt5Core.")
endif()
message(STATUS "Qt version: ${Qt5Core_VERSION}")

# =========================================================================
# Sources
# =========================================================================
set(BS_SOURCES
    "${BS_SRC_DIR}/main.cpp"
    "${BS_SRC_DIR}/app_fileassoc.cpp"
    "${BS_SRC_DIR}/app_settingsmanager.cpp"
    "${BS_SRC_DIR}/app_thememanager.cpp"
    "${BS_SRC_DIR}/core_bitpackedreader.cpp"
    "${BS_SRC_DIR}/core_bytesource.cpp"
    "${BS_SRC_DIR}/core_demobuilder.cpp"
    "${BS_SRC_DIR}/core_pixeldecoder.cpp"
    "${BS_SRC_DIR}/pal_palettemanager.cpp"
    "${BS_SRC_DIR}/pal_palettesettings.cpp"
    "${BS_SRC_DIR}/render_canvaswidget.cpp"
    "${BS_SRC_DIR}/render_tilemanager.cpp"
    "${BS_SRC_DIR}/ui_aboutdialog.cpp"
    "${BS_SRC_DIR}/ui_byteconverter.cpp"
    "${BS_SRC_DIR}/ui_byteinfo.cpp"
    "${BS_SRC_DIR}/ui_controlspanel.cpp"
    "${BS_SRC_DIR}/ui_hexviewwidget.cpp"
    "${BS_SRC_DIR}/ui_int64spinbox.cpp"
    "${BS_SRC_DIR}/ui_mainwindow.cpp"
    "${BS_SRC_DIR}/ui_mainwindowconfig.cpp"
    "${BS_SRC_DIR}/ui_mainwindowdebug.cpp"
    "${BS_SRC_DIR}/ui_panorama.cpp"
    "${BS_SRC_DIR}/resources/resources.qrc"
)

if(WIN32)
    list(APPEND BS_SOURCES "${BS_SRC_DIR}/resources/win_app.rc")
    add_executable(byte0scope WIN32 ${BS_SOURCES})
else()
    add_executable(byte0scope ${BS_SOURCES})
endif()

target_link_libraries(byte0scope PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)

if(COMMAND qt5_import_plugins)
    qt5_import_plugins(byte0scope
        EXCLUDE
            Qt5::QGifPlugin
            Qt5::QICNSPlugin
            Qt5::QJpegPlugin
            Qt5::QTgaPlugin
            Qt5::QTiffPlugin
            Qt5::QWbmpPlugin
            Qt5::QWebpPlugin
			Qt5::QWindowsVistaStylePlugin
    )
endif()

# =========================================================================
# Localization (Qt Linguist)
# =========================================================================

# Locate Qt5LinguistTools (lupdate / lrelease). Optional — if missing,
# translation targets are simply not created.
find_package(Qt5LinguistTools QUIET)

set(BS_TRANSLATIONS "ru" CACHE STRING
    "Semicolon-separated list of language codes to build .qm for")
set(BS_TS_DIR "${BS_SRC_DIR}/translations")

# Create the translations directory at configure time so lupdate can write
# new .ts files into it without extra steps.
file(MAKE_DIRECTORY "${BS_TS_DIR}")

set(BS_TS_FILES "")
foreach(_lang ${BS_TRANSLATIONS})
    list(APPEND BS_TS_FILES "${BS_TS_DIR}/byte0scope_${_lang}.ts")
endforeach()

# Languages whose .ts file already exists. Only these are compiled by
# lrelease; new languages must first be created by running update_translations.
set(_existing_ts "")
foreach(_ts ${BS_TS_FILES})
    if(EXISTS "${_ts}")
        list(APPEND _existing_ts "${_ts}")
    else()
        message(STATUS "Translation not found yet (run update_translations): ${_ts}")
    endif()
endforeach()

# Directory with qtbase_<lang>.qm: SDK root first, then ask qmake.
set(_qt_tr_dir "")
if(QT_ROOT AND EXISTS "${QT_ROOT}/translations")
    set(_qt_tr_dir "${QT_ROOT}/translations")
elseif(TARGET Qt5::qmake)
    get_target_property(_qmake_exe Qt5::qmake IMPORTED_LOCATION)
    if(_qmake_exe)
        execute_process(
            COMMAND "${_qmake_exe}" -query QT_INSTALL_TRANSLATIONS
            OUTPUT_VARIABLE _qt_tr_dir
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    endif()
endif()

if(Qt5LinguistTools_FOUND AND _existing_ts)
    qt5_add_translation(BS_QM_FILES ${_existing_ts})

    add_custom_target(translations DEPENDS ${BS_QM_FILES})
    add_dependencies(byte0scope translations)

    # --- Embed .qm files into the executable as Qt resources ---
    # A small .qrc is generated at configure time listing every .qm
    # (app translations + Qt base translations). rcc compiles it into
    # the binary, so no loose files are needed next to the .exe.
    set(_tr_qrc "${CMAKE_CURRENT_BINARY_DIR}/byte0scope_translations.qrc")
    file(WRITE "${_tr_qrc}" "<RCC>\n<qresource prefix=\"/translations\">\n")

    # App translations (byte0scope_<lang>.qm, produced by lrelease)
    foreach(_qm ${BS_QM_FILES})
        get_filename_component(_qm_name "${_qm}" NAME)
        file(APPEND "${_tr_qrc}"
            "  <file alias=\"${_qm_name}\">${_qm}</file>\n")
    endforeach()

    # Qt base translations (qtbase_<lang>.qm from the Qt SDK)
    foreach(_lang ${BS_TRANSLATIONS})
        set(_qtbase "${_qt_tr_dir}/qtbase_${_lang}.qm")
        if(_qt_tr_dir AND EXISTS "${_qtbase}")
            file(APPEND "${_tr_qrc}"
                "  <file alias=\"qtbase_${_lang}.qm\">${_qtbase}</file>\n")
        endif()
    endforeach()

    file(APPEND "${_tr_qrc}" "</qresource>\n</RCC>\n")

    qt5_add_resources(BS_TR_RESOURCES "${_tr_qrc}")
    target_sources(byte0scope PRIVATE ${BS_TR_RESOURCES})

    # Make sure rcc does not run before lrelease has produced the .qm files.
    set_property(SOURCE "${_tr_qrc}" PROPERTY OBJECT_DEPENDS "${BS_QM_FILES}")

    message(STATUS "Translations embedded into the executable via byte0scope_translations.qrc")
endif()

# lupdate / lrelease drivers. Run manually:
#   cmake --build build --target update_translations
#   cmake --build build --target release_translations
if(Qt5LinguistTools_FOUND)
    # Files to skip when feeding sources to lupdate:
    #   - .qrc / .rc — not C++ sources.
    #   - thememanager.cpp — may contain a C++11 raw string literal for QSS
    #     which lupdate 5.6.3 cannot parse. If it has no tr() strings anyway,
    #     skipping it is safe. Remove the skip below if you add tr() there
    #     (and convert the raw string to a regular literal, or use a newer
    #     lupdate from a separate Qt tools package).
    set(_lupdate_sources "")
    foreach(_src ${BS_SOURCES})
        if(_src MATCHES "\\.(qrc|rc)$")
            continue()
        endif()
        if(_src MATCHES "thememanager\\.cpp$")
            continue()
        endif()
        list(APPEND _lupdate_sources "${_src}")
    endforeach()

    # CORRECT SYNTAX: lupdate [options] source-files -ts ts-files
    # The -ts flag MUST come after the source files. Without it lupdate
    # warns "no TS files specified" and produces only diagnostics.
    add_custom_target(update_translations
        COMMAND Qt5::lupdate
                -locations relative
                ${_lupdate_sources}
                -ts ${BS_TS_FILES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Updating .ts translation files via lupdate"
        VERBATIM)

    add_custom_target(release_translations
        COMMAND Qt5::lrelease ${BS_TS_FILES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Compiling .ts into .qm via lrelease"
        VERBATIM)
endif()

# =========================================================================
# Static Qt components (Windows / MinGW only)
# =========================================================================
if(WIN32)
    function(link_static_qt_lib name path)
        if(EXISTS "${path}")
            target_link_libraries(byte0scope PRIVATE "${path}")
            message(STATUS "  + ${name}: ${path}")
        else()
            message(STATUS "  - ${name}: not found, skipped")
        endif()
    endfunction()
endif()

if(WIN32 AND BS_STATIC_QT)
    message(STATUS "Static Qt components (profile ${_profile}):")

    if(_profile STREQUAL "xp32")
        # --- Qt 5.6.3: libraries are linked by hand, in this order ---
        link_static_qt_lib("PCRE"            "${QT_LIB_DIR}/libqtpcre.a")
        link_static_qt_lib("HarfBuzz"        "${QT_LIB_DIR}/libqtharfbuzzng.a")
        link_static_qt_lib("qwindows"        "${QT_ROOT}/plugins/platforms/libqwindows.a")
        if(NOT BS_DROP_QICO)
            link_static_qt_lib("qico"        "${QT_ROOT}/plugins/imageformats/libqico.a")
        endif()
        link_static_qt_lib("PlatformSupport" "${QT_LIB_DIR}/libQt5PlatformSupport.a")
        link_static_qt_lib("FreeType"        "${QT_LIB_DIR}/libqtfreetype.a")

        set(_sys_libs shell32 ws2_32 winmm imm32 psapi user32 gdi32)
    else()
        # --- Qt 5.15 (win64): prefer imported plugin targets; they pull in
        #     their own static dependencies (.prl). ---
        if(TARGET Qt5::QWindowsIntegrationPlugin)
            target_link_libraries(byte0scope PRIVATE Qt5::QWindowsIntegrationPlugin)
            message(STATUS "  + qwindows: Qt5::QWindowsIntegrationPlugin")
            if(NOT BS_DROP_QICO)
                if(TARGET Qt5::QICOPlugin)
                    target_link_libraries(byte0scope PRIVATE Qt5::QICOPlugin)
                    message(STATUS "  + qico:     Qt5::QICOPlugin")
                else()
                    message(STATUS "  - qico: Qt5::QICOPlugin not found, skipped")
                endif()
            endif()
        else()
            # Fallback: manual list (names as in Qt 5.15 static builds).
            message(STATUS "  (plugin targets not found, linking .a files manually)")
            link_static_qt_lib("qwindows"    "${QT_ROOT}/plugins/platforms/libqwindows.a")
            if(NOT BS_DROP_QICO)
                link_static_qt_lib("qico"    "${QT_ROOT}/plugins/imageformats/libqico.a")
            endif()
            link_static_qt_lib("EventDispatcher" "${QT_LIB_DIR}/libQt5EventDispatcherSupport.a")
            link_static_qt_lib("FontDatabase"    "${QT_LIB_DIR}/libQt5FontDatabaseSupport.a")
            link_static_qt_lib("ThemeSupport"    "${QT_LIB_DIR}/libQt5ThemeSupport.a")
            link_static_qt_lib("Accessibility"   "${QT_LIB_DIR}/libQt5AccessibilitySupport.a")
            link_static_qt_lib("UIAutomation"    "${QT_LIB_DIR}/libQt5WindowsUIAutomationSupport.a")
            link_static_qt_lib("HarfBuzz"        "${QT_LIB_DIR}/libqtharfbuzz.a")
            link_static_qt_lib("FreeType"        "${QT_LIB_DIR}/libqtfreetype.a")
            link_static_qt_lib("libpng"          "${QT_LIB_DIR}/libqtlibpng.a")
            link_static_qt_lib("PCRE2"           "${QT_LIB_DIR}/libqtpcre2.a")
        endif()

        set(_sys_libs shell32 ws2_32 winmm imm32 psapi user32 gdi32
                      advapi32 ole32 oleaut32 uuid version dwmapi uxtheme
                      winspool comdlg32 netapi32 userenv mpr wtsapi32)
    endif()

    if(NOT BS_NO_OPENGL)
        list(APPEND _sys_libs opengl32)
    endif()
    target_link_libraries(byte0scope PRIVATE ${_sys_libs})

    if(MINGW AND NOT EXISTS "${QT_ROOT}/plugins/platforms/libqwindows.a")
        message(FATAL_ERROR
            "Platform plugin not found:\n  ${QT_ROOT}/plugins/platforms/libqwindows.a\n"
            "Is this a static Qt SDK? (or pass -DBS_STATIC_QT=OFF for dynamic Qt)")
    endif()
elseif(WIN32)
    # Dynamic Qt on Windows: only the system libraries used by project code.
    target_link_libraries(byte0scope PRIVATE shell32 psapi user32 gdi32)
endif()

# =========================================================================
# Post-build: strip + UPX
# =========================================================================
if(MINGW)
    if(BS_STRIP AND CMAKE_STRIP)
        add_custom_command(TARGET byte0scope POST_BUILD
            COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:byte0scope>
            COMMENT "Stripping byte0scope.exe")
    endif()

    if(BS_UPX)
        find_program(UPX_EXECUTABLE NAMES upx upx.exe)
        if(NOT UPX_EXECUTABLE)
            message(WARNING "UPX not found in PATH; packing skipped.")
        else()
            if(BS_UPX_MODE STREQUAL "fast")
                set(_upx_opts --fast)
            elseif(BS_UPX_MODE STREQUAL "brute")
                set(_upx_opts --best --ultra-brute)
            else()
                set(_upx_opts --best --lzma)
            endif()
            message(STATUS "UPX: ${UPX_EXECUTABLE} (mode=${BS_UPX_MODE})")
            add_custom_command(TARGET byte0scope POST_BUILD
                COMMAND ${UPX_EXECUTABLE} ${_upx_opts} $<TARGET_FILE:byte0scope>
                COMMENT "UPX compressing byte0scope.exe (${BS_UPX_MODE})")
        endif()
    endif()
endif()

# =========================================================================
# Summary
# =========================================================================
message(STATUS "-------------------------------------------------")
message(STATUS "Byte-0-scope ${APP_VERSION}")
message(STATUS "  Profile:      ${_profile}")
message(STATUS "  Pointer size: ${CMAKE_SIZEOF_VOID_P} bytes")
message(STATUS "  Static Qt:    ${BS_STATIC_QT}")
if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND BS_LARGE_ADDRESS_AWARE)
    message(STATUS "  Large address aware: ON")
endif()
if(WIN32)
    message(STATUS "  _WIN32_WINNT: ${_winver}")
endif()
if(MINGW)
    message(STATUS "  Optimization: ${_opt}  LTO: ${BS_LTO}")
endif()
message(STATUS "-------------------------------------------------")