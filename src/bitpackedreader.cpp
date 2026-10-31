// Byte-0-scope - binary data visualizer
// File: bitpackedreader.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bitpackedreader.h"

BitPackedReader::BitPackedReader(int bitsPerPixel, const QString &bitOrder,
                                 int rowPaddingBits, const QString &pixelOrder)
    : m_bpp(qBound(1, bitsPerPixel, 8)),
      m_bitOrder(bitOrder.toLower()),
      m_pixelOrder(pixelOrder.toLower()),
      m_rowPadding(qMax(0, rowPaddingBits)),
      m_maxValue((1 << bitsPerPixel) - 1)
{
}

int BitPackedReader::rowBytes(int rowWidth) const
{
    const int totalBits = rowWidth * m_bpp + m_rowPadding;
    return (totalBits + 7) / 8;
}

QVector<quint8> BitPackedReader::extractRow(const QByteArray &raw, int rowWidth) const
{
    QVector<quint8> out(rowWidth, 0);
    if (raw.isEmpty() || rowWidth <= 0)
        return out;

    const uchar *bytes = reinterpret_cast<const uchar *>(raw.constData());
    const int nBytes = raw.size();
    const int neededBits = rowWidth * m_bpp;

    QVector<quint8> bits(neededBits, 0);
    int bitPos = 0;
    for (int i = 0; i < nBytes && bitPos < neededBits; ++i) {
        const uchar byte = bytes[i];
        for (int k = 0; k < 8 && bitPos < neededBits; ++k) {
            const int bit = (m_bitOrder == QLatin1String("lsb"))
                ? ((byte >> k) & 1)
                : ((byte >> (7 - k)) & 1);
            bits[bitPos++] = quint8(bit);
        }
    }

    // Упаковка битов в пиксели
    if (m_bpp == 1) {
        for (int i = 0; i < rowWidth; ++i)
            out[i] = bits[i];
        return out;
    }

    for (int i = 0; i < rowWidth; ++i) {
        int value = 0;
        if (m_pixelOrder == QLatin1String("lsb")) {
            for (int k = 0; k < m_bpp; ++k)
                value |= (bits[i * m_bpp + k] & 1) << k;
        } else {
            for (int k = 0; k < m_bpp; ++k)
                value = (value << 1) | (bits[i * m_bpp + k] & 1);
        }
        out[i] = quint8(value);
    }
    return out;
}

QVector<QRgb> BitPackedReader::buildLut(const QVector<QColor> &palette) const
{
    const int size = 1 << m_bpp;
    QVector<QRgb> lut(size);
    for (int i = 0; i < size; ++i) {
        if (!palette.isEmpty() && i < palette.size()) {
            const QColor &c = palette[i];
            lut[i] = qRgba(c.red(), c.green(), c.blue(), 255);
        } else {
            const int v = int(i * 255 / qMax(1, size - 1));
            lut[i] = qRgba(v, v, v, 255);
        }
    }
    return lut;
}