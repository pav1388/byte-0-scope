// Byte-0-scope - binary data visualizer
// File: appversion.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Версия задаётся из проекта (DEFINES += APP_VERSION=...).
// Раньше значение по умолчанию было продублировано в main.cpp,
// mainwindow.cpp ("er-dev") и aboutdialog.cpp ("dev").
#ifndef APP_VERSION
#  define APP_VERSION "er-dev"
#endif
