// Byte-0-scope - binary data visualizer
// File: pngwriter.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QColor>
#include <QString>
#include <QVector>

// Минимальный энкодер индексированного PNG (bit depth 1/2/4/8).
// Использует qCompress() из Qt (zlib-совместимый поток).
namespace PNGWriter {

bool writeIndexed(const QString &path,
                  const QVector<quint8> &indices,  // width*height, row-major
                  const QVector<QColor> &palette,
                  int width, int height, int bitDepth);

} // namespace PNGWriter