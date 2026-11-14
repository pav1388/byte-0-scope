// Byte-0-scope - binary data visualizer
// File: render_pngstream.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "render_pngstream.h"

#include <QSaveFile>
#include <QtEndian>

#include <zlib.h>

#include <climits>
#include <cstring>
#include <vector>

namespace PngStream {
namespace {

bool writeChunk(QIODevice &out, const char *type,
                const quint8 *data, int len)
{
    quint8 header[8];
    const quint32 lenBE = qToBigEndian<quint32>(quint32(len));
    std::memcpy(header, &lenBE, 4);
    std::memcpy(header + 4, type, 4);

    if (out.write(reinterpret_cast<const char *>(header), 8) != 8)
        return false;
    if (len > 0
            && out.write(reinterpret_cast<const char *>(data), len) != len)
        return false;

    quint32 crc = ::crc32(0, reinterpret_cast<const Bytef *>(type), 4);
    if (len > 0)
        crc = ::crc32(crc, data, len);
    const quint32 crcBE = qToBigEndian<quint32>(crc);
    return out.write(reinterpret_cast<const char *>(&crcBE), 4) == 4;
}

// Сжимает строки в zlib-поток и пишет его в IDAT-чанки порциями.
class IdatStream {
public:
    explicit IdatStream(QIODevice &out) : m_out(out)
    {
        std::memset(&m_z, 0, sizeof(m_z));
        m_idat.reserve(kFlushThreshold * 2);
    }
    ~IdatStream()
    {
        if (m_ready)
            deflateEnd(&m_z);
    }

    // Описание последней ошибки.
    const QString &error() const { return m_error; }

    bool begin()
    {
        m_ready = deflateInit2(&m_z, 6, Z_DEFLATED, 15, 8,
                               Z_DEFAULT_STRATEGY) == Z_OK;
        if (!m_ready)
            m_error = QStringLiteral("deflateInit2 failed");
        return m_ready;
    }

    bool write(quint8 *data, size_t size)
    {
        m_z.next_in  = data;
        m_z.avail_in = uInt(size);
        while (m_z.avail_in > 0) {
            quint8 temp[16 * 1024];
            m_z.next_out  = temp;
            m_z.avail_out = sizeof(temp);
            if (deflate(&m_z, Z_NO_FLUSH) != Z_OK)
                return fail(QStringLiteral("deflate error"));
            append(temp);
            if (int(m_idat.size()) >= kFlushThreshold && !flush())
                return false;
        }
        return true;
    }

    bool finish()
    {
        int rc = Z_OK;
        while (rc != Z_STREAM_END) {
            quint8 temp[16 * 1024];
            m_z.next_in   = nullptr;
            m_z.avail_in  = 0;
            m_z.next_out  = temp;
            m_z.avail_out = sizeof(temp);
            rc = deflate(&m_z, Z_FINISH);
            if (rc == Z_STREAM_ERROR)
                return fail(QStringLiteral("deflate finish error"));
            append(temp);
        }
        return flush();
    }

private:
    static constexpr int kFlushThreshold = 256 * 1024;

    bool fail(const QString &msg)
    {
        m_error = msg;
        return false;
    }

    void append(const quint8 *temp)
    {
        const int produced = 16 * 1024 - int(m_z.avail_out);
        if (produced > 0)
            m_idat.insert(m_idat.end(), temp, temp + produced);
    }

    bool flush()
    {
        if (m_idat.empty())
            return true;
        if (!writeChunk(m_out, "IDAT", m_idat.data(), int(m_idat.size())))
            return fail(m_out.errorString());
        m_idat.clear();
        return true;
    }

    QIODevice          &m_out;
    z_stream            m_z;
    bool                m_ready = false;
    std::vector<quint8> m_idat;
    QString             m_error;
};

int channels(ColorType type)
{
    return type == ColorType::Rgb ? 3 : type == ColorType::Rgba ? 4 : 1;
}

bool validFormat(const Format &f)
{
    if (f.type != ColorType::Indexed)
        return f.bitDepth == 8;
    const int n = f.palette.size();
    return (f.bitDepth == 1 || f.bitDepth == 2 || f.bitDepth == 4
            || f.bitDepth == 8)
        && n >= 1 && n <= (1 << f.bitDepth);
}

// Пишет весь PNG в out. При ошибке или отмене возвращает false и заполняет
// *errorMessage; откат результата - на вызывающем.
bool writeImage(QIODevice &out, const Format &format, int width, int height,
                size_t rowBytes, const RowProvider &provideRow,
                QString *errorMessage)
{
    auto fail = [errorMessage](const QString &msg) {
        if (errorMessage)
            *errorMessage = msg;
        return false;
    };

    static const quint8 kSignature[8] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
    };
    if (out.write(reinterpret_cast<const char *>(kSignature), 8) != 8)
        return fail(out.errorString());

    quint8 ihdr[13] = { 0 };
    const quint32 wBE = qToBigEndian<quint32>(quint32(width));
    const quint32 hBE = qToBigEndian<quint32>(quint32(height));
    std::memcpy(ihdr, &wBE, 4);
    std::memcpy(ihdr + 4, &hBE, 4);
    ihdr[8] = quint8(format.bitDepth);
    ihdr[9] = format.type == ColorType::Indexed ? 3
            : format.type == ColorType::Rgb     ? 2 : 6;
    if (!writeChunk(out, "IHDR", ihdr, 13))
        return fail(out.errorString());

    if (format.type == ColorType::Indexed) {
        std::vector<quint8> plte;
        plte.reserve(size_t(format.palette.size()) * 3);
        for (const QRgb c : format.palette) {
            plte.push_back(quint8(qRed(c)));
            plte.push_back(quint8(qGreen(c)));
            plte.push_back(quint8(qBlue(c)));
        }
        if (!writeChunk(out, "PLTE", plte.data(), int(plte.size())))
            return fail(out.errorString());
    }

    IdatStream idat(out);
    if (!idat.begin())
        return fail(idat.error());

    // Sub-фильтр; шаг - байты на пиксель, для глубины < 8 бит - 1 байт.
    const size_t stride = format.bitDepth < 8
        ? 1 : size_t(channels(format.type)) * size_t(format.bitDepth / 8);

    std::vector<quint8> row(1 + rowBytes);
    row[0] = 1;
    quint8 *pixels = row.data() + 1;

    for (int y = 0; y < height; ++y) {
        if (!provideRow(y, pixels)) {
            if (errorMessage && errorMessage->isEmpty())
                *errorMessage = QStringLiteral("canceled");
            return false;
        }

        // Справа налево, чтобы не затирать ещё не обработанные байты.
        for (size_t i = rowBytes; i-- > stride; )
            pixels[i] = quint8(pixels[i] - pixels[i - stride]);

        if (!idat.write(row.data(), row.size()))
            return fail(idat.error());
    }

    if (!idat.finish())
        return fail(idat.error());
    if (!writeChunk(out, "IEND", nullptr, 0))
        return fail(out.errorString());
    return true;
}

} // namespace

qint64 rowBytes(const Format &format, int width)
{
    if (width <= 0 || !validFormat(format))
        return 0;
    const qint64 bits = qint64(width) * format.bitDepth * channels(format.type);
    return (bits + 7) / 8;
}

bool write(const QString &path, const Format &format, int width, int height,
           const RowProvider &provideRow, QString *errorMessage)
{
    auto fail = [errorMessage](const QString &msg) {
        if (errorMessage)
            *errorMessage = msg;
        return false;
    };

    if (width <= 0 || height <= 0 || !provideRow || !validFormat(format))
        return fail(QStringLiteral("invalid PNG parameters"));

    const qint64 bytes = rowBytes(format, width);
    if (bytes <= 0 || bytes > qint64(INT_MAX - 1))
        return fail(QStringLiteral("PNG row is too wide for this build"));

    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly))
        return fail(out.errorString());

    if (!writeImage(out, format, width, height, size_t(bytes), provideRow,
                    errorMessage)) {
        out.cancelWriting();
        return false;
    }
    if (!out.commit())
        return fail(out.errorString());
    return true;
}

} // namespace PngStream
