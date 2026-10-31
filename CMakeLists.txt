# Byte-0-scope — binary file visualizer
# File: CMakeLists.txt
# Copyright (C) 2026 pav13
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.5)
project(byte0scope VERSION 0.0 LANGUAGES CXX)

if(WIN32)
    enable_language(RC)
endif()

# =========================================================================
# Paths
# =========================================================================
set(BS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH
    "Directory containing project sources")
set(BS_QT_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/qt" CACHE PATH
    "Local Qt SDK root (contains bin/, lib/, plugins/)")

if(EXISTS "${BS_QT_DIR}")
    set(CMAKE_PREFIX_PATH "${BS_QT_DIR};${CMAKE_PREFIX_PATH}")
    message(STATUS "Local Qt SDK:    ${BS_QT_DIR}")
elseif(WIN32)
    message(FATAL_ERROR
        "Local Qt SDK not found at:\n  ${BS_QT_DIR}\n"
        "Place it next to CMakeLists.txt or pass -DBS_QT_DIR=<path>.")
endif()
message(STATUS "Project sources: ${BS_SRC_DIR}")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" APP_VERSION_RAW)
string(STRIP "${APP_VERSION_RAW}" APP_VERSION)
add_definitions(-DAPP_VERSION="${APP_VERSION}")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

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
option(BS_LTO       "Enable link-time optimization for project code"  ON)
option(BS_STRIP     "Run strip --strip-all on the executable"         ON)
option(BS_UPX       "Pack the executable with UPX"                    OFF)
option(BS_NO_DEBUG  "Define QT_NO_DEBUG_OUTPUT and QT_NO_DEBUG"       ON)
option(BS_NO_OPENGL "Set if the Qt SDK was built with -no-opengl"     OFF)
option(BS_DROP_QICO "Do not link qico (use PNG icon instead)"         OFF)

set(BS_UPX_MODE "best" CACHE STRING "UPX mode: fast | best | brute")
set_property(CACHE BS_UPX_MODE PROPERTY STRINGS fast best brute)

# =========================================================================
# MinGW: section GC + LTO + Os + static runtime
# =========================================================================
if(MINGW)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -finput-charset=UTF-8 -fexec-charset=UTF-8")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -DNDEBUG")
    if(BS_LTO)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
    endif()

    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -s")
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

# =========================================================================
# Qt5
# =========================================================================
find_package(Qt5 5.6 REQUIRED COMPONENTS Core Gui Widgets)

if(WIN32)
    get_target_property(_qt_core_loc Qt5::Core LOCATION)
    if(NOT _qt_core_loc)
        message(FATAL_ERROR "Could not locate Qt5Core.")
    endif()
    get_filename_component(QT_LIB_DIR "${_qt_core_loc}" DIRECTORY)
    get_filename_component(QT_ROOT   "${QT_LIB_DIR}" DIRECTORY)
    message(STATUS "Qt lib dir: ${QT_LIB_DIR}")
    message(STATUS "Qt root:    ${QT_ROOT}")
endif()

# =========================================================================
# Sources
# =========================================================================
set(BS_SOURCES
    "${BS_SRC_DIR}/main.cpp"
    "${BS_SRC_DIR}/aboutdialog.cpp"
    "${BS_SRC_DIR}/bitpackedreader.cpp"
    "${BS_SRC_DIR}/byteinfo.cpp"
    "${BS_SRC_DIR}/bytesource.cpp"
    "${BS_SRC_DIR}/canvaswidget.cpp"
    "${BS_SRC_DIR}/controlspanel.cpp"
    "${BS_SRC_DIR}/demobuilder.cpp"
    "${BS_SRC_DIR}/fileassoc.cpp"
    "${BS_SRC_DIR}/palettemanager.cpp"
    "${BS_SRC_DIR}/palettesettings.cpp"
    "${BS_SRC_DIR}/panoramawidget.cpp"
    "${BS_SRC_DIR}/pixeldecoder.cpp"
    "${BS_SRC_DIR}/pngwriter.cpp"
    "${BS_SRC_DIR}/settingsmanager.cpp"
    "${BS_SRC_DIR}/thememanager.cpp"
    "${BS_SRC_DIR}/tilemanager.cpp"
    "${BS_SRC_DIR}/mainwindow.cpp"
    "${BS_SRC_DIR}/resources/resources.qrc"
)

if(WIN32)
    list(APPEND BS_SOURCES "${BS_SRC_DIR}/resources/win_app.rc")
    add_executable(byte0scope WIN32 ${BS_SOURCES})
else()
    add_executable(byte0scope ${BS_SOURCES})
endif()

target_link_libraries(byte0scope PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)

# =========================================================================
# Localization (Qt Linguist)
# =========================================================================

# Locate Qt5LinguistTools (lupdate / lrelease). Optional — if missing,
# translation targets are simply not created.
find_package(Qt5LinguistTools QUIET)

set(BS_TRANSLATIONS "en" CACHE STRING
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
        set(_qtbase "${QT_ROOT}/translations/qtbase_${_lang}.qm")
        if(EXISTS "${_qtbase}")
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
# Static Qt components (Windows/MinGW-only paths)
# =========================================================================
if(WIN32)
    function(link_static_qt_prv name path)
        if(EXISTS "${path}")
            target_link_libraries(byte0scope PRIVATE "${path}")
            message(STATUS "  + ${name}: ${path}")
        else()
            message(STATUS "  - ${name}: not found, skipped")
        endif()
    endfunction()

    message(STATUS "Static Qt components:")
    link_static_qt_prv("PCRE"            "${QT_LIB_DIR}/libqtpcre.a")
    link_static_qt_prv("HarfBuzz"        "${QT_LIB_DIR}/libqtharfbuzzng.a")
    link_static_qt_prv("qwindows"        "${QT_ROOT}/plugins/platforms/libqwindows.a")
    if(NOT BS_DROP_QICO)
        link_static_qt_prv("qico"        "${QT_ROOT}/plugins/imageformats/libqico.a")
    endif()
    link_static_qt_prv("PlatformSupport" "${QT_LIB_DIR}/libQt5PlatformSupport.a")
    link_static_qt_prv("FreeType"        "${QT_LIB_DIR}/libqtfreetype.a")
endif()

if(WIN32)
    set(_sys_libs shell32 ws2_32 winmm imm32 psapi user32 gdi32)
    if(NOT BS_NO_OPENGL)
        list(APPEND _sys_libs opengl32)
    endif()
    target_link_libraries(byte0scope PRIVATE ${_sys_libs})
endif()

if(MINGW AND NOT EXISTS "${QT_ROOT}/plugins/platforms/libqwindows.a")
    message(FATAL_ERROR
        "Platform plugin not found:\n  ${QT_ROOT}/plugins/platforms/libqwindows.a")
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