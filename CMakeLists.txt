# Byte-o-scope — binary file visualizer
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
	"${BS_SRC_DIR}/fileassoc.cpp"
    "${BS_SRC_DIR}/bytesource.cpp"
    "${BS_SRC_DIR}/pixeldecoder.cpp"
    "${BS_SRC_DIR}/pngwriter.cpp"
    "${BS_SRC_DIR}/bitpackedreader.cpp"
    "${BS_SRC_DIR}/thememanager.cpp"
    "${BS_SRC_DIR}/settingsmanager.cpp"
    "${BS_SRC_DIR}/palettemanager.cpp"
    "${BS_SRC_DIR}/tilemanager.cpp"
    "${BS_SRC_DIR}/canvaswidget.cpp"
    "${BS_SRC_DIR}/byteinfo.cpp"
    "${BS_SRC_DIR}/panoramawidget.cpp"
    "${BS_SRC_DIR}/palettesettings.cpp"
    "${BS_SRC_DIR}/controlspanel.cpp"
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