// Byte-0-scope - binary data visualizer
// File: definitions.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QVariantMap>

#ifndef APP_VERSION
#  define APP_VERSION "er-dev"
#endif

//  Configuration keys (shared by INI settings and per-file .b0s JSON)
#define CFG_VERSION             "version"
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

//  Configuration defaults
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
#define CFG_DEF_VERSION         1

//  Width mode enum
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

// Default display configuration for a freshly opened file with no .b0s.
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

namespace AppLimits {
constexpr const char *LangRu    = "ru";
constexpr const char *LangEn    = "en";
constexpr int         RecentMax = 10;
}

//  Palette identifiers
namespace PaletteIds {

// 8 bpp
constexpr const char *Gray          = "gray";
constexpr const char *Inverted      = "inverted";
constexpr const char *Heat          = "heat";
constexpr const char *Thermal       = "thermal";
constexpr const char *Viridis       = "viridis";
constexpr const char *Plasma        = "plasma";
constexpr const char *Rainbow       = "rainbow";
constexpr const char *Hsv           = "hsv";
constexpr const char *Solarize      = "solarize";
constexpr const char *HashColor     = "hash_color";
constexpr const char *BitScale      = "bit_scale";
constexpr const char *BitRotation   = "bit_rotation";
constexpr const char *ZerosOnes     = "zeros_ones";
constexpr const char *Nonzero       = "nonzero";
constexpr const char *HighBit       = "highbit";
constexpr const char *LowBit        = "lowbit";
constexpr const char *Nibbles       = "nibbles";
constexpr const char *HighNibble    = "high_nibble";
constexpr const char *LowNibble     = "low_nibble";
constexpr const char *Threshold     = "threshold";
constexpr const char *Ascii         = "ascii";
constexpr const char *Entropy       = "entropy";
constexpr const char *Bitplane      = "bitplane";
constexpr const char *Custom        = "custom";
constexpr const char *ByteFrequency = "byte_frequency";
constexpr const char *Repeats       = "repeats";
constexpr const char *FromFile      = "from_file";

// 1 bpp
constexpr const char *Mono1         = "mono_1";
constexpr const char *MonoInverted  = "mono_inverted";
constexpr const char *MonoAmber     = "mono_amber";
constexpr const char *MonoGreen     = "mono_green";
constexpr const char *MonoRed       = "mono_red";
constexpr const char *MonoBlue      = "mono_blue";

// 2 bpp
constexpr const char *Gray2          = "gray_2";
constexpr const char *GameboyClassic = "gameboy_classic";
constexpr const char *GameboyPocket  = "gameboy_pocket";
constexpr const char *Cga0           = "cga_0";
constexpr const char *Cga1           = "cga_1";
constexpr const char *CgaHigh        = "cga_high";
constexpr const char *GreenAmber     = "green_amber";
constexpr const char *Neon2          = "neon_2";

// 3 bpp
constexpr const char *Gray3    = "gray_3";
constexpr const char *Rgb332   = "rgb_332";
constexpr const char *Rainbow8 = "rainbow_8";
constexpr const char *Warm3    = "warm_3";
constexpr const char *Cool3    = "cool_3";

// 4 bpp
constexpr const char *Gray4         = "gray_4";
constexpr const char *Ega16         = "ega_16";
constexpr const char *EgaAlt        = "ega_alt";
constexpr const char *Windows16     = "windows_16";
constexpr const char *Commodore64   = "commodore_64";
constexpr const char *ZxSpectrum    = "zx_spectrum";
constexpr const char *AmberCrt      = "amber_crt";
constexpr const char *GreenTerminal = "green_terminal";
constexpr const char *Heat16        = "heat_16";
constexpr const char *Rainbow16     = "rainbow_16";

// 16 bpp
constexpr const char *Rgb565          = "rgb565";
constexpr const char *Bgr565          = "bgr565";
constexpr const char *Rgb555          = "rgb555";
constexpr const char *Argb1555        = "argb1555";
constexpr const char *Integer16       = "integer_16";
constexpr const char *Integer16Signed = "integer_16_signed";

// 24 bpp
constexpr const char *Rgb = "rgb";
constexpr const char *Bgr = "bgr";
constexpr const char *Gbr = "gbr";
constexpr const char *Brg = "brg";
constexpr const char *Grb = "grb";
constexpr const char *Rbg = "rbg";

// 32 bpp
constexpr const char *Rgba            = "rgba";
constexpr const char *Argb            = "argb";
constexpr const char *Bgra            = "bgra";
constexpr const char *Abgr            = "abgr";
constexpr const char *Integer32       = "integer_32";
constexpr const char *Integer32Signed = "integer_32_signed";
constexpr const char *Float32         = "float_32";

// 64 bpp
constexpr const char *Integer64       = "integer_64";
constexpr const char *Integer64Signed = "integer_64_signed";
constexpr const char *Float64         = "float_64";

} // namespace PaletteIds