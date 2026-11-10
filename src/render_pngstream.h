// Byte-0-scope - binary data visualizer
// File: render_pngstream.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QtGlobal>

#include <functional>

namespace PngStream {

// Заполняет одну строку в формате RGBA (не premultiplied), 4 байта на пиксель.
// Возвращает false для отмены экспорта.
using RowProvider = std::function<bool(int rowIndex, quint8 *rowData)>;

// Пишет PNG (color type 6, 8 бит/канал, без интерлейса) потоково.
// Память: O(width), весь образ никогда не держится в RAM.
bool writeRgba(const QString &path, int width, int height,
               const RowProvider &provideRow, QString *errorMessage);

} // namespace PngStream