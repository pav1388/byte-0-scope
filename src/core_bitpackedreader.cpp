// Byte-0-scope - binary data visualizer
// File: core_bitpackedreader.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core_bitpackedreader.h"

BitPackedReader::BitPackedReader(int bitsPerPixel, const QString &bitOrder,
                                 const QString &pixelOrder)
    : m_bpp(qBound(1, bitsPerPixel, 8)),
      m_lsbBits(bitOrder.compare(QLatin1String("lsb"), Qt::CaseInsensitive) == 0),
      m_lsbPixels(pixelOrder.compare(QLatin1String("lsb"), Qt::CaseInsensitive) == 0)
{
}

QVector<QRgb> BitPackedReader::buildLut(const QVector<QColor> &palette) const
{
    const int size = 1 << m_bpp;
    QVector<QRgb> lut(size);
    for (int i = 0; i < size; ++i) {
        if (i < palette.size()) {
            lut[i] = palette[i].rgb();
        } else {
            const int v = i * 255 / (size - 1);
            lut[i] = qRgb(v, v, v);
        }
    }
    return lut;
}
