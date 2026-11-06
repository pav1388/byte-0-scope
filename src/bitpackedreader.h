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
//
// Это единственная реализация извлечения bit-packed пикселей: extractRow()
// и тайловый рендерер (TileManager) используют valueAt().
class BitPackedReader {
public:
    BitPackedReader(int bitsPerPixel, const QString &bitOrder,
                    int rowPaddingBits, const QString &pixelOrder);

    int bpp() const      { return m_bpp; }
    int maxValue() const { return m_maxValue; }

    // Полный размер строки в байтах (с учётом паддинга).
    int rowBytes(int rowWidth) const;

    // Значение пикселя с номером index в строке row. availableBytes — число
    // доступных байтов строки; недостающие биты читаются как нули.
    // row может быть nullptr (все пиксели равны 0).
    unsigned valueAt(const uchar *row, qint64 availableBytes, int index) const;

    // Извлечь одну строку. Возвращает массив длиной rowWidth.
    QVector<quint8> extractRow(const QByteArray &raw, int rowWidth) const;

    // Построить LUT [0..maxValue] -> ARGB. Если palette пуст — grayscale.
    QVector<QRgb> buildLut(const QVector<QColor> &palette) const;

private:
    int  m_bpp;
    bool m_lsbBits;     // bit_order  = "lsb"
    bool m_lsbPixels;   // pixel_order = "lsb"
    int  m_rowPadding;
    int  m_maxValue;
};
