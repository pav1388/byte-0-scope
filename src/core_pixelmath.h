// Byte-0-scope - binary data visualizer
// File: core_pixelmath.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtGlobal>

#include <cmath>
#include <cstring>
#include <limits>

// Математика пикселей и геометрии региона.
namespace PixelMath {

// ---------------------------------------------------------------------------
//  Биты
// ---------------------------------------------------------------------------

// Реверс битов в байте. Используется при bit_order = "lsb".
inline quint8 reverseBits8(quint8 b) noexcept
{
    b = quint8((b & 0xF0u) >> 4) | quint8((b & 0x0Fu) << 4);
    b = quint8((b & 0xCCu) >> 2) | quint8((b & 0x33u) << 2);
    b = quint8((b & 0xAAu) >> 1) | quint8((b & 0x55u) << 1);
    return b;
}

//  Геометрия региона
inline quint64 effectiveRegionLength(quint64 sourceSize, quint64 offset,
                                     quint64 requestedLength) noexcept
{
    if (offset >= sourceSize)
        return 0;
    const quint64 available = sourceSize - offset;
    return requestedLength > 0 ? qMin(requestedLength, available) : available;
}

// Размер строки bit-packed изображения в байтах (с учётом паддинга строки).
inline qint64 bitpackedRowBytes(qint64 width, int bitsPerPixel,
                                int paddingBits) noexcept
{
    const qint64 rowBits = width * qBound(1, bitsPerPixel, 8)
                         + qMax(0, paddingBits);
    return (rowBits + 7) / 8;
}

// Количество пикселей в byteCount байтах (с насыщением до INT_MAX).
inline int pixelCountForBytes(quint64 byteCount, bool bitpacked,
                              int bitsPerPixel, int bytesPerPixel) noexcept
{
    const quint64 intMax = quint64(std::numeric_limits<int>::max());
    quint64 pixels = 0;
    if (bitpacked) {
        const quint64 bpp = quint64(qBound(1, bitsPerPixel, 8));
        const quint64 maxBytesWithoutSaturation = (intMax * bpp) / 8;
        if (byteCount > maxBytesWithoutSaturation)
            return std::numeric_limits<int>::max();
        pixels = (byteCount * 8) / bpp;
    } else {
        pixels = byteCount / quint64(qMax(1, bytesPerPixel));
    }
    return int(qMin(pixels, intMax));
}

// Высота изображения (в строках) для byteCount байт при заданной ширине.
inline qint64 imageHeightForBytes(quint64 byteCount, int width,
                                  bool bitpacked, int bitsPerPixel,
                                  int rowPaddingBits,
                                  int bytesPerPixel) noexcept
{
    if (byteCount == 0 || width <= 0)
        return 0;

    quint64 rows = 0;
    if (bitpacked) {
        const qint64 rowBytes = bitpackedRowBytes(width, bitsPerPixel,
                                                  rowPaddingBits);
        if (rowBytes <= 0)
            return 0;
        const quint64 stride = quint64(rowBytes);
        rows = byteCount / stride + (byteCount % stride != 0 ? 1 : 0);
    } else {
        const quint64 bpp = quint64(qMax(1, bytesPerPixel));
        const quint64 pixels = byteCount / bpp;
        const quint64 w = quint64(width);
        rows = pixels / w + (pixels % w != 0 ? 1 : 0);
    }

    return rows > quint64(std::numeric_limits<qint64>::max())
        ? std::numeric_limits<qint64>::max() : qint64(rows);
}

//  Bit-packed пиксели (1..8 bpp)
inline unsigned bitpackedValue(const uchar *row, qint64 availableBytes,
                               qint64 firstBit, int bpp,
                               bool lsbBits, bool lsbPixels) noexcept
{
    const qint64 byteIndex = firstBit >> 3;
    const int shift = int(firstBit & 7);

    unsigned b0 = 0;
    unsigned b1 = 0;
    if (row) {
        if (byteIndex < availableBytes)
            b0 = row[byteIndex];
        if (byteIndex + 1 < availableBytes)
            b1 = row[byteIndex + 1];
    }

    const unsigned mask = (1u << bpp) - 1u;
    unsigned group;
    if (lsbBits)
        group = ((b0 | (b1 << 8)) >> shift) & mask;
    else
        group = (((b0 << 8) | b1) >> (16 - shift - bpp)) & mask;

    if (lsbBits == lsbPixels)
        return group;
    return unsigned(reverseBits8(quint8(group))) >> (8 - bpp);
}

//  Многобайтовые форматы (16/24/32/64 bpp)
enum PixelFormat {
    PF_Unknown = 0,
    PF_Rgb, PF_Bgr, PF_Gbr, PF_Brg, PF_Grb, PF_Rbg,
    PF_Rgba, PF_Argb, PF_Bgra, PF_Abgr,
    PF_Rgb565, PF_Bgr565, PF_Rgb555, PF_Argb1555,
    PF_Int16, PF_Int32, PF_Int64,
    PF_Float32, PF_Float64,
    PF_FromFile
};

struct Rgba8 {
    int r, g, b, a;
};

inline int expand5(int v) noexcept { return (v << 3) | (v >> 2); }
inline int expand6(int v) noexcept { return (v << 2) | (v >> 4); }

inline quint16 readU16BE(const uchar *p) noexcept
{
    return quint16((unsigned(p[0]) << 8) | unsigned(p[1]));
}

inline quint32 readU32BE(const uchar *p) noexcept
{
    return (quint32(p[0]) << 24) | (quint32(p[1]) << 16)
         | (quint32(p[2]) << 8)  |  quint32(p[3]);
}

inline quint64 readU64BE(const uchar *p) noexcept
{
    return (quint64(readU32BE(p)) << 32) | quint64(readU32BE(p + 4));
}

// Яркость 0..255 из значения в диапазоне [0, 1].
inline int unitToGray(double v) noexcept
{
    const double s = v * 255.0;
    if (!(s > 0.0))
        return 0;
    if (s >= 255.0)
        return 255;
    return int(std::lround(s));
}

inline int unitToGrayF(float v) noexcept
{
    const float s = v * 255.0f;
    if (!(s > 0.0f))
        return 0;
    if (s >= 255.0f)
        return 255;
    return int(std::lround(s));
}

// Декодирует «нормализованный» буфер
inline bool decodeNormalized(PixelFormat fmt, bool isSigned,
                             const uchar *s, int n, Rgba8 *out) noexcept
{
    int r = 0, g = 0, b = 0, a = 255;

    switch (fmt) {
    case PF_Rgb:
    case PF_Bgr:
    case PF_Gbr:
    case PF_Brg:
    case PF_Grb:
    case PF_Rbg: {
        if (n < 3)
            return false;
        int ri = 0, gi = 1, bi = 2;
        switch (fmt) {
        case PF_Bgr: ri = 2; gi = 1; bi = 0; break;
        case PF_Gbr: ri = 1; gi = 2; bi = 0; break;
        case PF_Brg: ri = 2; gi = 0; bi = 1; break;
        case PF_Grb: ri = 1; gi = 0; bi = 2; break;
        case PF_Rbg: ri = 0; gi = 2; bi = 1; break;
        default: break;
        }
        r = s[ri]; g = s[gi]; b = s[bi];
        break;
    }
    case PF_Rgba:
    case PF_Argb:
    case PF_Bgra:
    case PF_Abgr: {
        if (n < 4)
            return false;
        int ri = 0, gi = 1, bi = 2, ai = 3;
        switch (fmt) {
        case PF_Argb: ri = 1; gi = 2; bi = 3; ai = 0; break;
        case PF_Bgra: ri = 2; gi = 1; bi = 0; ai = 3; break;
        case PF_Abgr: ri = 3; gi = 2; bi = 1; ai = 0; break;
        default: break;
        }
        r = s[ri]; g = s[gi]; b = s[bi]; a = s[ai];
        break;
    }
    case PF_Rgb565:
    case PF_Bgr565:
    case PF_Rgb555:
    case PF_Argb1555: {
        if (n < 2)
            return false;
        const int v = readU16BE(s);
        if (fmt == PF_Rgb565) {
            r = expand5((v >> 11) & 0x1F);
            g = expand6((v >>  5) & 0x3F);
            b = expand5( v        & 0x1F);
        } else if (fmt == PF_Bgr565) {
            b = expand5((v >> 11) & 0x1F);
            g = expand6((v >>  5) & 0x3F);
            r = expand5( v        & 0x1F);
        } else {
            if (fmt == PF_Argb1555)
                a = ((v >> 15) & 1) ? 255 : 0;
            r = expand5((v >> 10) & 0x1F);
            g = expand5((v >>  5) & 0x1F);
            b = expand5( v        & 0x1F);
        }
        break;
    }
    case PF_Int16: {
        if (n < 2)
            return false;
        const quint16 u = readU16BE(s);
        const double val = isSigned ? double(qint16(u)) / 32768.0
                                    : double(u) / 65535.0;
        r = g = b = unitToGray(val);
        break;
    }
    case PF_Int32: {
        if (n < 4)
            return false;
        const quint32 u = readU32BE(s);
        const double val = isSigned ? double(qint32(u)) / 2147483648.0
                                    : double(u) / 4294967295.0;
        r = g = b = unitToGray(val);
        break;
    }
    case PF_Int64: {
        if (n < 8)
            return false;
        const quint64 u = readU64BE(s);
        const double val = isSigned
            ? double(qint64(u)) / 9223372036854775808.0
            : double(u) / 18446744073709551615.0;
        r = g = b = unitToGray(val);
        break;
    }
    case PF_Float32: {
        if (n < 4)
            return false;
        const quint32 bits = readU32BE(s);
        float f;
        static_assert(sizeof(f) == sizeof(bits), "unexpected float size");
        std::memcpy(&f, &bits, sizeof(f));
        r = g = b = unitToGrayF(f);
        break;
    }
    case PF_Float64: {
        if (n < 8)
            return false;
        const quint64 bits = readU64BE(s);
        double d;
        static_assert(sizeof(d) == sizeof(bits), "unexpected double size");
        std::memcpy(&d, &bits, sizeof(d));
        r = g = b = unitToGray(d);
        break;
    }
    case PF_FromFile:
    case PF_Unknown:
    default:
        return false;
    }

    out->r = r;
    out->g = g;
    out->b = b;
    out->a = a;
    return true;
}

} // namespace PixelMath
