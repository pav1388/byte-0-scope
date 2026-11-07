// Byte-0-scope - binary data visualizer
// File: app_settingsmanager.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>

// Загрузка/сохранение пользовательских настроек в INI-файл.
// Приоритет: рядом с exe; если каталог недоступен для записи — appdata.
class SettingsManager {
public:
    SettingsManager();
    QVariantMap load() const;
    bool save(const QVariantMap &settings) const;
    static QStringList filterExisting(const QStringList &paths, int limit = -1);
    QString configPath() const { return m_path; }

private:
    QString m_path;
};