// Byte-0-scope - binary data visualizer
// File: core_bitpackedreader.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QColor>
#include <QString>
#include <QVector>

// Извлекает N-битные пиксели (N = 1..8) из байтового потока.
class BitPackedReader {
public:
    BitPackedReader(int bitsPerPixel, const QString &bitOrder,
                    int rowPaddingBits, const QString &pixelOrder);

    int bpp() const      { return m_bpp; }
    int maxValue() const { return m_maxValue; }
    int rowBytes(int rowWidth) const;
    unsigned valueAt(const uchar *row, qint64 availableBytes, int index) const;
    QVector<quint8> extractRow(const QByteArray &raw, int rowWidth) const;
    QVector<QRgb> buildLut(const QVector<QColor> &palette) const;

private:
    int  m_bpp;
    bool m_lsbBits;
    bool m_lsbPixels;
    int  m_rowPadding;
    int  m_maxValue;
};
