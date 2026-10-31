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

void decodeImpl(const uchar *p, int n, const QString &id,
                const QVariantMap &params,
                int *outR, int *outG, int *outB, int *outA)
{
    int r = 0, g = 0, b = 0, a = 255;

    if (n <= 0 || !p) {
        *outR = *outG = *outB = 0;
        *outA = 255;
        return;
    }

    if (decodeRgbaOrder(id, p, n, &r, &g, &b, &a)) {
        // ok
    } else if (decodeRgbOrder(id, p, n, &r, &g, &b)) {
        // ok
    } else if ((id == "rgb565" || id == "bgr565"
             || id == "rgb555" || id == "argb1555") && n >= 2) {
        const QString e = params.value("endianness", "big").toString();
        const quint16 v = readU16(p, e);
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
    } else if (id == "integer_16" && n >= 2) {
        const QString e = params.value("endianness", "big").toString();
        const bool sgn  = params.value("signed", false).toBool();
        const quint16 u = readU16(p, e);
        const double val = sgn ? double(qint16(u)) / 32768.0
                               : double(u) / 65535.0;
        const int v = qBound(0, int(std::lround(val * 255.0)), 255);
        r = g = b = v;
    } else if ((id == "integer_32" || id == "integer_64") && n >= 4) {
        const QString e = params.value("endianness", "big").toString();
        const bool sgn  = params.value("signed", false).toBool();
        if (id == "integer_32") {
            const qint32 v = (e == "little")
                ? qFromLittleEndian<qint32>(p)
                : qFromBigEndian<qint32>(p);
            const double val = sgn ? double(v) / 2147483648.0
                                   : double(quint32(v)) / 4294967295.0;
            r = g = b = qBound(0, int(std::lround(val * 255.0)), 255);
        } else {
            const qint64 v = (e == "little")
                ? qFromLittleEndian<qint64>(p)
                : qFromBigEndian<qint64>(p);
            const double val = sgn
                ? double(v) / 9223372036854775808.0
                : double(quint64(v)) / 18446744073709551615.0;
            r = g = b = qBound(0, int(std::lround(val * 255.0)), 255);
        }
    } else if (id == "float_32" && n >= 4) {
        const QString e = params.value("endianness", "big").toString();
        quint32 bits = (e == "little")
            ? qFromLittleEndian<quint32>(p)
            : qFromBigEndian<quint32>(p);
        float f;
        std::memcpy(&f, &bits, 4);
        const int g8 = qBound(0, int(std::lround(f * 255.0f)), 255);
        r = g = b = g8;
    } else if (id == "float_64" && n >= 8) {
        const QString e = params.value("endianness", "big").toString();
        quint64 bits = (e == "little")
            ? qFromLittleEndian<quint64>(p)
            : qFromBigEndian<quint64>(p);
        double d;
        std::memcpy(&d, &bits, 8);
        const int g8 = qBound(0, int(std::lround(d * 255.0)), 255);
        r = g = b = g8;
    } else if (id == "from_file" && n >= 1) {
        const QVariantMap prepared = params;
        const QString fmt = prepared.value("format", "auto").toString();
        const QString e   = prepared.value("endianness", "big").toString();
        const int recSize = qMax(1, prepared.value("record_size", 1).toInt());
        const QByteArray rec(reinterpret_cast<const char *>(p), qMin(n, recSize));
        const QColor c = interpretPaletteRecord(rec, fmt, recSize, e);
        r = c.red();
        g = c.green();
        b = c.blue();
        a = c.alpha();
    } else {
        r = g = b = p[0];
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
        {"integer_16", "Integer 16"}, {"integer_32", "Integer 32"},
        {"integer_64", "Integer 64"}, {"float_32", "Float 32"},
        {"float_64", "Float 64"}
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
        "integer_16", "integer_32", "integer_64",
        "float_32", "float_64"
    };
    for (const char *n : kNames)
        if (id == QLatin1String(n))
            return true;
    return false;
}

QRgb decodeArgbRaw(const uchar *p, int n, const QString &id,
                   const QVariantMap &params)
{
    int r, g, b, a;
    decodeImpl(p, n, id, params, &r, &g, &b, &a);
    return qRgba(r, g, b, a);
}

quint32 decodeArgb(const QByteArray &bytes, const QString &id,
                   const QVariantMap &params)
{
    return decodeArgbRaw(reinterpret_cast<const uchar *>(bytes.constData()),
                         bytes.size(), id, params);
}

QString describe(const QByteArray &px, const QString &id,
                 const QVariantMap &params)
{
    if (px.isEmpty())
        return QString();
    const uchar *p = reinterpret_cast<const uchar *>(px.constData());
    const int n = px.size();

    int r, g, b, a;
    decodeImpl(p, n, id, params, &r, &g, &b, &a);

    if (id == "rgba" || id == "argb"
     || id == "bgra" || id == "abgr") {
        return QString("%1: R=%2 G=%3 B=%4 A=%5")
                .arg(displayFormatName(id)).arg(r).arg(g).arg(b).arg(a);
    }
    if (id == "rgb" || id == "bgr" || id == "gbr"
     || id == "brg" || id == "grb" || id == "rbg") {
        return QString("%1: R=%2 G=%3 B=%4")
                .arg(displayFormatName(id)).arg(r).arg(g).arg(b);
    }
    if (id == "rgb565" || id == "bgr565" || id == "rgb555"
     || id == "argb1555") {
        return QString("%1: R=%2 G=%3 B=%4")
                .arg(displayFormatName(id)).arg(r).arg(g).arg(b);
    }
    if (id == "integer_16" || id == "integer_32" || id == "integer_64"
     || id == "float_32" || id == "float_64") {
        return QString("%1: %2").arg(displayFormatName(id)).arg(r);
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