// Byte-0-scope - binary data visualizer
// File: pixeldecoder.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QColor>
#include <QString>
#include <QVariantMap>

namespace PixelDecoder {

QRgb decodeArgb(const QByteArray &pixelData, const QString &paletteId,
                const QVariantMap &params = QVariantMap());

QRgb decodeArgbRaw(const uchar *pixelData, int size,
                   const QString &paletteId,
                   const QVariantMap &params = QVariantMap());

void decode(const QByteArray &pixelData, const QString &paletteId,
            const QVariantMap &params,
            int *outR, int *outG, int *outB, int *outA);

QString describe(const QByteArray &pixelData, const QString &paletteId,
                 const QVariantMap &params = QVariantMap());

QColor interpretPaletteRecord(const QByteArray &record, const QString &format,
                              int recordSize, const QString &endianness);

bool isMultibytePalette(const QString &paletteId);

inline quint16 rd16be(const uchar *p) { return quint16((p[0] << 8) | p[1]); }
inline quint16 rd16le(const uchar *p) { return quint16((p[1] << 8) | p[0]); }

} // namespace PixelDecoder