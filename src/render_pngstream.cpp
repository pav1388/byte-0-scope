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

} // namespace

bool writeRgba(const QString &path, int width, int height,
               const RowProvider &provideRow, QString *errorMessage)
{
    auto fail = [errorMessage](const QString &msg) {
        if (errorMessage)
            *errorMessage = msg;
        return false;
    };

    if (width <= 0 || height <= 0 || !provideRow)
        return fail(QStringLiteral("invalid PNG dimensions"));

    const qint64 rowBytes = qint64(width) * 4;
    if (rowBytes <= 0 || rowBytes > qint64(INT_MAX - 1))
        return fail(QStringLiteral("PNG row is too wide for this build"));

    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly))
        return fail(out.errorString());

    // PNG signature
    static const quint8 kSignature[8] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
    };
    if (out.write(reinterpret_cast<const char *>(kSignature), 8) != 8) {
        out.cancelWriting();
        return fail(out.errorString());
    }

    // IHDR: RGBA, 8 bit, no interlace
    {
        quint8 ihdr[13];
        const quint32 wBE = qToBigEndian<quint32>(quint32(width));
        const quint32 hBE = qToBigEndian<quint32>(quint32(height));
        std::memcpy(ihdr, &wBE, 4);
        std::memcpy(ihdr + 4, &hBE, 4);
        ihdr[8]  = 8;   // bit depth
        ihdr[9]  = 6;   // color type = RGBA
        ihdr[10] = 0;   // compression
        ihdr[11] = 0;   // filter
        ihdr[12] = 0;   // interlace = none
        if (!writeChunk(out, "IHDR", ihdr, 13)) {
            out.cancelWriting();
            return fail(out.errorString());
        }
    }

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    if (deflateInit2(&strm, 6, Z_DEFLATED, 15, 8,
                     Z_DEFAULT_STRATEGY) != Z_OK) {
        out.cancelWriting();
        return fail(QStringLiteral("deflateInit2 failed"));
    }

    constexpr int kIdatThreshold = 256 * 1024;
    std::vector<quint8> idat;
    idat.reserve(kIdatThreshold * 2);
    std::vector<quint8> row(1 + size_t(rowBytes));

    auto flushIdat = [&]() -> bool {
        if (idat.empty())
            return true;
        if (!writeChunk(out, "IDAT", idat.data(), int(idat.size()))) {
            out.cancelWriting();
            return false;
        }
        idat.clear();
        return true;
    };

    bool ok = true;

    for (int y = 0; y < height && ok; ++y) {
        row[0] = 1;  // filter type: Sub

        if (!provideRow(y, row.data() + 1)) {
            ok = false;
            if (errorMessage && errorMessage->isEmpty())
                *errorMessage = QStringLiteral("canceled");
            break;
        }

        // Sub filter (right-to-left, чтобы не затирать ещё не обработанные)
        quint8 *p = row.data() + 1;
        for (int i = int(rowBytes) - 1; i >= 4; --i)
            p[i] = quint8(p[i] - p[i - 4]);

        strm.next_in  = row.data();
        strm.avail_in = uInt(row.size());

        while (strm.avail_in > 0 && ok) {
            quint8 temp[16 * 1024];
            strm.next_out  = temp;
            strm.avail_out = sizeof(temp);
            if (deflate(&strm, Z_NO_FLUSH) != Z_OK) {
                ok = false;
                if (errorMessage)
                    *errorMessage = QStringLiteral("deflate error");
                break;
            }
            const int produced = int(sizeof(temp)) - int(strm.avail_out);
            if (produced > 0)
                idat.insert(idat.end(), temp, temp + produced);
            if (int(idat.size()) >= kIdatThreshold) {
                if (!flushIdat()) {
                    ok = false;
                    if (errorMessage)
                        *errorMessage = out.errorString();
                    break;
                }
            }
        }
    }

    if (ok) {
        // Закрываем zlib-поток
        while (true) {
            quint8 temp[16 * 1024];
            strm.next_in   = nullptr;
            strm.avail_in  = 0;
            strm.next_out  = temp;
            strm.avail_out = sizeof(temp);
            const int rc = deflate(&strm, Z_FINISH);
            if (rc == Z_STREAM_ERROR) {
                ok = false;
                if (errorMessage)
                    *errorMessage = QStringLiteral("deflate finish error");
                break;
            }
            const int produced = int(sizeof(temp)) - int(strm.avail_out);
            if (produced > 0)
                idat.insert(idat.end(), temp, temp + produced);
            if (rc == Z_STREAM_END)
                break;
        }
        if (ok && !flushIdat()) {
            ok = false;
            if (errorMessage)
                *errorMessage = out.errorString();
        }
    }

    deflateEnd(&strm);

    if (ok) {
        if (!writeChunk(out, "IEND", nullptr, 0)) {
            ok = false;
            if (errorMessage)
                *errorMessage = out.errorString();
        }
    }

    if (!ok) {
        out.cancelWriting();
        return false;
    }

    if (!out.commit())
        return fail(out.errorString());

    return true;
}

} // namespace PngStream