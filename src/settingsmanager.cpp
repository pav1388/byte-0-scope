// Byte-0-scope - binary data visualizer
// File: settingsmanager.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingsmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>

// --- Настройки модуля SettingsManager (префикс SM_) ---

#define SM_INI_FILENAME     "byte0scope-settings.ini"
#define SM_FALLBACK_DIRNAME ".byte-0-scope"

namespace {

// Проверяет, можно ли писать в путь (или создать файл по этому пути).
bool isWritableConfigPath(const QString &path)
{
    QFileInfo fi(path);
    if (fi.exists()) {
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
            f.close();
            return true;
        }
        return false;
    }
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.close();
        QFile::remove(path);
        return true;
    }
    return false;
}

} // namespace

SettingsManager::SettingsManager()
{
    // --- 1. Пытаемся положить .ini рядом с exe (portable) ---
    const QString exeDir  = QCoreApplication::applicationDirPath();
    const QString exePath = exeDir + "/" + QStringLiteral(SM_INI_FILENAME);

    if (!exeDir.isEmpty() && isWritableConfigPath(exePath)) {
        m_path = exePath;
        return;
    }

    // --- 2. Fallback: пользовательский AppConfigLocation ---
    QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation);
    if (dir.isEmpty())
        dir = QDir::homePath() + "/" + QStringLiteral(SM_FALLBACK_DIRNAME);
    QDir().mkpath(dir);
    m_path = dir + "/" + QStringLiteral(SM_INI_FILENAME);
}

QVariantMap SettingsManager::load() const
{
    QSettings s(m_path, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    s.setIniCodec("UTF-8");
#endif

    QVariantMap m;
    const QStringList keys = s.allKeys();
    for (const QString &key : keys)
        m[key] = s.value(key);
    return m;
}

bool SettingsManager::save(const QVariantMap &settings) const
{
    QSettings s(m_path, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    s.setIniCodec("UTF-8");
#endif

    s.clear();
    for (auto it = settings.constBegin(); it != settings.constEnd(); ++it)
        s.setValue(it.key(), it.value());
    s.sync();
    return s.status() == QSettings::NoError;
}

QStringList SettingsManager::filterExisting(const QStringList &paths, int limit)
{
    QStringList result;
    QSet<QString> seen;
    for (const QString &p : paths) {
        QFileInfo fi(p);
        if (!fi.exists())
            continue;
        const QString key = fi.absoluteFilePath();
        if (seen.contains(key))
            continue;
        seen.insert(key);
        result.append(key);
    }
    if (limit > 0 && result.size() > limit)
        result = result.mid(0, limit);
    return result;
}