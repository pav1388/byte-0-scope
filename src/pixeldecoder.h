// Byte-0-scope - binary data visualizer
// File: pixeldecoder.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "pixelmath.h"

#include <QByteArray>
#include <QColor>
#include <QString>
#include <QVariantMap>

namespace PixelDecoder {

// Реверс битов в байте. Используется при bit_order = "lsb".
inline quint8 reverseBits8(quint8 b) noexcept
{
    return PixelMath::reverseBits8(b);
}

// Декодер многобайтовых пикселей.
//
// Разбор идентификатора палитры, порядка байтов/битов и параметров делается
// один раз в конструкторе, поэтому decode() подходит для горячего цикла
// (раньше каждый пиксель заново сравнивал строки и доставал значения из
// QVariantMap). Объект не изменяется после создания — decode() можно
// вызывать из нескольких потоков.
class Decoder {
public:
    Decoder(const QString &paletteId, const QString &bitOrder,
            const QString &byteOrder,
            const QVariantMap &params = QVariantMap());

    PixelMath::PixelFormat format() const   { return m_format; }
    bool                   isSigned() const { return m_signed; }

    // Приводит байты пикселя к «нормализованному» виду: big-endian и
    // нормальный порядок битов. Возвращает число записанных байтов (<= 8).
    int normalize(const uchar *pixel, int size, uchar out[8]) const;

    QRgb decode(const uchar *pixel, int size) const;

private:
    PixelMath::PixelFormat m_format;
    bool    m_littleEndian;
    bool    m_reverseBits;
    bool    m_signed;

    // Параметры палитры from_file (используются только для PF_FromFile).
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
