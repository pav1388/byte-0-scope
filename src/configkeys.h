// Byte-0-scope - binary data visualizer
// File: configkeys.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QVariantMap>

// Единое место для ключей конфигурации. Одни и те же ключи используются в
// трёх местах: INI-настройках приложения, per-file конфиге (.b0s, JSON) и
// QVariantMap между MainWindow и ControlsPanel. Раньше они были продублированы
// строковыми литералами в каждом из них.

// --- Ключи ---
#define CFG_BPP_MODE            "bpp_mode"
#define CFG_PALETTE             "palette"
#define CFG_BIT_ORDER           "bit_order"
#define CFG_BYTE_ORDER          "byte_order"
#define CFG_PIXEL_ORDER         "pixel_order"
#define CFG_ROW_PADDING_BITS    "row_padding_bits"
#define CFG_WIDTH_MODE          "width_mode"
#define CFG_FIXED_WIDTH         "fixed_width"
#define CFG_WIDTH_STEP          "width_step"
#define CFG_SCALE               "scale"
#define CFG_AUTO_SCALE          "auto_scale"
#define CFG_FIT_TO_WINDOW       "fit_to_window"
#define CFG_OFFSET              "offset"
#define CFG_LENGTH              "length"
#define CFG_LENGTH_MODE         "length_mode"
#define CFG_BYTES_PER_ROW       "bytes_per_row"
#define CFG_MIRROR_H            "mirror_horizontal"
#define CFG_MIRROR_V            "mirror_vertical"
#define CFG_PALETTE_PARAMS      "palette_params"
#define CFG_BYTE_TOOLTIP        "byte_tooltip"

// --- Значения по умолчанию ---
#define CFG_DEF_PALETTE         "heat"
#define CFG_DEF_BPP_MODE        8
#define CFG_DEF_BIT_ORDER       "msb"
#define CFG_DEF_PIXEL_ORDER     "msb"
#define CFG_DEF_BYTE_ORDER      "big"
#define CFG_DEF_ROW_PADDING     0
#define CFG_DEF_WIDTH_MODE      "fixed"
#define CFG_DEF_FIXED_WIDTH     256
#define CFG_DEF_WIDTH_STEP      8
#define CFG_DEF_SCALE           1
#define CFG_DEF_BYTES_PER_ROW   16

// --- Режимы ширины ---
// Индексы совпадают с порядком элементов в ControlsPanel::m_widthModeCombo.
enum WidthMode {
    WM_Fixed = 0,
    WM_Auto,
    WM_Power,
    WM_Aspect16x9,
    WM_Aspect4x3,
    WM_BytesPerRow,
    WM_Nice,
    WM_Divisor,
    WM_Count
};

inline QString widthModeKey(int mode)
{
    switch (mode) {
    case WM_Auto:        return QStringLiteral("auto");
    case WM_Power:       return QStringLiteral("power");
    case WM_Aspect16x9:  return QStringLiteral("aspect16_9");
    case WM_Aspect4x3:   return QStringLiteral("aspect4_3");
    case WM_BytesPerRow: return QStringLiteral("bytes");
    case WM_Nice:        return QStringLiteral("nice");
    case WM_Divisor:     return QStringLiteral("divisor");
    case WM_Fixed:
    default:             return QStringLiteral("fixed");
    }
}

inline int widthModeFromKey(const QString &key)
{
    for (int mode = 0; mode < WM_Count; ++mode) {
        if (key == widthModeKey(mode))
            return mode;
    }
    return WM_Fixed;
}

// Конфигурация отображения по умолчанию (то, что применяется при открытии
// файла без .b0s и при сбросе настроек).
inline QVariantMap defaultDisplayConfig()
{
    QVariantMap cfg;
    cfg[QStringLiteral(CFG_PALETTE)]          = QStringLiteral(CFG_DEF_PALETTE);
    cfg[QStringLiteral(CFG_SCALE)]            = CFG_DEF_SCALE;
    cfg[QStringLiteral(CFG_AUTO_SCALE)]       = false;
    cfg[QStringLiteral(CFG_FIT_TO_WINDOW)]    = false;
    cfg[QStringLiteral(CFG_WIDTH_MODE)]       = QStringLiteral(CFG_DEF_WIDTH_MODE);
    cfg[QStringLiteral(CFG_BYTES_PER_ROW)]    = CFG_DEF_BYTES_PER_ROW;
    cfg[QStringLiteral(CFG_FIXED_WIDTH)]      = CFG_DEF_FIXED_WIDTH;
    cfg[QStringLiteral(CFG_WIDTH_STEP)]       = CFG_DEF_WIDTH_STEP;
    cfg[QStringLiteral(CFG_OFFSET)]           = 0;
    cfg[QStringLiteral(CFG_LENGTH)]           = 0;
    cfg[QStringLiteral(CFG_LENGTH_MODE)]      = false;
    cfg[QStringLiteral(CFG_BPP_MODE)]         = CFG_DEF_BPP_MODE;
    cfg[QStringLiteral(CFG_BIT_ORDER)]        = QStringLiteral(CFG_DEF_BIT_ORDER);
    cfg[QStringLiteral(CFG_PIXEL_ORDER)]      = QStringLiteral(CFG_DEF_PIXEL_ORDER);
    cfg[QStringLiteral(CFG_BYTE_ORDER)]       = QStringLiteral(CFG_DEF_BYTE_ORDER);
    cfg[QStringLiteral(CFG_ROW_PADDING_BITS)] = CFG_DEF_ROW_PADDING;
    return cfg;
}
