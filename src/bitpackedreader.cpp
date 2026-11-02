// Byte-0-scope - binary data visualizer
// File: bitpackedreader.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bitpackedreader.h"

#include <limits>

BitPackedReader::BitPackedReader(int bitsPerPixel, const QString &bitOrder,
                                 int rowPaddingBits, const QString &pixelOrder)
    : m_bpp(qBound(1, bitsPerPixel, 8)),
      m_bitOrder(bitOrder.toLower()),
      m_pixelOrder(pixelOrder.toLower()),
      m_rowPadding(qMax(0, rowPaddingBits)),
      m_maxValue((1 << qBound(1, bitsPerPixel, 8)) - 1)
{
}

int BitPackedReader::rowBytes(int rowWidth) const
{
    if (rowWidth <= 0)
        return 0;
    const qint64 totalBits = qint64(rowWidth) * m_bpp + m_rowPadding;
    const qint64 bytes = (totalBits + 7) / 8;
    return (bytes > 0 && bytes <= std::numeric_limits<int>::max())
        ? int(bytes) : 0;
}

QVector<quint8> BitPackedReader::extractRow(const QByteArray &raw, int rowWidth) const
{
    // The application width control is capped at 65536. Reject unreasonable
    // direct API calls rather than attempting multi-gigabyte allocations.
    if (rowWidth <= 0 || rowWidth > 65536)
        return QVector<quint8>();

    QVector<quint8> out(rowWidth, 0);
    if (raw.isEmpty())
        return out;

    const uchar *bytes = reinterpret_cast<const uchar *>(raw.constData());
    const qint64 availableBits = qint64(raw.size()) * 8;
    for (int i = 0; i < rowWidth; ++i) {
        const qint64 firstBit = qint64(i) * m_bpp;
        int value = 0;
        for (int k = 0; k < m_bpp; ++k) {
            const qint64 bitIndex = firstBit + k;
            int bit = 0;
            if (bitIndex < availableBits) {
                const qint64 byteIndex = bitIndex >> 3;
                const int bitInByte = int(bitIndex & 7);
                const uchar byte = bytes[byteIndex];
                bit = (m_bitOrder == QLatin1String("lsb"))
                    ? ((byte >> bitInByte) & 1)
                    : ((byte >> (7 - bitInByte)) & 1);
            }
            if (m_pixelOrder == QLatin1String("lsb"))
                value |= bit << k;
            else
                value = (value << 1) | bit;
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
