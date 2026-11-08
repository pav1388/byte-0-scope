// Byte-0-scope - binary data visualizer
// File: ui_mainwindowconfig.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_mainwindowconfig.h"
#include "app_settingsmanager.h"
#include "app_thememanager.h"
#include "definitions.h"
#include "pal_palettemanager.h"
#include "ui_byteinfo.h"
#include "ui_controlspanel.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>
#include <QScreen>
#include <QStatusBar>
#include <QVariantMap>

//  StartupDisplayDefaults
bool loadStartupDisplayDefaults(const QVariantMap &ini,
                                StartupDisplayDefaults *out)
{
    if (!out)
        return false;

    StartupDisplayDefaults s;

    s.hasAny =
        ini.contains(QStringLiteral(CFG_BPP_MODE)) ||
        ini.contains(QStringLiteral(CFG_PALETTE)) ||
        ini.contains(QStringLiteral(CFG_WIDTH_MODE)) ||
        ini.contains(QStringLiteral(CFG_SCALE)) ||
        ini.contains(QStringLiteral(CFG_BYTE_TOOLTIP));

    if (!s.hasAny) {
        *out = s;
        return false;
    }

    s.bppMode = ini.value(QStringLiteral(CFG_BPP_MODE),
                          CFG_DEF_BPP_MODE).toInt();

    s.paletteId = ini.value(QStringLiteral(CFG_PALETTE),
                            QStringLiteral(CFG_DEF_PALETTE)).toString();

    s.bitOrder = ini.value(QStringLiteral(CFG_BIT_ORDER),
                           QStringLiteral(CFG_DEF_BIT_ORDER)).toString();
    s.byteOrder = ini.value(QStringLiteral(CFG_BYTE_ORDER),
                            QStringLiteral(CFG_DEF_BYTE_ORDER)).toString();
    s.pixelOrder = ini.value(QStringLiteral(CFG_PIXEL_ORDER),
                             QStringLiteral(CFG_DEF_PIXEL_ORDER)).toString();
    s.rowPaddingBits = ini.value(QStringLiteral(CFG_ROW_PADDING_BITS),
                                 CFG_DEF_ROW_PADDING).toInt();

    s.scale = ini.value(QStringLiteral(CFG_SCALE),
                        CFG_DEF_SCALE).toInt();
    s.widthModeKey = ini.value(QStringLiteral(CFG_WIDTH_MODE),
                               QStringLiteral(CFG_DEF_WIDTH_MODE)).toString();
    s.fixedWidth = ini.value(QStringLiteral(CFG_FIXED_WIDTH),
                             CFG_DEF_FIXED_WIDTH).toInt();
    s.widthStep = ini.value(QStringLiteral(CFG_WIDTH_STEP),
                            CFG_DEF_WIDTH_STEP).toInt();
    s.bytesPerRow = ini.value(QStringLiteral(CFG_BYTES_PER_ROW),
                              CFG_DEF_BYTES_PER_ROW).toInt();

    s.autoScale = ini.value(QStringLiteral(CFG_AUTO_SCALE), false).toBool();
    s.fitToWindow = ini.value(QStringLiteral(CFG_FIT_TO_WINDOW), false).toBool();
    s.byteTooltip = ini.value(QStringLiteral(CFG_BYTE_TOOLTIP), false).toBool();

    *out = s;
    return true;
}

QVariantMap startupDefaultsToConfig(const StartupDisplayDefaults &s)
{
    QVariantMap cfg;
    cfg[QStringLiteral(CFG_BPP_MODE)]         = s.bppMode;
    cfg[QStringLiteral(CFG_PALETTE)]          = s.paletteId;
    cfg[QStringLiteral(CFG_BIT_ORDER)]        = s.bitOrder;
    cfg[QStringLiteral(CFG_BYTE_ORDER)]       = s.byteOrder;
    cfg[QStringLiteral(CFG_PIXEL_ORDER)]      = s.pixelOrder;
    cfg[QStringLiteral(CFG_ROW_PADDING_BITS)] = s.rowPaddingBits;
    cfg[QStringLiteral(CFG_SCALE)]            = s.scale;
    cfg[QStringLiteral(CFG_WIDTH_MODE)]       = s.widthModeKey;
    cfg[QStringLiteral(CFG_FIXED_WIDTH)]      = s.fixedWidth;
    cfg[QStringLiteral(CFG_WIDTH_STEP)]       = s.widthStep;
    cfg[QStringLiteral(CFG_BYTES_PER_ROW)]    = s.bytesPerRow;
    cfg[QStringLiteral(CFG_AUTO_SCALE)]       = s.autoScale;
    cfg[QStringLiteral(CFG_FIT_TO_WINDOW)]    = s.fitToWindow;
    return cfg;
}

//  .b0s per-file JSON reader
bool readConfigFile(const QString &path, QJsonObject *out, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseError);
    f.close();
    if (parseError.error != QJsonParseError::NoError) {
        if (error)
            *error = parseError.errorString();
        return false;
    }
    if (!doc.isObject()) {
        if (error)
            *error = QStringLiteral("not a JSON object");
        return false;
    }
    *out = doc.object();
    return true;
}

//  MainWindow — INI load/save
void MainWindow::loadSettings()
{
    const QVariantMap s = m_settings->load();
    if (s.isEmpty())
        return;

    {
        const QString themeStr = s.value("theme", "light").toString();
        const bool wantDark =
            (themeStr.compare("dark", Qt::CaseInsensitive) == 0);
        m_theme->setDark(wantDark);
    }

    m_lastDir               = s.value("last_directory").toString();
    m_perfSliderValue       = s.value("perf_slider_value", 10).toInt();
    m_loadIntoMemory        = s.value("load_into_memory", false).toBool();
    m_showDebugInfo         = s.value("show_debug_info", false).toBool();
    m_startupGeometryB64    = s.value("window_geometry").toString();
    m_startupNormalGeometry = s.value("window_normal_geometry").toString();
    m_startupMaximized      = s.value("window_maximized", false).toBool();
    m_startupFullscreen     = s.value("window_fullscreen", false).toBool();
	m_hexViewHeight         = s.value("hex_view_height", 0).toInt();
	m_hexBytesPerLine       = s.value("hex_bytes_per_line", 16).toInt();
	m_hexFontPt             = s.value("hex_font_pt", 9).toInt();
	m_hexViewVisible        = s.value("hex_view_visible", false).toBool();
    m_language = s.value("language",
                         QString::fromLatin1(AppLimits::LangEn)).toString();
    if (m_language.isEmpty())
        m_language = QString::fromLatin1(AppLimits::LangEn);

    const QStringList recents = s.value("recent_files").toStringList();
    m_recentFiles = SettingsManager::filterExisting(recents,
                                                    AppLimits::RecentMax);

    loadStartupDisplayDefaults(s, &m_startupDefaults);

    if (!m_startupDefaults.paletteId.isEmpty() &&
        !m_paletteManager->contains(m_startupDefaults.paletteId)) {
        m_startupDefaults.paletteId = QStringLiteral(CFG_DEF_PALETTE);
    }
}

void MainWindow::saveSettings()
{
    QVariantMap s;
    s["language"] = m_language;
    s["theme"]             = (m_theme && m_theme->isDark())
                                 ? QStringLiteral("dark")
                                 : QStringLiteral("light");
    s[QStringLiteral(CFG_BPP_MODE)]         = m_bppMode;
    s[QStringLiteral(CFG_BIT_ORDER)]        = m_bitOrder;
    s[QStringLiteral(CFG_BYTE_ORDER)]       = m_byteOrder;
    s[QStringLiteral(CFG_PIXEL_ORDER)]      = m_pixelOrder;
    s[QStringLiteral(CFG_ROW_PADDING_BITS)] = m_rowPaddingBits;
    s[QStringLiteral(CFG_SCALE)]            = m_scale;
    s[QStringLiteral(CFG_WIDTH_MODE)]       = widthModeKey(m_widthMode);
    s[QStringLiteral(CFG_BYTES_PER_ROW)]    = m_bytesPerRow;
    s[QStringLiteral(CFG_FIXED_WIDTH)]      = m_fixedWidth;
    s[QStringLiteral(CFG_WIDTH_STEP)]       = m_widthStep;
    s[QStringLiteral(CFG_AUTO_SCALE)]       = m_autoScale;
    s[QStringLiteral(CFG_FIT_TO_WINDOW)]    = m_fitToWindow;
    s[QStringLiteral(CFG_PALETTE)]          = m_currentPaletteId;
    s["perf_slider_value"] = m_perfSliderValue;
    s["load_into_memory"]  = m_loadIntoMemory;
    s["last_directory"]    = m_lastDir;
    s[QStringLiteral(CFG_BYTE_TOOLTIP)] = (m_byteInfo && m_byteInfo->isPopupEnabled());
    s["show_debug_info"]   = m_showDebugInfo;
    s["recent_files"]      = m_recentFiles;
    s["window_geometry"]   = QString::fromLatin1(saveGeometry().toBase64());
	s["hex_view_height"]    = m_hexViewHeight;
	s["hex_bytes_per_line"] = m_hexView ? m_hexView->bytesPerLine() : 16;
	s["hex_font_pt"]        = m_hexView ? m_hexView->fontPt() : 9;
	s["hex_view_visible"]   = m_hexViewAct && m_hexViewAct->isChecked();

    const QRect ng = normalGeometry();
    s["window_normal_geometry"] = QString("%1,%2,%3,%4")
        .arg(ng.x()).arg(ng.y()).arg(ng.width()).arg(ng.height());
    s["window_maximized"]  = isMaximized();
    s["window_fullscreen"] = isFullScreen();

    m_settings->save(s);
}

void MainWindow::restoreWindowGeometry()
{
    bool restored = false;

    if (!m_startupGeometryB64.isEmpty()) {
        const QByteArray geo =
            QByteArray::fromBase64(m_startupGeometryB64.toLatin1());
        if (!geo.isEmpty())
            restored = restoreGeometry(geo);
    }

    if (!restored && !m_startupNormalGeometry.isEmpty()) {
        const QStringList parts = m_startupNormalGeometry.split(',');
        if (parts.size() == 4) {
            bool okX, okY, okW, okH;
            const int x = parts[0].toInt(&okX);
            const int y = parts[1].toInt(&okY);
            const int w = parts[2].toInt(&okW);
            const int h = parts[3].toInt(&okH);
            if (okX && okY && okW && okH && w > 0 && h > 0)
                setGeometry(x, y, w, h);
        }
    }

    if (m_startupFullscreen)
        showFullScreen();
    else if (m_startupMaximized)
        showMaximized();

    const QRect frame = frameGeometry();
    bool onScreen = false;
    for (const QScreen *scr : QGuiApplication::screens()) {
        if (scr->availableGeometry().intersects(frame)) {
            onScreen = true;
            break;
        }
    }
    if (!onScreen) {
        const QRect avail =
            QGuiApplication::primaryScreen()->availableGeometry();
        const QSize sz = size();
        move(avail.center() - QPoint(sz.width() / 2, sz.height() / 2));
    }
}

//  MainWindow — .b0s per-file config (JSON)
QString MainWindow::configPathForFile(const QString &filePath) const
{
    if (filePath.isEmpty())
        return QString();
    if (!m_source.isOpen() || !m_xorLabel.isEmpty())
        return QString();
    return filePath + QLatin1String(".b0s");
}

QJsonObject MainWindow::configToJson() const
{
    QJsonObject c;
    c[QStringLiteral(CFG_VERSION)]          = m_version;
    c[QStringLiteral(CFG_BPP_MODE)]         = m_bppMode;
    c[QStringLiteral(CFG_BIT_ORDER)]        = m_bitOrder;
    c[QStringLiteral(CFG_PIXEL_ORDER)]      = m_pixelOrder;
    c[QStringLiteral(CFG_BYTE_ORDER)]       = m_byteOrder;
    c[QStringLiteral(CFG_ROW_PADDING_BITS)] = m_rowPaddingBits;
    c[QStringLiteral(CFG_SCALE)]            = m_scale;
    c[QStringLiteral(CFG_WIDTH_MODE)]       = widthModeKey(m_widthMode);
    c[QStringLiteral(CFG_BYTES_PER_ROW)]    = m_bytesPerRow;
    c[QStringLiteral(CFG_FIXED_WIDTH)]      = m_fixedWidth;
    c[QStringLiteral(CFG_WIDTH_STEP)]       = m_widthStep;
    c[QStringLiteral(CFG_AUTO_SCALE)]       = m_autoScale;
    c[QStringLiteral(CFG_FIT_TO_WINDOW)]    = m_fitToWindow;
    c[QStringLiteral(CFG_OFFSET)]           = qint64(m_offset);
    c[QStringLiteral(CFG_LENGTH)]           = qint64(m_length);
    c[QStringLiteral(CFG_LENGTH_MODE)]      = m_lengthMode;
    c[QStringLiteral(CFG_MIRROR_H)]         = m_mirrorH;
    c[QStringLiteral(CFG_MIRROR_V)]         = m_mirrorV;
    c[QStringLiteral(CFG_PALETTE)]          = m_currentPaletteId;

    QJsonObject pp;
    for (auto it = m_paletteParams.constBegin();
         it != m_paletteParams.constEnd(); ++it) {
        const QVariantMap m = it.value().toMap();
        QJsonObject sub;
        for (auto mi = m.constBegin(); mi != m.constEnd(); ++mi)
            sub[mi.key()] = QJsonValue::fromVariant(mi.value());
        pp[it.key()] = sub;
    }
    c[QStringLiteral(CFG_PALETTE_PARAMS)] = pp;
    return c;
}

bool MainWindow::applyConfigFromJson(const QJsonObject &c)
{
    if (c.isEmpty()) {
        statusBar()->showMessage(tr("The .b0s settings file is empty."), 7000);
        return false;
    }
	
	m_widthFromConfig = c.contains(QLatin1String(CFG_WIDTH_MODE))
						|| c.contains(QLatin1String(CFG_FIXED_WIDTH));

    setMirrorState(
        c.value(QStringLiteral(CFG_MIRROR_H)).toBool(m_mirrorH),
        c.value(QStringLiteral(CFG_MIRROR_V)).toBool(m_mirrorV));

    const QJsonObject pp = c.value(QStringLiteral(CFG_PALETTE_PARAMS)).toObject();
    for (auto it = pp.constBegin(); it != pp.constEnd(); ++it) {
        QVariantMap m;
        const QJsonObject sub = it.value().toObject();
        for (auto si = sub.constBegin(); si != sub.constEnd(); ++si)
            m[si.key()] = si.value().toVariant();
        const QString &paletteId = it.key();
        if (m_paletteManager->contains(paletteId))
            m_paletteParams[paletteId] = m;
    }

    QVariantMap vmap;
    for (auto it = c.constBegin(); it != c.constEnd(); ++it)
        vmap[it.key()] = it.value().toVariant();
    if (m_controls)
        m_controls->setConfig(vmap);

    const QString paletteId = c.value(QStringLiteral(CFG_PALETTE))
                               .toString(m_currentPaletteId);
    if (m_paletteManager->contains(paletteId))
        onPaletteChanged(paletteId);

    return true;
}

void MainWindow::saveCurrentConfig()
{
    if (!m_source.isOpen())
        return;
    const QString cfgPath = configPathForFile(m_source.sourcePath());
    if (cfgPath.isEmpty())
        return;

    QSaveFile f(cfgPath);
    if (!f.open(QIODevice::WriteOnly)
            || f.write(QJsonDocument(configToJson())
                           .toJson(QJsonDocument::Indented)) < 0
            || !f.commit()) {
        statusBar()->showMessage(
            tr("Could not save the .b0s settings: %1").arg(f.errorString()),
            7000);
        return;
    }
    statusBar()->showMessage(
        tr("Settings saved: %1").arg(QFileInfo(cfgPath).fileName()), 4000);
}

void MainWindow::loadConfigFromFile()
{
    if (!m_source.isOpen())
        return;
    const QString cfgPath = configPathForFile(m_source.sourcePath());
    if (cfgPath.isEmpty())
        return;
    if (!QFile::exists(cfgPath)) {
        statusBar()->showMessage(
            tr("No .b0s settings file found for this file."), 5000);
        return;
    }

    QJsonObject cfg;
    QString error;
    if (!readConfigFile(cfgPath, &cfg, &error)) {
        statusBar()->showMessage(
            tr("Could not read the .b0s settings: %1").arg(error), 7000);
        return;
    }
    applyConfigFromJson(cfg);
}