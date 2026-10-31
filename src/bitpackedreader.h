// Byte-0-scope - binary data visualizer
// File: bitpackedreader.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QColor>
#include <QString>
#include <QVector>

// Извлекает N-битные пиксели (N = 1..8) из байтового потока.
// Порт BitPackedReader без numpy.
class BitPackedReader {
public:
    BitPackedReader(int bitsPerPixel, const QString &bitOrder,
                    int rowPaddingBits, const QString &pixelOrder);

    int bpp() const      { return m_bpp; }
    int maxValue() const { return m_maxValue; }

    // Полный размер строки в байтах (с учётом паддинга).
    int rowBytes(int rowWidth) const;

    // Извлечь одну строку. Возвращает массив длиной rowWidth.
    QVector<quint8> extractRow(const QByteArray &raw, int rowWidth) const;

    // Построить LUT [0..maxValue] -> ARGB. Если palette пуст — grayscale.
    QVector<QRgb> buildLut(const QVector<QColor> &palette) const;

private:
    int     m_bpp;
    QString m_bitOrder;
    QString m_pixelOrder;
    int     m_rowPadding;
    int     m_maxValue;
};