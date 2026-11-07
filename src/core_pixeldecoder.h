// Byte-0-scope - binary data visualizer
// File: core_pixeldecoder.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core_pixelmath.h"

#include <QByteArray>
#include <QColor>
#include <QString>
#include <QVariantMap>

namespace PixelDecoder {

inline quint8 reverseBits8(quint8 b) noexcept
{
    return PixelMath::reverseBits8(b);
}

// Декодер многобайтовых пикселей.
class Decoder {
public:
    Decoder(const QString &paletteId, const QString &bitOrder,
            const QString &byteOrder,
            const QVariantMap &params = QVariantMap());

    PixelMath::PixelFormat format() const   { return m_format; }
    bool                   isSigned() const { return m_signed; }

    int normalize(const uchar *pixel, int size, uchar out[8]) const;

    QRgb decode(const uchar *pixel, int size) const;

private:
    PixelMath::PixelFormat m_format;
    bool    m_littleEndian;
    bool    m_reverseBits;
    bool    m_signed;

    QString m_fileFormat;
    QString m_fileEndianness;
    int     m_fileRecordSize;
};

PixelMath::PixelFormat parseFormat(const QString &paletteId);

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
