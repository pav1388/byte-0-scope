// Byte-0-scope - binary data visualizer
// File: pixeldecoder.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pixeldecoder.h"

#include <QtGlobal>
#include <QtEndian>
#include <QHash>
#include <cmath>
#include <cstring>

namespace PixelDecoder {

namespace {

inline int expand5(int v) { return (v << 3) | (v >> 2); }
inline int expand6(int v) { return (v << 2) | (v >> 4); }

inline quint16 readU16BE(const uchar *p)
{
    return qFromBigEndian<quint16>(p);
}

// Для interpretPaletteRecord(): endianness приходит из параметров
// палитры from_file и не связан с глобальным byte order изображения.
inline quint16 readU16(const uchar *p, const QString &endianness)
{
    return endianness == QLatin1String("little")
        ? qFromLittleEndian<quint16>(p)
        : qFromBigEndian<quint16>(p);
}

bool decodeRgbOrder(const QString &id, const uchar *p, int n,
                    int *outR, int *outG, int *outB)
{
    if (n < 3)
        return false;
    int ri, gi, bi;
    if      (id == "rgb") { ri = 0; gi = 1; bi = 2; }
    else if (id == "bgr") { ri = 2; gi = 1; bi = 0; }
    else if (id == "gbr") { ri = 1; gi = 2; bi = 0; }
    else if (id == "brg") { ri = 2; gi = 0; bi = 1; }
    else if (id == "grb") { ri = 1; gi = 0; bi = 2; }
    else if (id == "rbg") { ri = 0; gi = 2; bi = 1; }
    else return false;
    *outR = p[ri];
    *outG = p[gi];
    *outB = p[bi];
    return true;
}

bool decodeRgbaOrder(const QString &id, const uchar *p, int n,
                     int *outR, int *outG, int *outB, int *outA)
{
    if (n < 4)
        return false;
    int ri, gi, bi, ai;
    if      (id == "rgba") { ri = 0; gi = 1; bi = 2; ai = 3; }
    else if (id == "argb") { ri = 1; gi = 2; bi = 3; ai = 0; }
    else if (id == "bgra") { ri = 2; gi = 1; bi = 0; ai = 3; }
    else if (id == "abgr") { ri = 3; gi = 2; bi = 1; ai = 0; }
    else return false;
    *outR = p[ri];
    *outG = p[gi];
    *outB = p[bi];
    *outA = p[ai];
    return true;
}

// Integer-палитры (16/32/64). Суффикс "_signed" в id зашивает знаковость —
// params["signed"] используется только как fallback (для обратной совместимости).
bool isIntegerPalette16(const QString &id) { return id == "integer_16" || id == "integer_16_signed"; }
bool isIntegerPalette32(const QString &id) { return id == "integer_32" || id == "integer_32_signed"; }
bool isIntegerPalette64(const QString &id) { return id == "integer_64" || id == "integer_64_signed"; }

bool integerSigned(const QString &id, const QVariantMap &params)
{
    if (id.endsWith(QLatin1String("_signed")))
        return true;
    return params.value("signed", false).toBool();
}

void decodeImpl(const uchar *p, int n, const QString &id,
                const QString &bitOrder, const QString &byteOrder,
                const QVariantMap &params,
                int *outR, int *outG, int *outB, int *outA)
{
    int r = 0, g = 0, b = 0, a = 255;

    if (n <= 0 || !p) {
        *outR = *outG = *outB = 0;
        *outA = 255;
        return;
    }

    const bool le  = (byteOrder == QLatin1String("little"));
    const bool rev = (bitOrder == QLatin1String("lsb"));

    // Нормализуем порядок байтов и битов в локальный буфер.
    // После этого декодируем так, будто данные в big-endian с нормальными битами.
    uchar buf[8];
    const int cnt = qMin(n, 8);
    for (int i = 0; i < cnt; ++i) {
        const uchar raw = le ? p[cnt - 1 - i] : p[i];
        buf[i] = rev ? reverseBits8(raw) : raw;
    }
    const uchar *src = buf;

    if (decodeRgbaOrder(id, src, n, &r, &g, &b, &a)) {
        // ok
    } else if (decodeRgbOrder(id, src, n, &r, &g, &b)) {
        // ok
    } else if ((id == "rgb565" || id == "bgr565"
             || id == "rgb555" || id == "argb1555") && n >= 2) {
        const quint16 v = readU16BE(src);
        if (id == "rgb565") {
            r = expand5((v >> 11) & 0x1F);
            g = expand6((v >>  5) & 0x3F);
            b = expand5( v        & 0x1F);
        } else if (id == "bgr565") {
            b = expand5((v >> 11) & 0x1F);
            g = expand6((v >>  5) & 0x3F);
            r = expand5( v        & 0x1F);
        } else if (id == "rgb555") {
            r = expand5((v >> 10) & 0x1F);
            g = expand5((v >>  5) & 0x1F);
            b = expand5( v        & 0x1F);
        } else { // ARGB1555
            a = ((v >> 15) & 1) ? 255 : 0;
            r = expand5((v >> 10) & 0x1F);
            g = expand5((v >>  5) & 0x1F);
            b = expand5( v        & 0x1F);
        }
    } else if (isIntegerPalette16(id) && n >= 2) {
        const bool sgn = integerSigned(id, params);
        const quint16 u = readU16BE(src);
        const double val = sgn ? double(qint16(u)) / 32768.0
                               : double(u) / 65535.0;
        r = g = b = qBound(0, int(std::lround(val * 255.0)), 255);
    } else if (isIntegerPalette32(id) && n >= 4) {
        const bool sgn = integerSigned(id, params);
        const qint32 v = qFromBigEndian<qint32>(src);
        const double val = sgn ? double(v) / 2147483648.0
                               : double(quint32(v)) / 4294967295.0;
        r = g = b = qBound(0, int(std::lround(val * 255.0)), 255);
    } else if (isIntegerPalette64(id) && n >= 8) {
        const bool sgn = integerSigned(id, params);
        const qint64 v = qFromBigEndian<qint64>(src);
        const double val = sgn
            ? double(v) / 9223372036854775808.0
            : double(quint64(v)) / 18446744073709551615.0;
        r = g = b = qBound(0, int(std::lround(val * 255.0)), 255);
    } else if (id == "float_32" && n >= 4) {
        quint32 bits = qFromBigEndian<quint32>(src);
        float f;
        std::memcpy(&f, &bits, 4);
        r = g = b = qBound(0, int(std::lround(f * 255.0f)), 255);
    } else if (id == "float_64" && n >= 8) {
        quint64 bits = qFromBigEndian<quint64>(src);
        double d;
        std::memcpy(&d, &bits, 8);
        r = g = b = qBound(0, int(std::lround(d * 255.0)), 255);
    } else if (id == "from_file" && n >= 1) {
        // Палитра from_file интерпретирует байты через свои params
        // (format, record_size, endianness) — не через глобальный byte_order.
        // Поэтому здесь используем исходный p[], а не нормализованный buf[].
        const QString fmt = params.value("format", "auto").toString();
        const QString e   = params.value("endianness", "big").toString();
        const int recSize = qMax(1, params.value("record_size", 1).toInt());
        const QByteArray rec(reinterpret_cast<const char *>(p), qMin(n, recSize));
        const QColor c = interpretPaletteRecord(rec, fmt, recSize, e);
        r = c.red();
        g = c.green();
        b = c.blue();
        a = c.alpha();
    } else {
        r = g = b = src[0];
    }

    *outR = r;
    *outG = g;
    *outB = b;
    *outA = a;
}

QString displayFormatName(const QString &id)
{
    static const QHash<QString, QString> labels = {
        {"rgb", "RGB"}, {"bgr", "BGR"}, {"gbr", "GBR"},
        {"brg", "BRG"}, {"grb", "GRB"}, {"rbg", "RBG"},
        {"rgba", "RGBA"}, {"argb", "ARGB"},
        {"bgra", "BGRA"}, {"abgr", "ABGR"},
        {"rgb565", "RGB565"}, {"bgr565", "BGR565"},
        {"rgb555", "RGB555"}, {"argb1555", "ARGB1555"},
        {"integer_16",        "Integer 16 (unsigned)"},
        {"integer_16_signed", "Integer 16 (signed)"},
        {"integer_32",        "Integer 32 (unsigned)"},
        {"integer_32_signed", "Integer 32 (signed)"},
        {"integer_64",        "Integer 64 (unsigned)"},
        {"integer_64_signed", "Integer 64 (signed)"},
        {"float_32", "Float 32"}, {"float_64", "Float 64"}
    };
    return labels.value(id, id);
}

} // namespace

bool isMultibytePalette(const QString &id)
{
    static const char *kNames[] = {
        "rgba", "argb", "bgra", "abgr",
        "rgb", "bgr", "gbr", "brg", "grb", "rbg",
        "rgb565", "bgr565", "rgb555", "argb1555",
        "integer_16", "integer_16_signed",
        "integer_32", "integer_32_signed",
        "integer_64", "integer_64_signed",
        "float_32", "float_64"
    };
    for (const char *n : kNames)
        if (id == QLatin1String(n))
            return true;
    return false;
}

QRgb decodeArgbRaw(const uchar *p, int n, const QString &id,
                   const QString &bitOrder, const QString &byteOrder,
                   const QVariantMap &params)
{
    int r, g, b, a;
    decodeImpl(p, n, id, bitOrder, byteOrder, params, &r, &g, &b, &a);
    return qRgba(r, g, b, a);
}

quint32 decodeArgb(const QByteArray &bytes, const QString &id,
                   const QString &bitOrder, const QString &byteOrder,
                   const QVariantMap &params)
{
    return decodeArgbRaw(reinterpret_cast<const uchar *>(bytes.constData()),
                         bytes.size(), id, bitOrder, byteOrder, params);
}

QString describe(const QByteArray &px, const QString &id,
                 const QString &bitOrder, const QString &byteOrder,
                 const QVariantMap &params)
{
    if (px.isEmpty())
        return QString();
    const uchar *p = reinterpret_cast<const uchar *>(px.constData());
    const int n = px.size();

    int r, g, b, a;
    decodeImpl(p, n, id, bitOrder, byteOrder, params, &r, &g, &b, &a);

    if (id == "rgba" || id == "argb"
     || id == "bgra" || id == "abgr") {
        return QString("R=%1 G=%2 B=%3 A=%4").arg(r).arg(g).arg(b).arg(a);
    }
    if (id == "rgb" || id == "bgr" || id == "gbr"
     || id == "brg" || id == "grb" || id == "rbg") {
        return QString("R=%1 G=%2 B=%3").arg(r).arg(g).arg(b);
    }
    if (id == "rgb565" || id == "bgr565" || id == "rgb555"
     || id == "argb1555") {
        return QString("R=%1 G=%2 B=%3").arg(r).arg(g).arg(b);
    }
    if (isIntegerPalette16(id) || isIntegerPalette32(id) || isIntegerPalette64(id)) {
        const bool sgn = integerSigned(id, params);
        QString rawVal;
        if (isIntegerPalette16(id) && n >= 2) {
            const quint16 u = readU16BE(
                reinterpret_cast<const uchar *>(px.constData()));
            rawVal = QString::number(sgn ? qint16(u) : qint32(u));
        } else if (isIntegerPalette32(id) && n >= 4) {
            const qint32 v = qFromBigEndian<qint32>(
                reinterpret_cast<const uchar *>(px.constData()));
            rawVal = QString::number(sgn ? qint64(v) : qint64(quint32(v)));
        } else if (isIntegerPalette64(id) && n >= 8) {
            const qint64 v = qFromBigEndian<qint64>(
                reinterpret_cast<const uchar *>(px.constData()));
            rawVal = sgn ? QString::number(v)
                         : QString::number(quint64(v));
        }
        return QString("%1 > gray=%2").arg(rawVal).arg(r);
    }
    if (id == "float_32" || id == "float_64") {
        return QString("%1").arg(r);
    }
    return QString();
}

QColor interpretPaletteRecord(const QByteArray &record,
                              const QString &format,
                              int recordSize,
                              const QString &endianness)
{
    auto at = [&](int i) -> int {
        return (i >= 0 && i < record.size())
            ? static_cast<uchar>(record.at(i))
            : 0;
    };

    QString f = format;
    if (f == "auto") {
        const int recSize = recordSize > 0 ? recordSize : record.size();
        if (recSize == 1)      f = "grayscale";
        else if (recSize == 2) f = "rgb565";
        else if (recSize == 3) f = "rgb";
        else if (recSize >= 4) f = "rgba";
    }

    if (f == "grayscale")
        return QColor(at(0), at(0), at(0));

    if (f == "rgb332") {
        const int v = at(0);
        return QColor(((v >> 5) & 0x07) * 255 / 7,
                      ((v >> 2) & 0x07) * 255 / 7,
                      ( v       & 0x03) * 255 / 3);
    }

    if (f == "rgb565" || f == "bgr565" || f == "rgb555" || f == "argb1555") {
        if (record.size() < 2)
            return QColor(0, 0, 0);
        const uchar *p = reinterpret_cast<const uchar *>(record.constData());
        const quint16 v = readU16(p, endianness);
        if (f == "rgb565")
            return QColor(expand5((v >> 11) & 0x1F),
                          expand6((v >>  5) & 0x3F),
                          expand5( v        & 0x1F));
        if (f == "bgr565")
            return QColor(expand5( v        & 0x1F),
                          expand6((v >>  5) & 0x3F),
                          expand5((v >> 11) & 0x1F));
        if (f == "rgb555")
            return QColor(expand5((v >> 10) & 0x1F),
                          expand5((v >>  5) & 0x1F),
                          expand5( v        & 0x1F));
        return QColor(expand5((v >> 10) & 0x1F),
                      expand5((v >>  5) & 0x1F),
                      expand5( v        & 0x1F),
                      ((v >> 15) & 1) ? 255 : 0);
    }

    if (f == "rgb")  return QColor(at(0), at(1), at(2));
    if (f == "bgr")  return QColor(at(2), at(1), at(0));
    if (f == "gbr")  return QColor(at(2), at(0), at(1));
    if (f == "brg")  return QColor(at(1), at(2), at(0));
    if (f == "grb")  return QColor(at(1), at(0), at(2));
    if (f == "rbg")  return QColor(at(0), at(2), at(1));
    if (f == "rgba") return QColor(at(0), at(1), at(2), at(3));
    if (f == "argb") return QColor(at(1), at(2), at(3), at(0));
    if (f == "bgra") return QColor(at(2), at(1), at(0), at(3));
    if (f == "abgr") return QColor(at(3), at(2), at(1), at(0));

    return QColor(at(0), at(0), at(0));
}

} // namespace PixelDecoder