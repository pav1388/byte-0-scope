// Byte-0-scope - binary data visualizer
// File: pixeldecoder.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pixeldecoder.h"

#include <QtEndian>

namespace PixelDecoder {

using namespace PixelMath;

namespace {

// Для interpretPaletteRecord(): endianness приходит из параметров
// палитры from_file и не связан с глобальным byte order изображения.
inline quint16 readU16(const uchar *p, const QString &endianness)
{
    return endianness == QLatin1String("little")
        ? qFromLittleEndian<quint16>(p)
        : qFromBigEndian<quint16>(p);
}

struct FormatInfo {
    PixelFormat format;
    bool        isSigned;
};

struct FormatEntry {
    const char *id;
    PixelFormat format;
    bool        isSigned;
};

// Идентификатор палитры -> формат. Суффикс "_signed" в id зашивает знаковость;
// params["signed"] используется только как fallback (обратная совместимость).
FormatInfo lookupFormat(const QString &id)
{
    static const FormatEntry kFormats[] = {
        { "rgb",               PF_Rgb,      false },
        { "bgr",               PF_Bgr,      false },
        { "gbr",               PF_Gbr,      false },
        { "brg",               PF_Brg,      false },
        { "grb",               PF_Grb,      false },
        { "rbg",               PF_Rbg,      false },
        { "rgba",              PF_Rgba,     false },
        { "argb",              PF_Argb,     false },
        { "bgra",              PF_Bgra,     false },
        { "abgr",              PF_Abgr,     false },
        { "rgb565",            PF_Rgb565,   false },
        { "bgr565",            PF_Bgr565,   false },
        { "rgb555",            PF_Rgb555,   false },
        { "argb1555",          PF_Argb1555, false },
        { "integer_16",        PF_Int16,    false },
        { "integer_16_signed", PF_Int16,    true  },
        { "integer_32",        PF_Int32,    false },
        { "integer_32_signed", PF_Int32,    true  },
        { "integer_64",        PF_Int64,    false },
        { "integer_64_signed", PF_Int64,    true  },
        { "float_32",          PF_Float32,  false },
        { "float_64",          PF_Float64,  false },
        { "from_file",         PF_FromFile, false },
    };
    for (const FormatEntry &e : kFormats) {
        if (id == QLatin1String(e.id)) {
            FormatInfo info;
            info.format   = e.format;
            info.isSigned = e.isSigned;
            return info;
        }
    }
    FormatInfo none;
    none.format   = PF_Unknown;
    none.isSigned = false;
    return none;
}

bool isPlainRgbFamily(PixelFormat f)
{
    return f == PF_Rgb || f == PF_Bgr || f == PF_Gbr
        || f == PF_Brg || f == PF_Grb || f == PF_Rbg
        || f == PF_Rgb565 || f == PF_Bgr565
        || f == PF_Rgb555 || f == PF_Argb1555;
}

bool isRgbaFamily(PixelFormat f)
{
    return f == PF_Rgba || f == PF_Argb || f == PF_Bgra || f == PF_Abgr;
}

} // namespace

PixelFormat parseFormat(const QString &paletteId)
{
    return lookupFormat(paletteId).format;
}

bool isMultibytePalette(const QString &id)
{
    const PixelFormat f = parseFormat(id);
    return f != PF_Unknown && f != PF_FromFile;
}

// =========================================================================
//  Decoder
// =========================================================================

Decoder::Decoder(const QString &paletteId, const QString &bitOrder,
                 const QString &byteOrder, const QVariantMap &params)
    : m_format(PF_Unknown),
      m_littleEndian(byteOrder == QLatin1String("little")),
      m_reverseBits(bitOrder == QLatin1String("lsb")),
      m_signed(false),
      m_fileRecordSize(1)
{
    const FormatInfo info = lookupFormat(paletteId);
    m_format = info.format;
    m_signed = info.isSigned
        || params.value(QStringLiteral("signed"), false).toBool();

    if (m_format == PF_FromFile) {
        m_fileFormat     = params.value(QStringLiteral("format"),
                                        QStringLiteral("auto")).toString();
        m_fileEndianness = params.value(QStringLiteral("endianness"),
                                        QStringLiteral("big")).toString();
        m_fileRecordSize = qMax(1, params.value(QStringLiteral("record_size"),
                                                1).toInt());
    }
}

int Decoder::normalize(const uchar *p, int n, uchar out[8]) const
{
    // Нормализуем порядок байтов и битов в локальный буфер. После этого
    // декодируем так, будто данные в big-endian с нормальными битами.
    const int cnt = qBound(0, n, 8);
    for (int i = 0; i < cnt; ++i) {
        const uchar raw = m_littleEndian ? p[cnt - 1 - i] : p[i];
        out[i] = m_reverseBits ? PixelMath::reverseBits8(raw) : raw;
    }
    return cnt;
}

QRgb Decoder::decode(const uchar *p, int n) const
{
    if (n <= 0 || !p)
        return qRgba(0, 0, 0, 255);

    uchar buf[8];
    const int cnt = normalize(p, n, buf);

    Rgba8 px = { 0, 0, 0, 255 };
    if (m_format == PF_FromFile) {
        // Палитра from_file интерпретирует байты через свои параметры
        // (format, record_size, endianness) — не через глобальный byte_order.
        // Поэтому здесь используем исходный p[], а не нормализованный buf[].
        const QByteArray rec(reinterpret_cast<const char *>(p),
                             qMin(n, m_fileRecordSize));
        const QColor c = interpretPaletteRecord(rec, m_fileFormat,
                                                m_fileRecordSize,
                                                m_fileEndianness);
        return qRgba(c.red(), c.green(), c.blue(), c.alpha());
    }

    if (!decodeNormalized(m_format, m_signed, buf, cnt, &px)) {
        // Неизвестный формат или не хватает данных — серый по первому байту.
        px.r = px.g = px.b = buf[0];
        px.a = 255;
    }
    return qRgba(px.r, px.g, px.b, px.a);
}

// =========================================================================
//  Свободные функции
// =========================================================================

QRgb decodeArgbRaw(const uchar *p, int n, const QString &id,
                   const QString &bitOrder, const QString &byteOrder,
                   const QVariantMap &params)
{
    return Decoder(id, bitOrder, byteOrder, params).decode(p, n);
}

QRgb decodeArgb(const QByteArray &bytes, const QString &id,
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

    const Decoder decoder(id, bitOrder, byteOrder, params);
    const PixelFormat fmt = decoder.format();

    const uchar *p = reinterpret_cast<const uchar *>(px.constData());
    const QRgb argb = decoder.decode(p, px.size());
    const int r = qRed(argb);
    const int g = qGreen(argb);
    const int b = qBlue(argb);
    const int a = qAlpha(argb);

    if (isRgbaFamily(fmt))
        return QStringLiteral("R=%1 G=%2 B=%3 A=%4").arg(r).arg(g).arg(b).arg(a);
    if (isPlainRgbFamily(fmt))
        return QStringLiteral("R=%1 G=%2 B=%3").arg(r).arg(g).arg(b);

    if (fmt == PF_Int16 || fmt == PF_Int32 || fmt == PF_Int64) {
        // Сырое значение берём из нормализованных байтов — тех же, что
        // использует декодер и что показывает строка «Dec» в панели.
        uchar buf[8];
        const int cnt = decoder.normalize(p, px.size(), buf);
        const bool sgn = decoder.isSigned();

        QString rawVal;
        if (fmt == PF_Int16 && cnt >= 2) {
            const quint16 u = readU16BE(buf);
            rawVal = QString::number(sgn ? qint32(qint16(u)) : qint32(u));
        } else if (fmt == PF_Int32 && cnt >= 4) {
            const quint32 u = readU32BE(buf);
            rawVal = QString::number(sgn ? qint64(qint32(u)) : qint64(u));
        } else if (fmt == PF_Int64 && cnt >= 8) {
            const quint64 u = readU64BE(buf);
            rawVal = sgn ? QString::number(qint64(u)) : QString::number(u);
        }
        return QStringLiteral(" > gray=%1").arg(r);
    }
    if (fmt == PF_Float32 || fmt == PF_Float64)
        return QString::number(r);
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
    if (f == QLatin1String("auto")) {
        const int recSize = recordSize > 0 ? recordSize : record.size();
        if (recSize == 1)      f = QStringLiteral("grayscale");
        else if (recSize == 2) f = QStringLiteral("rgb565");
        else if (recSize == 3) f = QStringLiteral("rgb");
        else if (recSize >= 4) f = QStringLiteral("rgba");
    }

    if (f == QLatin1String("grayscale"))
        return QColor(at(0), at(0), at(0));

    if (f == QLatin1String("rgb332")) {
        const int v = at(0);
        return QColor(((v >> 5) & 0x07) * 255 / 7,
                      ((v >> 2) & 0x07) * 255 / 7,
                      ( v       & 0x03) * 255 / 3);
    }

    if (f == QLatin1String("rgb565") || f == QLatin1String("bgr565")
            || f == QLatin1String("rgb555") || f == QLatin1String("argb1555")) {
        if (record.size() < 2)
            return QColor(0, 0, 0);
        const uchar *p = reinterpret_cast<const uchar *>(record.constData());
        const int v = readU16(p, endianness);
        if (f == QLatin1String("rgb565"))
            return QColor(expand5((v >> 11) & 0x1F),
                          expand6((v >>  5) & 0x3F),
                          expand5( v        & 0x1F));
        if (f == QLatin1String("bgr565"))
            return QColor(expand5( v        & 0x1F),
                          expand6((v >>  5) & 0x3F),
                          expand5((v >> 11) & 0x1F));
        if (f == QLatin1String("rgb555"))
            return QColor(expand5((v >> 10) & 0x1F),
                          expand5((v >>  5) & 0x1F),
                          expand5( v        & 0x1F));
        return QColor(expand5((v >> 10) & 0x1F),
                      expand5((v >>  5) & 0x1F),
                      expand5( v        & 0x1F),
                      ((v >> 15) & 1) ? 255 : 0);
    }

    if (f == QLatin1String("rgb"))  return QColor(at(0), at(1), at(2));
    if (f == QLatin1String("bgr"))  return QColor(at(2), at(1), at(0));
    if (f == QLatin1String("gbr"))  return QColor(at(2), at(0), at(1));
    if (f == QLatin1String("brg"))  return QColor(at(1), at(2), at(0));
    if (f == QLatin1String("grb"))  return QColor(at(1), at(0), at(2));
    if (f == QLatin1String("rbg"))  return QColor(at(0), at(2), at(1));
    if (f == QLatin1String("rgba")) return QColor(at(0), at(1), at(2), at(3));
    if (f == QLatin1String("argb")) return QColor(at(1), at(2), at(3), at(0));
    if (f == QLatin1String("bgra")) return QColor(at(2), at(1), at(0), at(3));
    if (f == QLatin1String("abgr")) return QColor(at(3), at(2), at(1), at(0));

    return QColor(at(0), at(0), at(0));
}

} // namespace PixelDecoder
