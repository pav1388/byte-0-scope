// Byte-0-scope - binary data visualizer
// File: bytesource.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bytesource.h"

#include <QFile>
#include <QFileInfo>
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

BytesSource::BytesSource() {}
BytesSource::~BytesSource() { close(); }

qint64 BytesSource::availableMemory()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms))
        return qint64(ms.ullAvailPhys);
    return 0;
#elif defined(Q_OS_LINUX)
    QFile f(QStringLiteral("/proc/meminfo"));
    if (!f.open(QIODevice::ReadOnly))
        return 0;

    qint64 memAvailable = -1, memFree = -1, buffers = 0, cached = 0;
    while (!f.atEnd()) {
        const QByteArray line = f.readLine();
        auto value = [&](int prefixLen) -> qint64 {
            return line.mid(prefixLen).trimmed().split(' ').value(0).toLongLong()
                   * BS_BYTES_PER_KB;
        };
        if (line.startsWith("MemAvailable:"))      memAvailable = value(13);
        else if (line.startsWith("MemFree:"))      memFree      = value(8);
        else if (line.startsWith("Buffers:"))      buffers      = value(8);
        else if (line.startsWith("Cached:"))       cached       = value(7);
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
    m_lastError.clear();

    if (!loadIntoMemory) {
        // ---------- mmap (по умолчанию) ----------
        auto mf = std::make_shared<MappedFile>(path);

        if (!mf->file.open(QIODevice::ReadOnly)) {
            m_lastError = QStringLiteral("Не удалось открыть файл:\n%1")
                              .arg(mf->file.errorString());
            return false;
        }

        mf->size = mf->file.size();
        if (mf->size < 0) {
            m_lastError = QStringLiteral("Не удалось определить размер файла.");
            return false;
        }
        if (mf->size == 0) {
            m_path = path;
            m_open = true;
            return true;
        }

        mf->mapped = mf->file.map(0, mf->size);
        if (!mf->mapped) {
            m_lastError = QStringLiteral(
                "Не удалось отобразить файл в память.\n\n"
                "Размер файла: %1 МБ\n"
                "Причина: %2\n\n"
                "Возможные решения:\n"
                "• Включите «Загружать файл целиком в память» в настройках\n"
                "• Перезапустите программу\n"
                "• Проверьте свободное место в файле подкачки")
                .arg(mf->size / BS_BYTES_PER_MB, 0, 'f', 1)
                .arg(mf->file.errorString());
            mf->file.close();
            return false;
        }

        m_mapped = mf;
        m_path   = path;
        m_open   = true;
        return true;
    }

    // ---------- readAll (как demo/XOR) ----------
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    const qint64 qbaMax = qint64(QByteArray::maxSize());
#else
    const qint64 qbaMax = qint64(std::numeric_limits<int>::max());
#endif

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        m_lastError = QStringLiteral("Не удалось открыть файл:\n%1")
                          .arg(f.errorString());
        return false;
    }

    const qint64 fileSize = f.size();
    if (fileSize < 0) {
        m_lastError = QStringLiteral("Не удалось определить размер файла.");
        return false;
    }

    if (fileSize > qbaMax) {
        m_lastError = QStringLiteral(
            "Файл слишком большой для загрузки в память.\n\n"
            "Размер файла:    %1 МБ (%2 байт)\n"
            "Максимум QByteArray: %3 МБ (%4 байт)\n\n"
            "Отключите «Загружать файл целиком в память» в настройках — "
            "программа будет использовать mmap и сможет открыть этот файл.")
            .arg(fileSize / BS_BYTES_PER_MB, 0, 'f', 1).arg(fileSize)
            .arg(qbaMax   / BS_BYTES_PER_MB, 0, 'f', 1).arg(qbaMax);
        return false;
    }

    const qint64 available = availableMemory();
    if (available > 0) {
        const qint64 reserve = qMax<qint64>(BS_MEMORY_RESERVE_MIN,
                                            available / BS_MEMORY_RESERVE_FRACTION);
        if (fileSize + reserve > available) {
            m_lastError = QStringLiteral(
                "Недостаточно свободной памяти для загрузки файла.\n\n"
                "Размер файла:    %1 МБ\n"
                "Доступно памяти: %2 МБ\n"
                "Нужен резерв:    %3 МБ\n\n"
                "Отключите «Загружать файл целиком в память» в настройках "
                "или закройте другие приложения.")
                .arg(fileSize  / BS_BYTES_PER_MB, 0, 'f', 1)
                .arg(available / BS_BYTES_PER_MB, 0, 'f', 1)
                .arg(reserve   / BS_BYTES_PER_MB, 0, 'f', 1);
            return false;
        }
    }

    m_data = f.readAll();
    if (m_data.size() != fileSize) {
        m_lastError = QStringLiteral(
            "Ошибка чтения файла.\n"
            "Прочитано %1 из %2 байт.\n%3")
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
    if (m_mapped)
        return m_mapped->size;
    return m_data.size();
}

void BytesSource::close()
{
    m_mapped.reset();      // отображение снимется, когда последний
                           // RenderContext отпустит shared_ptr
    m_data.clear();
    m_path.clear();
    m_lastError.clear();
    m_open = false;
}

QByteArray BytesSource::data() const
{
    if (m_mapped && m_mapped->mapped) {
        const qint64 sz = m_mapped->size;
        if (sz > std::numeric_limits<int>::max()) {
            // fromRawData принимает int — файлы >2 ГБ через QByteArray
            // не представить. Рендер всё равно работает: он читает
            // через mapped()->mapped.
            return QByteArray();
        }
        return QByteArray::fromRawData(
            reinterpret_cast<const char *>(m_mapped->mapped), int(sz));
    }
    return m_data;
}

quint8 BytesSource::at(qint64 offset) const
{
    if (offset < 0)
        return 0;
    if (m_mapped && m_mapped->mapped) {
        if (offset >= m_mapped->size)
            return 0;
        return m_mapped->mapped[offset];
    }
    if (offset >= m_data.size())
        return 0;
    return quint8(m_data.at(int(offset)));
}

QByteArray BytesSource::mid(qint64 offset, qint64 length) const
{
    if (length <= 0 || offset < 0)
        return QByteArray();

    if (m_mapped && m_mapped->mapped) {
        if (offset >= m_mapped->size)
            return QByteArray();
        const qint64 avail = m_mapped->size - offset;
        const qint64 take  = qMin(length, avail);
        return QByteArray(reinterpret_cast<const char *>(m_mapped->mapped) + offset,
                          int(take));
    }
    if (offset >= m_data.size())
        return QByteArray();
    return m_data.mid(int(offset), int(length));
}

const uchar *BytesSource::rawData() const
{
    if (m_mapped && m_mapped->mapped)
        return m_mapped->mapped;
    return reinterpret_cast<const uchar *>(m_data.constData());
}

void BytesSource::setData(const QByteArray &data)
{
    m_mapped.reset();
    m_data = data;
    m_path.clear();
    m_lastError.clear();
    m_open = true;
}