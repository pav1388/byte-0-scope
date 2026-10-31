// Byte-0-scope - binary data visualizer
// File: bytesource.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QFile>
#include <QString>

#include <memory>

struct MappedFile {
    QFile  file;
    uchar *mapped = nullptr;
    qint64 size   = 0;

    explicit MappedFile(const QString &path) : file(path) {}
    ~MappedFile() { if (mapped) file.unmap(mapped); }
};

using MappedFilePtr = std::shared_ptr<MappedFile>;

class BytesSource {
public:
    BytesSource();
    ~BytesSource();

    bool    open(const QString &path, bool loadIntoMemory = false);
    void    close();
    bool    isOpen() const { return m_open; }

    qint64  size() const;

    quint8  at(qint64 offset) const;
    QByteArray mid(qint64 offset, qint64 length) const;

    const uchar *rawData() const;
    QByteArray   data() const;
    MappedFilePtr mapped() const { return m_mapped; }

    void    setData(const QByteArray &data);

    QString sourcePath() const { return m_path; }
    QString lastError() const  { return m_lastError; }

    static qint64 availableMemory();

private:
    MappedFilePtr m_mapped;
    QByteArray    m_data;
    QString       m_path;
    QString       m_lastError;
    bool          m_open = false;
};