// Byte-0-scope - binary data visualizer
// File: pngwriter.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pngwriter.h"

#include <QFile>
#include <QtEndian>
#include <cstring>

namespace PNGWriter {

namespace {

quint32 crc32Png(const QByteArray &data)
{
    static quint32 table[256];
    static bool init = false;
    if (!init) {
        for (quint32 n = 0; n < 256; ++n) {
            quint32 c = n;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[n] = c;
        }
        init = true;
    }
    quint32 c = 0xFFFFFFFFu;
    const uchar *p = reinterpret_cast<const uchar *>(data.constData());
    for (int i = 0; i < data.size(); ++i)
        c = table[(c ^ quint8(p[i])) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

QByteArray makeChunk(const QByteArray &type, const QByteArray &data)
{
    QByteArray out;
    out.resize(8 + data.size() + 4);
    uchar *p = reinterpret_cast<uchar *>(out.data());
    qToBigEndian<quint32>(quint32(data.size()), p);
    std::memcpy(p + 4, type.constData(), 4);
    std::memcpy(p + 8, data.constData(), data.size());

    QByteArray crcInput(type);
    crcInput.append(data);
    qToBigEndian<quint32>(crc32Png(crcInput), p + 8 + data.size());
    return out;
}

} // namespace

bool writeIndexed(const QString &path, const QVector<quint8> &indices,
                  const QVector<QColor> &palette,
                  int width, int height, int bitDepth)
{
    if (bitDepth != 1 && bitDepth != 2 && bitDepth != 4 && bitDepth != 8)
        return false;

    const int maxColors = 1 << bitDepth;
    if (palette.isEmpty() || palette.size() > maxColors)
        return false;
    if (indices.size() < width * height)
        return false;

    // IHDR
    QByteArray ihdr(13, '\0');
    uchar *h = reinterpret_cast<uchar *>(ihdr.data());
    qToBigEndian<quint32>(quint32(width), h);
    qToBigEndian<quint32>(quint32(height), h + 4);
    h[8]  = uchar(bitDepth);
    h[9]  = 3;   // color type = indexed
    h[10] = 0;
    h[11] = 0;
    h[12] = 0;

    // PLTE
    QByteArray plte(palette.size() * 3, '\0');
    uchar *pl = reinterpret_cast<uchar *>(plte.data());
    for (int i = 0; i < palette.size(); ++i) {
        pl[i * 3]     = uchar(palette[i].red());
        pl[i * 3 + 1] = uchar(palette[i].green());
        pl[i * 3 + 2] = uchar(palette[i].blue());
    }

    // IDAT: filter byte 0 + упакованные индексы (MSB-first)
    const int perByte = 8 / bitDepth;
    const int mask = maxColors - 1;
    QByteArray raw;
    raw.reserve(height * (1 + (width + perByte - 1) / perByte));
    for (int y = 0; y < height; ++y) {
        raw.append('\0');  // filter type None
        int acc = 0;
        int cnt = 0;
        for (int x = 0; x < width; ++x) {
            acc = (acc << bitDepth) | (indices[y * width + x] & mask);
            if (++cnt == perByte) {
                raw.append(char(acc));
                acc = 0;
                cnt = 0;
            }
        }
        if (cnt > 0) {
            acc <<= (perByte - cnt) * bitDepth;
            raw.append(char(acc));
        }
    }

    QByteArray compressed = qCompress(raw, 9);
    QByteArray idat = compressed.mid(4);  // strip big-endian size prefix

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write("\x89PNG\r\n\x1a\n", 8);
    f.write(makeChunk("IHDR", ihdr));
    f.write(makeChunk("PLTE", plte));
    f.write(makeChunk("IDAT", idat));
    f.write(makeChunk("IEND", QByteArray()));
    f.close();
    return true;
}

} // namespace PNGWriter