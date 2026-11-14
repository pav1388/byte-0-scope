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

// Декодер многобайтовых пикселей.
class Decoder {
public:
    Decoder(const QString &paletteId, const QString &bitOrder,
            const QString &byteOrder,
            const QVariantMap &params = QVariantMap());

    PixelMath::PixelFormat format() const { return m_format; }

    QRgb decode(const uchar *pixel, int size) const;

private:
    // Приводит байты пикселя к big-endian и MSB-first; возвращает их число.
    int normalize(const uchar *pixel, int size, uchar out[8]) const;

    PixelMath::PixelFormat m_format;
    bool    m_littleEndian;
    bool    m_reverseBits;
    bool    m_signed;

    QString m_fileFormat;
    QString m_fileEndianness;
    int     m_fileRecordSize;
};

QString describe(const QByteArray &pixelData, const QString &paletteId,
                 const QString &bitOrder, const QString &byteOrder,
                 const QVariantMap &params = QVariantMap());

QColor interpretPaletteRecord(const QByteArray &record, const QString &format,
                              int recordSize, const QString &endianness);

} // namespace PixelDecoder
