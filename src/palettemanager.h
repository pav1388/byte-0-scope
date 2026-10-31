// Byte-0-scope - binary data visualizer
// File: palettemanager.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QColor>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <functional>

class PaletteManager {
public:
    PaletteManager();

    QStringList paletteIdsForBpp(int bpp) const;
    QStringList allPaletteIds() const;
    QString     displayName(const QString &id) const;
    bool        contains(const QString &id) const;

    int         bytesPerPixel(const QString &id) const;
    QVariantMap defaultParams(const QString &id) const;
    bool        isConfigurable(const QString &id) const;

    QVector<QColor> createPalette(const QString &id,
                                  const QVariantMap &params,
                                  const QByteArray &fileData = QByteArray(),
                                  const quint64 *byteHistogram = nullptr) const;

    static QVariantList defaultCustomRules();
    static QVariantList samplePalette(const QVariantList &source, int n);

private:
    typedef std::function<QVector<QColor>(const QVariantMap &)> GenFn;

    struct Config {
        bool        configurable  = false;
        QVariantMap defaultParams;
        int         bytesPerPixel = 1;
    };

    void registerBaseGenerators();
    void registerBaseConfigs();
    void registerSampledPalettes();

    QMap<int, QStringList> m_groups;
    QMap<QString, Config>  m_configs;
    QMap<QString, QString> m_names;
    QMap<QString, GenFn>   m_generators;
};