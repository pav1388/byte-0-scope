// Byte-0-scope - binary data visualizer
// File: render_pngstream.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include <functional>

namespace PngStream {

enum class ColorType { Indexed, Rgb, Rgba };

// Формат строки PNG.
//   Indexed: bitDepth 1/2/4/8, palette (1..2^bitDepth записей), строка -
//            упакованные индексы, MSB первым.
//   Rgb:     8 бит/канал, 3 байта на пиксель.
//   Rgba:    8 бит/канал, 4 байта на пиксель (не premultiplied).
struct Format {
    ColorType     type = ColorType::Rgba;
    int           bitDepth = 8;
    QVector<QRgb> palette;   // только для Indexed; альфа игнорируется
};

// Размер строки в байтах без байта фильтра; <= 0, если формат некорректен.
qint64 rowBytes(const Format &format, int width);

// Заполняет одну строку в формате Format; rowBytes() байт.
// Возвращает false для отмены экспорта.
using RowProvider = std::function<bool(int rowIndex, quint8 *rowData)>;

// Пишет PNG (без интерлейса) потоково.
// Память: O(width), весь образ никогда не держится в RAM.
bool write(const QString &path, const Format &format, int width, int height,
           const RowProvider &provideRow, QString *errorMessage);

} // namespace PngStream
