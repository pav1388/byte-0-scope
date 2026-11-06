// Byte-0-scope - binary data visualizer
// File: bitpackedreader.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bitpackedreader.h"
#include "pixelmath.h"

#include <limits>

BitPackedReader::BitPackedReader(int bitsPerPixel, const QString &bitOrder,
                                 int rowPaddingBits, const QString &pixelOrder)
    : m_bpp(qBound(1, bitsPerPixel, 8)),
      m_lsbBits(bitOrder.compare(QLatin1String("lsb"), Qt::CaseInsensitive) == 0),
      m_lsbPixels(pixelOrder.compare(QLatin1String("lsb"), Qt::CaseInsensitive) == 0),
      m_rowPadding(qMax(0, rowPaddingBits)),
      m_maxValue((1 << m_bpp) - 1)
{
}

int BitPackedReader::rowBytes(int rowWidth) const
{
    if (rowWidth <= 0)
        return 0;
    const qint64 bytes = PixelMath::bitpackedRowBytes(rowWidth, m_bpp,
                                                      m_rowPadding);
    return (bytes > 0 && bytes <= std::numeric_limits<int>::max())
        ? int(bytes) : 0;
}

unsigned BitPackedReader::valueAt(const uchar *row, qint64 availableBytes,
                                  int index) const
{
    return PixelMath::bitpackedValue(row, availableBytes,
                                     qint64(index) * m_bpp, m_bpp,
                                     m_lsbBits, m_lsbPixels);
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
    const qint64 available = raw.size();
    for (int i = 0; i < rowWidth; ++i)
        out[i] = quint8(valueAt(bytes, available, i));
    return out;
}

QVector<QRgb> BitPackedReader::buildLut(const QVector<QColor> &palette) const
{
    const int size = 1 << m_bpp;
    QVector<QRgb> lut(size);
    for (int i = 0; i < size; ++i) {
        if (i < palette.size()) {
            const QColor &c = palette[i];
            lut[i] = qRgba(c.red(), c.green(), c.blue(), 255);
        } else {
            const int v = int(i * 255 / qMax(1, size - 1));
            lut[i] = qRgba(v, v, v, 255);
        }
    }
    return lut;
}
