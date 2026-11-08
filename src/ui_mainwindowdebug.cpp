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

    const QString widthModeStr = widthModeKey(m_widthMode);

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
    L << QStringLiteral("file_mode = ") + mode;
    L << QStringLiteral("file_size = ") + QString::number(m_fileSize)
         + QStringLiteral(" (") + formatSize(m_fileSize) + QLatin1Char(')');
    L << QStringLiteral("offset = ") + QString::number(m_offset);
    L << QStringLiteral("length = ") + QString::number(m_length);
    L << QStringLiteral("load_into_memory = ") + formatBool(m_loadIntoMemory);
    L << QStringLiteral("source_path = ")
         + (m_source.sourcePath().isEmpty()
                ? QStringLiteral("(none)")
                : m_source.sourcePath());
    if (!m_xorLabel.isEmpty())
        L << QStringLiteral("xor_label = ") + m_xorLabel;

    // ────── Pixel format ──────
    L << QString();
    L << QStringLiteral("PIXEL_FORMAT:");
    L << QStringLiteral("bpp_mode = ") + QString::number(m_bppMode);
    L << QStringLiteral("is_bitpacked = ") + formatBool(isBitpacked());
    L << QStringLiteral("bytes_per_pixel = ") + QString::number(bytesPerPixel());
    L << QStringLiteral("bit_order = ") + m_bitOrder;
    L << QStringLiteral("byte_order = ") + m_byteOrder;
    L << QStringLiteral("pixel_order = ") + m_pixelOrder;
    L << QStringLiteral("row_padding_bits = ") + QString::number(m_rowPaddingBits);

    // ────── Geometry ──────
    L << QString();
    L << QStringLiteral("GEOMETRY:");
    L << QStringLiteral("width_mode = ") + widthModeStr;
    L << QStringLiteral("fixed_width = ") + QString::number(m_fixedWidth);
    L << QStringLiteral("width_step = ") + QString::number(m_widthStep);
    L << QStringLiteral("width_from_config = ") + formatBool(m_widthFromConfig);
    L << QStringLiteral("bytes_per_row = ") + QString::number(m_bytesPerRow);
    L << QStringLiteral("image_size = ") + QString::number(m_imageWidth)
         + QStringLiteral(" × ") + QString::number(m_imageHeight);
    L << QStringLiteral("viewport_rect = ")
         + QStringLiteral("%1,%2  %3×%4")
               .arg(m_viewportRect.x()).arg(m_viewportRect.y())
               .arg(m_viewportRect.width()).arg(m_viewportRect.height());

    // ────── View / zoom ──────
    L << QString();
    L << QStringLiteral("VIEW/ZOOM:");
    L << QStringLiteral("scale = ") + QString::number(m_scale);
    L << QStringLiteral("downsample = ") + QString::number(m_downsample);
    L << QStringLiteral("fit_to_window = ") + formatBool(m_fitToWindow);
    L << QStringLiteral("auto_scale = ") + formatBool(m_autoScale);
    L << QStringLiteral("mirror_h = ") + formatBool(m_mirrorH);
    L << QStringLiteral("mirror_v = ") + formatBool(m_mirrorV);
    L << QStringLiteral("hover_addr = ")
         + (m_hoverAddr < 0 ? QStringLiteral("-1 (none)")
                            : QString::number(m_hoverAddr));

    // ────── Tiles / cache ──────
    L << QString();
    L << QStringLiteral("TILES/CACHE:");
    L << QStringLiteral("tile_size = ") + QString::number(m_tileSize);
    L << QStringLiteral("tile_prefetch_margin = ")
         + QString::number(m_tilePrefetchMargin);
    L << QStringLiteral("cache_limit_mb = ") + QString::number(cacheLimit);
    L << QStringLiteral("cache_bytes = ") + QString::number(cacheB)
         + QStringLiteral(" (") + formatSize(cacheB) + QLatin1Char(')');
    L << QStringLiteral("active_tasks = ") + QString::number(activeT);
    L << QStringLiteral("perf_slider_value = ")
         + QString::number(m_perfSliderValue);

    // ────── Palette / histogram ──────
    L << QString();
    L << QStringLiteral("PALETTE:");
    L << QStringLiteral("palette = ")
         + (m_currentPaletteId.isEmpty()
                ? QStringLiteral("(none)")
                : m_currentPaletteId);
    L << QStringLiteral("palette_colors = ")
         + QString::number(m_currentPalette.size());
    L << QStringLiteral("palette_params_count = ")
         + QString::number(m_paletteParams.size());
    L << QStringLiteral("histogram_valid = ") + formatBool(m_byteHistogramValid);
    L << QStringLiteral("auto_apply_palette = ")
         + formatBool(m_autoApplyPalette);

    // ────── Hex view ──────
    L << QString();
    L << QStringLiteral("HEX_VIEW:");
    const bool hexActChecked = m_hexViewAct && m_hexViewAct->isChecked();
    L << QStringLiteral("menu_checked = ") + formatBool(hexActChecked);
    L << QStringLiteral("widget_visible = ")
         + formatBool(m_hexView && m_hexView->isVisible());
    L << QStringLiteral("widget_height = ")
         + (m_hexView ? QString::number(m_hexView->height())
                       : QStringLiteral("(none)"));
    L << QStringLiteral("saved_height = ")
         + QString::number(m_hexViewHeight);
    L << QStringLiteral("bytes_per_line = ")
         + QString::number(m_hexView ? m_hexView->bytesPerLine()
                                     : m_hexBytesPerLine);
    L << QStringLiteral("font_pt = ")
         + QString::number(m_hexView ? m_hexView->fontPt()
                                     : m_hexFontPt);

    // ────── Other ──────
    L << QString();
    L << QStringLiteral("OTHER:");
    L << QStringLiteral("language = ") + m_language;
    L << QStringLiteral("theme = ")
         + (m_theme && m_theme->isDark() ? QStringLiteral("dark")
                                         : QStringLiteral("light"));
    L << QStringLiteral("busy_png_export = ") + formatBool(m_busyPngExport);
    L << QStringLiteral("byte_tooltip = ")
         + formatBool(m_byteInfo && m_byteInfo->isPopupEnabled());
    L << QStringLiteral("show_debug_info = ") + formatBool(m_showDebugInfo);
    L << QStringLiteral("recent_files = ") + QString::number(m_recentFiles.size());

    m_debugLabel->setText(L.join(QStringLiteral("<br>")));
}