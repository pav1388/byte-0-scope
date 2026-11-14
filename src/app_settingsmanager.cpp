// Byte-0-scope - binary data visualizer
// File: app_settingsmanager.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app_settingsmanager.h"

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
    const bool existed = QFileInfo(path).exists();
    QFile f(path);
    if (!f.open(existed ? QIODevice::WriteOnly | QIODevice::Append
                        : QIODevice::WriteOnly))
        return false;
    f.close();
    if (!existed)
        QFile::remove(path);
    return true;
}

} // namespace

SettingsManager::SettingsManager()
{
    // --- 1. Пытаемся положить .ini рядом с exe (portable) ---
    const QString exeDir = QCoreApplication::applicationDirPath();
    if (!exeDir.isEmpty()) {
        const QString exePath = exeDir + QLatin1Char('/')
                              + QStringLiteral(SM_INI_FILENAME);
        if (isWritableConfigPath(exePath)) {
            m_path = exePath;
            return;
        }
    }

    // --- 2. Fallback: пользовательский AppConfigLocation ---
    QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation);
    if (dir.isEmpty())
        dir = QDir::homePath() + QLatin1Char('/')
            + QStringLiteral(SM_FALLBACK_DIRNAME);
    QDir().mkpath(dir);
    m_path = dir + QLatin1Char('/') + QStringLiteral(SM_INI_FILENAME);
}

QVariantMap SettingsManager::load() const
{
    QSettings s(m_path, QSettings::IniFormat);
    s.setIniCodec("UTF-8");

    QVariantMap m;
    const QStringList keys = s.allKeys();
    for (const QString &key : keys)
        m.insert(key, s.value(key));
    return m;
}

bool SettingsManager::save(const QVariantMap &settings) const
{
    QSettings s(m_path, QSettings::IniFormat);
    s.setIniCodec("UTF-8");

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
        const QFileInfo fi(p);
        if (!fi.exists())
            continue;
        const QString key = fi.absoluteFilePath();
        if (seen.contains(key))
            continue;
        seen.insert(key);
        result.append(key);
        if (limit > 0 && result.size() >= limit)
            break;
    }
    return result;
}
