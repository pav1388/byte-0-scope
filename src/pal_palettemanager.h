// Byte-0-scope - binary data visualizer
// File: pal_palettemanager.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QColor>
#include <QCoreApplication>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <functional>
#include <utility>

// Generator signature for a palette.
using PaletteGenFn = std::function<QVector<QColor>(const QVariantMap &)>;

//  PaletteDef — one entry in the palette registry.
struct PaletteDef {
    QString      id;              // PaletteIds::* value
    QString      name;            // user-visible name
    PaletteGenFn generator;       // color generator
    int          bpp;             // bpp mode this palette belongs to
    bool         configurable;    // shows an editor in the settings panel
    QVariantMap  defaultParams;   // initial params for the editor
    QVector<int> extraGroups;     // additional bpp groups to expose it in

    PaletteDef(const QString &_id, const QString &_name,
               PaletteGenFn _gen, int _bpp = 8,
               bool _configurable = false,
               const QVariantMap &_defaults = QVariantMap(),
               const QVector<int> &_extraGroups = QVector<int>())
        : id(_id), name(_name), generator(std::move(_gen)),
          bpp(_bpp), configurable(_configurable),
          defaultParams(_defaults), extraGroups(_extraGroups) {}
};

class PaletteManager {
    // Provides PaletteManager::tr() without inheriting QObject.
    // Required so lupdate recognizes the translation context of the
    // QCoreApplication::translate calls performed by the registration table.
    // Must stay before `public:` — the macro expands to `public: ... private:`.
    Q_DECLARE_TR_FUNCTIONS(PaletteManager)

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
    static QString defaultRuleColor(int index);
    static int averageRepeatCount(const quint64 *histogram);

private:
    struct Config {
        bool        configurable  = false;
        QVariantMap defaultParams;
        int         bytesPerPixel = 1;
    };

    void registerBasePalettes();
    void registerSampledPalettes();
    void registerPalette(const PaletteDef &def);

    QMap<int, QStringList>      m_groups;
    QMap<QString, Config>       m_configs;
    QMap<QString, QString>      m_names;
    QMap<QString, PaletteGenFn> m_generators;
};