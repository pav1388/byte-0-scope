// Byte-0-scope - binary data visualizer
// File: core_bytesource.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core_bytesource.h"
#include "qt_compat.h"

#include <QCoreApplication>
#include <limits>

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

// --- Настройки модуля BytesSource (префикс BS_) ---

// Резерв свободной памяти при загрузке файла целиком
#define BS_MEMORY_RESERVE_MIN       (512LL * 1024 * 1024)  // 512 МБ
#define BS_MEMORY_RESERVE_FRACTION  4                      // или 1/4 доступной

#define BS_BYTES_PER_KB             1024LL
#define BS_BYTES_PER_MB             (1024.0 * 1024.0)

namespace {

// Открывает файл на чтение и определяет его размер; при ошибке пишет
// сообщение в *error.
bool openForRead(QFile &file, qint64 *size, QString *error)
{
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QCoreApplication::translate(
                     "BytesSource", "Failed to open file:\n%1")
                     .arg(file.errorString());
        return false;
    }
    *size = file.size();
    if (*size < 0) {
        *error = QCoreApplication::translate(
            "BytesSource", "Failed to determine file size.");
        return false;
    }
    return true;
}

#ifdef Q_OS_LINUX
// Значение строки /proc/meminfo вида "Name:   1234 kB" в байтах.
qint64 meminfoBytes(const QByteArray &line, int prefixLen)
{
    return line.mid(prefixLen).trimmed().split(' ').value(0).toLongLong()
           * BS_BYTES_PER_KB;
}
#endif

} // namespace

qint64 BytesSource::availableMemory()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        // 32-bit process: the limit is the free address space (2 GB, or up to
        // 4 GB with /LARGEADDRESSAWARE), not the amount of free RAM.
        // On a 64-bit process ullAvailVirtual is huge, so min() = RAM.
        return qint64(qMin<quint64>(ms.ullAvailPhys, ms.ullAvailVirtual));
    }
    return 0;
#elif defined(Q_OS_LINUX)
    QFile f(QStringLiteral("/proc/meminfo"));
    if (!f.open(QIODevice::ReadOnly))
        return 0;

    qint64 memAvailable = -1, memFree = -1, buffers = 0, cached = 0;
    while (!f.atEnd()) {
        const QByteArray line = f.readLine();
        if (line.startsWith("MemAvailable:"))      memAvailable = meminfoBytes(line, 13);
        else if (line.startsWith("MemFree:"))      memFree      = meminfoBytes(line, 8);
        else if (line.startsWith("Buffers:"))      buffers      = meminfoBytes(line, 8);
        else if (line.startsWith("Cached:"))       cached       = meminfoBytes(line, 7);
    }

    if (memAvailable > 0)
        return memAvailable;
    if (memFree > 0)
        return memFree + buffers + cached;
    return 0;
#else
    return 0;
#endif
}

bool BytesSource::open(const QString &path, bool loadIntoMemory)
{
    close();
    return loadIntoMemory ? openInMemory(path) : openMapped(path);
}

bool BytesSource::openMapped(const QString &path)
{
    const auto mf = std::make_shared<MappedFile>(path);
    if (!openForRead(mf->file, &mf->size, &m_lastError))
        return false;

    // Пустой файл отображать нечего: источник открыт, но без mmap.
    if (mf->size > 0) {
        mf->mapped = mf->file.map(0, mf->size);
        if (!mf->mapped) {
            m_lastError = QCoreApplication::translate(
                "BytesSource",
                "Failed to map the file into memory.\n\n"
                "File size: %1 MB\n"
                "Reason: %2\n\n"
                "Possible solutions:\n"
                "• Enable \"Load whole file into memory\" in the settings\n"
                "• Restart the program\n"
                "• Check free space in the swap file")
                .arg(mf->size / BS_BYTES_PER_MB, 0, 'f', 1)
                .arg(mf->file.errorString());
            if (QtCompat::is32BitProcess()) {
                m_lastError += QCoreApplication::translate(
                    "BytesSource",
                    "\n\nThis is a 32-bit build: it can address only 2-4 GB of "
                    "memory in total. Use the 64-bit build to open large files.");
            }
            return false;
        }
        m_mapped = mf;
    }

    m_path = path;
    m_open = true;
    return true;
}

bool BytesSource::openInMemory(const QString &path)
{
    QFile f(path);
    qint64 fileSize = 0;
    if (!openForRead(f, &fileSize, &m_lastError))
        return false;

    const qint64 qbaMax = QtCompat::byteArrayMaxSize();
    if (fileSize > qbaMax) {
        m_lastError = QCoreApplication::translate(
            "BytesSource",
            "The file is too large to load into memory.\n\n"
            "File size:    %1 MB (%2 bytes)\n"
            "QByteArray max: %3 MB (%4 bytes)\n\n"
            "Disable \"Load whole file into memory\" in the settings — "
            "the program will use mmap and can open this file.")
            .arg(fileSize / BS_BYTES_PER_MB, 0, 'f', 1).arg(fileSize)
            .arg(qbaMax   / BS_BYTES_PER_MB, 0, 'f', 1).arg(qbaMax);
        return false;
    }

    const qint64 available = availableMemory();
    if (available > 0) {
        const qint64 reserve = qMax<qint64>(BS_MEMORY_RESERVE_MIN,
                                            available / BS_MEMORY_RESERVE_FRACTION);
        if (fileSize + reserve > available) {
            m_lastError = QCoreApplication::translate(
                "BytesSource",
                "Not enough free memory to load the file.\n\n"
                "File size:    %1 MB\n"
                "Memory available: %2 MB\n"
                "Reserve needed:   %3 MB\n\n"
                "Disable \"Load whole file into memory\" in the settings "
                "or close other applications.")
                .arg(fileSize  / BS_BYTES_PER_MB, 0, 'f', 1)
                .arg(available / BS_BYTES_PER_MB, 0, 'f', 1)
                .arg(reserve   / BS_BYTES_PER_MB, 0, 'f', 1);
            return false;
        }
    }

    m_data = f.readAll();
    if (m_data.size() != fileSize) {
        m_lastError = QCoreApplication::translate(
            "BytesSource",
            "File read error.\n"
            "Read %1 of %2 bytes.\n%3")
            .arg(m_data.size()).arg(fileSize).arg(f.errorString());
        m_data.clear();
        return false;
    }

    m_path = path;
    m_open = true;
    return true;
}

qint64 BytesSource::size() const
{
    return m_mapped ? m_mapped->size : qint64(m_data.size());
}

void BytesSource::close()
{
    m_mapped.reset();
    m_data.clear();
    m_path.clear();
    m_lastError.clear();
    m_open = false;
}

QByteArray BytesSource::data() const
{
    if (!m_mapped)
        return m_data;
    if (m_mapped->size > std::numeric_limits<int>::max())
        return QByteArray();
    return QByteArray::fromRawData(
        reinterpret_cast<const char *>(m_mapped->mapped), int(m_mapped->size));
}

quint8 BytesSource::at(qint64 offset) const
{
    if (offset < 0 || offset >= size())
        return 0;
    return rawData()[offset];
}

QByteArray BytesSource::mid(qint64 offset, qint64 length) const
{
    if (length <= 0 || offset < 0 || offset >= size())
        return QByteArray();

    // QByteArray size is int in Qt 5: never let it wrap around.
    const int take = int(qMin(length, qint64(std::numeric_limits<int>::max())));
    if (m_mapped) {
        return QByteArray(reinterpret_cast<const char *>(m_mapped->mapped) + offset,
                          int(qMin<qint64>(take, m_mapped->size - offset)));
    }
    // Не int(length): оно может стать отрицательным, а mid(pos, -1) вернёт
    // всё до конца. mid() сам обрежет длину и разделит буфер без копирования.
    return m_data.mid(int(offset), take);
}

const uchar *BytesSource::rawData() const
{
    return m_mapped ? m_mapped->mapped
                    : reinterpret_cast<const uchar *>(m_data.constData());
}

void BytesSource::setData(const QByteArray &data)
{
    m_mapped.reset();
    m_data = data;
    m_path.clear();
    m_lastError.clear();
    m_open = true;
}