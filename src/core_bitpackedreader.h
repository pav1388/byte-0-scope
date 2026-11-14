// Byte-0-scope - binary data visualizer
// File: core_bitpackedreader.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core_pixelmath.h"

#include <QColor>
#include <QString>
#include <QVector>

// Извлекает N-битные пиксели (N = 1..8) из байтового потока.
class BitPackedReader {
public:
    BitPackedReader(int bitsPerPixel, const QString &bitOrder,
                    const QString &pixelOrder);

    // Значение пикселя с номером index в строке из availableBytes байт.
    unsigned valueAt(const uchar *row, qint64 availableBytes, int index) const
    {
        return PixelMath::bitpackedValue(row, availableBytes,
                                         qint64(index) * m_bpp, m_bpp,
                                         m_lsbBits, m_lsbPixels);
    }

    // Таблица цветов на 2^bpp значений; недостающие записи - оттенки серого.
    QVector<QRgb> buildLut(const QVector<QColor> &palette) const;

private:
    int  m_bpp;
    bool m_lsbBits;
    bool m_lsbPixels;
};
