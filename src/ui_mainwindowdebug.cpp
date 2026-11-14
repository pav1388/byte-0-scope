// Byte-0-scope - binary data visualizer
// File: ui_mainwindowdebug.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

// Debug-panel implementation for MainWindow.

#include "ui_mainwindow.h"
#include "app_thememanager.h"
#include "definitions.h"
#include "render_canvaswidget.h"
#include "render_tilemanager.h"
#include "ui_byteinfo.h"

#include <QAction>
#include <QGroupBox>
#include <QString>
#include <QStringList>

// -------------------------------------------------------------------------
//  Helpers
// -------------------------------------------------------------------------

namespace {

QString formatSize(qint64 bytes)
{
    constexpr double KB = 1024.0;
    constexpr double MB = 1024.0 * 1024.0;
    constexpr double GB = 1024.0 * 1024.0 * 1024.0;

    if (bytes >= qint64(GB))
        return QString::number(bytes / GB, 'f', 2)
               + QLatin1Char(' ') + MainWindow::tr("GB");
    if (bytes >= qint64(MB))
        return QString::number(bytes / MB, 'f', 1)
               + QLatin1Char(' ') + MainWindow::tr("MB");
    if (bytes >= qint64(KB))
        return QString::number(bytes / KB, 'f', 1)
               + QLatin1Char(' ') + MainWindow::tr("KB");
    return QString::number(bytes) + QLatin1Char(' ') + MainWindow::tr("B");
}

QString formatBool(bool b)
{
    return b ? QStringLiteral("true") : QStringLiteral("false");
}

// «key = value»
QString kv(const char *key, const QString &value)
{
    return QString::fromLatin1(key) + QStringLiteral(" = ") + value;
}
QString kvNum(const char *key, qint64 value)
{
    return kv(key, QString::number(value));
}
QString kvBool(const char *key, bool value)
{
    return kv(key, formatBool(value));
}

} // namespace

void MainWindow::onDebugInfoToggled(bool enabled)
{
    m_showDebugInfo = enabled;
    if (m_debugGroup)
        m_debugGroup->setVisible(enabled);
    if (enabled)
        updateDebugInfo();
}

void MainWindow::updateDebugInfo()
{
    if (!m_showDebugInfo || !m_debugLabel)
        return;

    QString mode;
    if (!m_source.isOpen())
        mode = QStringLiteral("none");
    else if (!m_xorLabel.isEmpty())
        mode = QStringLiteral("xor");
    else if (!m_source.sourcePath().isEmpty())
        mode = m_source.mapped() ? QStringLiteral("mmap")
                                 : QStringLiteral("memory");
    else
        mode = QStringLiteral("demo");

    qint64 cacheB     = 0;
    int    activeT    = 0;
    int    cacheLimit = 0;
    if (m_canvas && m_canvas->tileManager()) {
        TileManager *tm = m_canvas->tileManager();
        cacheB     = tm->cacheBytes();
        activeT    = tm->activeTasks();
        cacheLimit = tm->cacheLimitMb();
    }

    QStringList L;
    L.reserve(64);

    // ────── Source ──────
    L << QStringLiteral("SOURCE:");
    L << kv("file_mode", mode);
    L << kv("file_size", QString::number(m_fileSize) + QStringLiteral(" (")
                         + formatSize(m_fileSize) + QLatin1Char(')'));
    L << kvNum("offset", m_offset);
    L << kvNum("length", m_length);
    L << kvBool("load_into_memory", m_loadIntoMemory);
    L << kv("source_path", m_source.sourcePath().isEmpty()
                               ? QStringLiteral("(none)")
                               : m_source.sourcePath());
    if (!m_xorLabel.isEmpty())
        L << kv("xor_label", m_xorLabel);

    // ────── Pixel format ──────
    L << QString() << QStringLiteral("PIXEL_FORMAT:");
    L << kvNum("bpp_mode", m_bppMode);
    L << kvBool("is_bitpacked", isBitpacked());
    L << kvNum("bytes_per_pixel", bytesPerPixel());
    L << kv("bit_order", m_bitOrder);
    L << kv("byte_order", m_byteOrder);
    L << kv("pixel_order", m_pixelOrder);
    L << kvNum("row_padding_bits", m_rowPaddingBits);

    // ────── Geometry ──────
    L << QString() << QStringLiteral("GEOMETRY:");
    L << kv("width_mode", widthModeKey(m_widthMode));
    L << kvNum("fixed_width", m_fixedWidth);
    L << kvNum("width_step", m_widthStep);
    L << kvBool("width_from_config", m_widthFromConfig);
    L << kvNum("bytes_per_row", m_bytesPerRow);
    L << kv("image_size", QString::number(m_imageWidth) + QStringLiteral(" × ")
                          + QString::number(m_imageHeight));
    L << kv("viewport_rect", QStringLiteral("%1,%2  %3×%4")
                                 .arg(m_viewportRect.x()).arg(m_viewportRect.y())
                                 .arg(m_viewportRect.width())
                                 .arg(m_viewportRect.height()));

    // ────── View / zoom ──────
    L << QString() << QStringLiteral("VIEW/ZOOM:");
    L << kvNum("scale", m_scale);
    L << kvNum("downsample", m_downsample);
    L << kvBool("fit_to_window", m_fitToWindow);
    L << kvBool("auto_scale", m_autoScale);
    L << kvBool("mirror_h", m_mirrorH);
    L << kvBool("mirror_v", m_mirrorV);
    L << kv("hover_addr", m_hoverAddr < 0 ? QStringLiteral("-1 (none)")
                                          : QString::number(m_hoverAddr));

    // ────── Tiles / cache ──────
    L << QString() << QStringLiteral("TILES/CACHE:");
    L << kvNum("tile_size", m_tileSize);
    L << kvNum("tile_prefetch_margin", m_tilePrefetchMargin);
    L << kvNum("cache_limit_mb", cacheLimit);
    L << kv("cache_bytes", QString::number(cacheB) + QStringLiteral(" (")
                           + formatSize(cacheB) + QLatin1Char(')'));
    L << kvNum("active_tasks", activeT);
    L << kvNum("perf_slider_value", m_perfSliderValue);

    // ────── Palette / histogram ──────
    L << QString() << QStringLiteral("PALETTE:");
    L << kv("palette", m_currentPaletteId.isEmpty() ? QStringLiteral("(none)")
                                                    : m_currentPaletteId);
    L << kvNum("palette_colors", m_currentPalette.size());
    L << kvNum("palette_params_count", m_paletteParams.size());
    L << kvBool("histogram_valid", m_byteHistogramValid);
    L << kvBool("auto_apply_palette", m_autoApplyPalette);

    // ────── Hex view ──────
    L << QString() << QStringLiteral("HEX_VIEW:");
    L << kvBool("menu_checked", m_hexViewAct && m_hexViewAct->isChecked());
    L << kvBool("widget_visible", m_hexView && m_hexView->isVisible());
    L << kv("widget_height", m_hexView ? QString::number(m_hexView->height())
                                       : QStringLiteral("(none)"));
    L << kvNum("saved_height", m_hexViewHeight);
    L << kvNum("bytes_per_line",
               m_hexView ? m_hexView->bytesPerLine() : m_hexBytesPerLine);
    L << kvNum("font_pt", m_hexView ? m_hexView->fontPt() : m_hexFontPt);

    // ────── Other ──────
    L << QString() << QStringLiteral("OTHER:");
    L << kv("language", m_language);
    L << kv("theme", m_theme && m_theme->isDark() ? QStringLiteral("dark")
                                                  : QStringLiteral("light"));
    L << kvBool("busy_png_export", m_busyPngExport);
    L << kvBool("byte_tooltip", m_byteInfo && m_byteInfo->isPopupEnabled());
    L << kvBool("show_debug_info", m_showDebugInfo);
    L << kvNum("recent_files", m_recentFiles.size());

    m_debugLabel->setText(L.join(QStringLiteral("<br>")));
}
