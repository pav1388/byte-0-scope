// Byte-0-scope - binary data visualizer
// File: pixeldecoder.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QColor>
#include <QString>
#include <QVariantMap>

namespace PixelDecoder {

// Реверс битов в байте. Используется при bit_order = "lsb" для bpp >= 8.
inline quint8 reverseBits8(quint8 b) noexcept
{
    b = quint8((b & 0xF0u) >> 4) | quint8((b & 0x0Fu) << 4);
    b = quint8((b & 0xCCu) >> 2) | quint8((b & 0x33u) << 2);
    b = quint8((b & 0xAAu) >> 1) | quint8((b & 0x55u) << 1);
    return b;
}

QRgb decodeArgb(const QByteArray &pixelData, const QString &paletteId,
                const QString &bitOrder, const QString &byteOrder,
                const QVariantMap &params = QVariantMap());

QRgb decodeArgbRaw(const uchar *pixelData, int size,
                   const QString &paletteId,
                   const QString &bitOrder, const QString &byteOrder,
                   const QVariantMap &params = QVariantMap());

QString describe(const QByteArray &pixelData, const QString &paletteId,
                 const QString &bitOrder, const QString &byteOrder,
                 const QVariantMap &params = QVariantMap());

QColor interpretPaletteRecord(const QByteArray &record, const QString &format,
                              int recordSize, const QString &endianness);

bool isMultibytePalette(const QString &paletteId);

} // namespace PixelDecoder