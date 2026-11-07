// Byte-0-scope - binary data visualizer
// File: pal_palettemanager.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pal_palettemanager.h"
#include "definitions.h"
#include "core_pixeldecoder.h"

#include <QCoreApplication>
#include <QSet>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <limits>

// --- PaletteManager module settings (prefix PM_) ---

#define PM_DEFAULT_PALETTE_SIZE     256
#define PM_FROM_FILE_COUNT_MAX      (1 << 20)

#define PM_THRESHOLD_DEFAULT        64
#define PM_ENTROPY_STEP_DEFAULT     16
#define PM_BITPLANE_DEFAULT         0
#define PM_MIN_COUNT_DEFAULT        0
#define PM_MIN_COUNT_FALLBACK       50
#define PM_MIN_COUNT_MAX_VALUE      100000000
#define PM_FROM_FILE_LENGTH_DEFAULT 256

#define PM_SAMPLED_BPPS             { 2, 3, 4 }

namespace {

inline QString pid(const char *s) { return QString::fromLatin1(s); }

//  bpp -> bytes-per-pixel
inline int bytesPerPixelForBpp(int bpp)
{
    if (bpp <= 8)
        return 1;
    return bpp / 8;
}

//  Gradient / palette helpers (unchanged)
QVector<QColor> grayPalette(int n)
{
    QVector<QColor> out;
    if (n <= 1) {
        out.append(QColor(0, 0, 0));
        return out;
    }
    out.reserve(n);
    for (int i = 0; i < n; ++i) {
        const int v = int(i * 255 / (n - 1));
        out.append(QColor(v, v, v));
    }
    return out;
}

typedef QPair<double, QColor> Stop;

QVector<QColor> gradientPalette(int n, QVector<Stop> stops)
{
    std::sort(stops.begin(), stops.end(),
              [](const Stop &a, const Stop &b) { return a.first < b.first; });
    QVector<QColor> out;
    out.reserve(n);
    for (int i = 0; i < n; ++i) {
        const double t = double(i) / qMax(1, n - 1);
        if (t <= stops.first().first) {
            out.append(stops.first().second);
            continue;
        }
        if (t >= stops.last().first) {
            out.append(stops.last().second);
            continue;
        }
        for (int j = 0; j + 1 < stops.size(); ++j) {
            const double p0 = stops[j].first;
            const double p1 = stops[j + 1].first;
            if (p0 <= t && t <= p1) {
                const double u = (t - p0) / qMax(1e-9, p1 - p0);
                const QColor &c0 = stops[j].second;
                const QColor &c1 = stops[j + 1].second;
                out.append(QColor(
                    int(c0.red()   + (c1.red()   - c0.red())   * u),
                    int(c0.green() + (c1.green() - c0.green()) * u),
                    int(c0.blue()  + (c1.blue()  - c0.blue())  * u)));
                break;
            }
        }
    }
    return out;
}

// --- Basic 8 bpp ---------------------------------------------------------
QVector<QColor> createGrayscale(const QVariantMap &)
{
    return grayPalette(PM_DEFAULT_PALETTE_SIZE);
}

QColor asciiParamColor(const QVariantMap &params,
                       const char *key,
                       const QColor &fallback)
{
    const QColor color(params.value(QLatin1String(key), fallback.name()).toString());
    return color.isValid() ? color : fallback;
}

QVector<QColor> createAscii(const QVariantMap &params)
{
    const QColor printableColor = asciiParamColor(
        params, "printable_color", QColor(90, 210, 150));
    const QColor nonPrintableColor = asciiParamColor(
        params, "nonprintable_color", QColor(230, 85, 85));

    const bool extended = params.value("extended", false).toBool();
    const QColor controlColor = asciiParamColor(
        params, "control_color", QColor(230, 85, 85));
    const QColor whitespaceColor = asciiParamColor(
        params, "whitespace_color", QColor(90, 190, 220));
    const QColor digitColor = asciiParamColor(
        params, "digit_color", QColor(255, 205, 86));
    const QColor upperColor = asciiParamColor(
        params, "uppercase_color", QColor(90, 160, 255));
    const QColor lowerColor = asciiParamColor(
        params, "lowercase_color", QColor(90, 210, 150));
    const QColor symbolColor = asciiParamColor(
        params, "symbol_color", QColor(225, 145, 90));
    const QColor extendedColor = asciiParamColor(
        params, "extended_color", QColor(170, 120, 230));

    QVector<QColor> out;
    out.reserve(PM_DEFAULT_PALETTE_SIZE);

    for (int value = 0; value < PM_DEFAULT_PALETTE_SIZE; ++value) {
        if (!extended) {
            out.append(value >= 0x20 && value <= 0x7E
                       ? printableColor : nonPrintableColor);
            continue;
        }

        if (value == 0x09 || value == 0x0A || value == 0x0B ||
            value == 0x0C || value == 0x0D || value == 0x20) {
            out.append(whitespaceColor);
        } else if (value < 0x20 || value == 0x7F) {
            out.append(controlColor);
        } else if (value >= '0' && value <= '9') {
            out.append(digitColor);
        } else if (value >= 'A' && value <= 'Z') {
            out.append(upperColor);
        } else if (value >= 'a' && value <= 'z') {
            out.append(lowerColor);
        } else if (value >= 0x21 && value <= 0x7E) {
            out.append(symbolColor);
        } else {
            out.append(extendedColor);
        }
    }

    return out;
}

QVector<QColor> createInverted(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
        o.append(QColor(255 - i, 255 - i, 255 - i));
    return o;
}

QVector<QColor> createZerosOnes(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        if (i == 0)
            o.append(QColor(0, 0, 255));
        else if (i == PM_DEFAULT_PALETTE_SIZE - 1)
            o.append(QColor(255, 255, 0));
        else
            o.append(QColor(0, 0, 0));
    }
    return o;
}

QVector<QColor> createNonzero(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
        o.append(i != 0 ? QColor(0, 255, 0) : QColor(0, 0, 0));
    return o;
}

QVector<QColor> createHighbit(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
        o.append((i & 0x80) ? QColor(255, 0, 0) : QColor(0, 0, 0));
    return o;
}

QVector<QColor> createLowbit(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
        o.append((i & 1) ? QColor(0, 255, 255) : QColor(0, 0, 0));
    return o;
}

QVector<QColor> createHeat(const QVariantMap &)
{
    return gradientPalette(PM_DEFAULT_PALETTE_SIZE, {
        {0.0 / 6.0, QColor(0, 0, 0)},     {1.0 / 6.0, QColor(0, 0, 255)},
        {2.0 / 6.0, QColor(0, 255, 255)}, {3.0 / 6.0, QColor(0, 255, 0)},
        {4.0 / 6.0, QColor(255, 255, 0)}, {5.0 / 6.0, QColor(255, 0, 0)},
        {6.0 / 6.0, QColor(255, 255, 255)},
    });
}

QVector<QColor> createThermal(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
        o.append(QColor(qMin(255, 3 * i), qMin(255, 2 * i), i / 2));
    return o;
}

QVector<QColor> createPlasma(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    const double twoPi = 2.0 * M_PI;
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        const double t = twoPi * i / double(PM_DEFAULT_PALETTE_SIZE - 1);
        o.append(QColor(
            int(255.0 * (0.5 + 0.5 * std::sin(t))),
            int(255.0 * (0.5 + 0.5 * std::sin(t + twoPi / 3.0))),
            int(255.0 * (0.5 + 0.5 * std::sin(t + 2.0 * twoPi / 3.0)))));
    }
    return o;
}

QVector<QColor> createViridis(const QVariantMap &)
{
    return gradientPalette(PM_DEFAULT_PALETTE_SIZE, {
        {0.00, QColor(68, 1, 84)},    {0.25, QColor(59, 82, 139)},
        {0.50, QColor(33, 145, 140)}, {0.75, QColor(94, 201, 98)},
        {1.00, QColor(253, 231, 37)},
    });
}

QVector<QColor> createRainbow(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    const int half = PM_DEFAULT_PALETTE_SIZE / 2;
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        if (i < half)
            o.append(QColor(255, i, 0));
        else
            o.append(QColor(255 - (i - half) * 2, 255, (i - half) * 2));
    }
    return o;
}

QVector<QColor> createHsv(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        const int h = int(i * 360 / (PM_DEFAULT_PALETTE_SIZE - 1)) % 360;
        o.append(QColor::fromHsv(h, 255, 255));
    }
    return o;
}

QVector<QColor> createNibbles(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
        o.append(QColor(i & 0xF0, (i << 4) & 0xF0, 0));
    return o;
}

QVector<QColor> createHighNibble(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        const int v = i & 0xF0;
        o.append(QColor(v, v, v));
    }
    return o;
}

QVector<QColor> createLowNibble(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        const int v = (i & 0x0F) << 4;
        o.append(QColor(v, v, v));
    }
    return o;
}

QVector<QColor> createSolarized(const QVariantMap &params)
{
    const int threshold = params.value("threshold", PM_THRESHOLD_DEFAULT).toInt();
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
        o.append(i < threshold ? QColor(7, 54, 101) : QColor(88, 110, 122));
    return o;
}

QVector<QColor> createEntropy(const QVariantMap &params)
{
    const int step = qMax(1, params.value("step", PM_ENTROPY_STEP_DEFAULT).toInt());
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        const int band = qMin(255, (i / step) * step + step / 2);
        const int r = (255 * band) / 255;
        o.append(QColor(r, 0, 255 - r));
    }
    return o;
}

QVector<QColor> createBitplane(const QVariantMap &params)
{
    const int plane = qBound(0, params.value("plane", PM_BITPLANE_DEFAULT).toInt(), 7);
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int b = 0; b < PM_DEFAULT_PALETTE_SIZE; ++b)
        o.append(((b >> plane) & 1) ? QColor(255, 255, 255) : QColor(0, 0, 0));
    return o;
}

QVector<QColor> createBitScale(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        int r = 0, g = 0, b = 0;
        if (i & 0x80) r += 255;
        if (i & 0x40) r += 128;
        if (i & 0x20) g += 255;
        if (i & 0x10) g += 128;
        if (i & 0x08) b += 255;
        if (i & 0x04) b += 128;
        int m = 0;
        if (i & 0x02) m += 128;
        if (i & 0x01) m += 64;
        r = qMin(255, r + m / 3);
        g = qMin(255, g + m / 3);
        b = qMin(255, b + m / 3);
        o.append(QColor(r, g, b));
    }
    return o;
}

QVector<QColor> createBitRotation(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        const int r = ((i << 1) | (i >> 7)) & 0xFF;
        const int g = ((i << 3) | (i >> 5)) & 0xFF;
        const int b = ((i << 5) | (i >> 3)) & 0xFF;
        o.append(QColor(r, g, b));
    }
    return o;
}

QVector<QColor> createSolarize(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        const double t = double(i) / (PM_DEFAULT_PALETTE_SIZE - 1);
        if (t < 0.5) {
            o.append(QColor(int(t * 2 * 255), int(t * 2 * 160), int(t * 2 * 40)));
        } else {
            const double u = (t - 0.5) * 2.0;
            o.append(QColor(int(255 * (1 - u * 0.6)),
                            int(160 * (1 - u * 0.9)),
                            int(40  * (1 - u))));
        }
    }
    return o;
}

QVector<QColor> createHashColor(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        const quint32 h = quint32(i) * 2654435761u;
        o.append(QColor((h >> 16) & 0xFF, (h >> 8) & 0xFF, h & 0xFF));
    }
    return o;
}

// --- 1 bpp ---------------------------------------------------------------
QVector<QColor> createMono1bpp(const QVariantMap &)     { return { QColor(0, 0, 0),     QColor(255, 255, 255) }; }
QVector<QColor> createMonoInverted(const QVariantMap &) { return { QColor(255, 255, 255), QColor(0, 0, 0)     }; }
QVector<QColor> createMonoAmber(const QVariantMap &)    { return { QColor(0, 0, 0),     QColor(255, 176, 0)   }; }
QVector<QColor> createMonoGreen(const QVariantMap &)    { return { QColor(0, 0, 0),     QColor(0, 255, 0)     }; }
QVector<QColor> createMonoRed(const QVariantMap &)      { return { QColor(0, 0, 0),     QColor(255, 0, 0)     }; }
QVector<QColor> createMonoBlue(const QVariantMap &)     { return { QColor(0, 0, 0),     QColor(80, 160, 255)  }; }

// --- 2 bpp ---------------------------------------------------------------
QVector<QColor> createGray2bpp(const QVariantMap &) { return grayPalette(4); }
QVector<QColor> createGameboyClassic(const QVariantMap &)
{
    return { QColor(15, 56, 15), QColor(48, 98, 48), QColor(139, 172, 15), QColor(155, 188, 15) };
}
QVector<QColor> createGameboyPocket(const QVariantMap &)
{
    return { QColor(4, 12, 8), QColor(60, 96, 72), QColor(140, 168, 120), QColor(220, 236, 200) };
}
QVector<QColor> createCga0(const QVariantMap &)
{
    return { QColor(0, 0, 0), QColor(0, 170, 0), QColor(170, 0, 0), QColor(170, 85, 0) };
}
QVector<QColor> createCga1(const QVariantMap &)
{
    return { QColor(0, 0, 0), QColor(0, 170, 170), QColor(170, 0, 170), QColor(170, 170, 170) };
}
QVector<QColor> createCgaHigh(const QVariantMap &)
{
    return { QColor(0, 0, 0), QColor(85, 255, 255), QColor(255, 85, 255), QColor(255, 255, 255) };
}
QVector<QColor> createGreenAmber2bpp(const QVariantMap &)
{
    return { QColor(0, 0, 0), QColor(0, 128, 0), QColor(170, 110, 0), QColor(255, 200, 0) };
}
QVector<QColor> createNeon2bpp(const QVariantMap &)
{
    return { QColor(0, 0, 0), QColor(0, 255, 128), QColor(255, 0, 200), QColor(255, 255, 0) };
}

// --- 3 bpp ---------------------------------------------------------------
QVector<QColor> createGray3bpp(const QVariantMap &) { return grayPalette(8); }
QVector<QColor> createRgb332(const QVariantMap &)
{
    return {
        QColor(0, 0, 0),       QColor(255, 0, 0),     QColor(0, 255, 0),     QColor(0, 0, 255),
        QColor(255, 255, 0),   QColor(255, 0, 255),   QColor(0, 255, 255),   QColor(255, 255, 255),
    };
}
QVector<QColor> createRainbow8(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(8);
    const double twoPi = 2.0 * M_PI;
    for (int i = 0; i < 8; ++i) {
        const double t = twoPi * i / 8.0;
        o.append(QColor(
            int(255.0 * (0.5 + 0.5 * std::cos(t))),
            int(255.0 * (0.5 + 0.5 * std::cos(t - twoPi / 3.0))),
            int(255.0 * (0.5 + 0.5 * std::cos(t - 2.0 * twoPi / 3.0)))));
    }
    return o;
}
QVector<QColor> createWarm3bpp(const QVariantMap &)
{
    return {
        QColor(0, 0, 0),   QColor(64, 0, 0),    QColor(128, 0, 0),    QColor(192, 0, 0),
        QColor(255, 64, 0),QColor(255, 128, 0), QColor(255, 192, 0),  QColor(255, 255, 128),
    };
}
QVector<QColor> createCool3bpp(const QVariantMap &)
{
    return {
        QColor(0, 0, 0),    QColor(0, 0, 64),    QColor(0, 0, 128),    QColor(0, 0, 192),
        QColor(0, 64, 255), QColor(0, 128, 255), QColor(0, 192, 255),  QColor(128, 255, 255),
    };
}

// --- 4 bpp ---------------------------------------------------------------
QVector<QColor> createGray4bpp(const QVariantMap &) { return grayPalette(16); }
QVector<QColor> createEga16(const QVariantMap &)
{
    return {
        QColor(0, 0, 0),        QColor(0, 0, 170),      QColor(0, 170, 0),       QColor(0, 170, 170),
        QColor(170, 0, 0),      QColor(170, 0, 170),    QColor(170, 85, 0),      QColor(170, 170, 170),
        QColor(85, 85, 85),     QColor(85, 85, 255),    QColor(85, 255, 85),     QColor(85, 255, 255),
        QColor(255, 85, 85),    QColor(255, 85, 255),   QColor(255, 255, 85),    QColor(255, 255, 255),
    };
}
QVector<QColor> createEgaAlt(const QVariantMap &)
{
    return {
        QColor(0, 0, 0),        QColor(170, 0, 0),      QColor(0, 170, 0),       QColor(170, 85, 0),
        QColor(0, 0, 170),      QColor(170, 0, 170),    QColor(0, 170, 170),     QColor(170, 170, 170),
        QColor(85, 85, 85),     QColor(255, 85, 85),    QColor(85, 255, 85),     QColor(255, 255, 85),
        QColor(85, 85, 255),    QColor(255, 85, 255),   QColor(85, 255, 255),    QColor(255, 255, 255),
    };
}
QVector<QColor> createWindows16(const QVariantMap &)
{
    return {
        QColor(0, 0, 0),        QColor(128, 0, 0),      QColor(0, 128, 0),       QColor(128, 128, 0),
        QColor(0, 0, 128),      QColor(128, 0, 128),    QColor(0, 128, 128),     QColor(192, 192, 192),
        QColor(128, 128, 128),  QColor(255, 0, 0),      QColor(0, 255, 0),       QColor(255, 255, 0),
        QColor(0, 0, 255),      QColor(255, 0, 255),    QColor(0, 255, 255),     QColor(255, 255, 255),
    };
}
QVector<QColor> createC64(const QVariantMap &)
{
    return {
        QColor(0, 0, 0),        QColor(255, 255, 255),  QColor(136, 0, 0),       QColor(170, 255, 238),
        QColor(204, 68, 204),   QColor(0, 204, 85),     QColor(0, 0, 170),       QColor(238, 238, 119),
        QColor(221, 136, 85),   QColor(102, 68, 0),     QColor(255, 119, 119),   QColor(51, 51, 51),
        QColor(119, 119, 119),  QColor(170, 255, 102),  QColor(0, 136, 255),     QColor(187, 187, 187),
    };
}
QVector<QColor> createZx(const QVariantMap &)
{
    return {
        QColor(0, 0, 0),        QColor(0, 0, 215),      QColor(215, 0, 0),       QColor(215, 0, 215),
        QColor(0, 215, 0),      QColor(0, 215, 215),    QColor(215, 215, 0),     QColor(215, 215, 215),
        QColor(0, 0, 0),        QColor(0, 0, 255),      QColor(255, 0, 0),       QColor(255, 0, 255),
        QColor(0, 255, 0),      QColor(0, 255, 255),    QColor(255, 255, 0),     QColor(255, 255, 255),
    };
}
QVector<QColor> createAmberCrt(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(16);
    for (int i = 0; i < 16; ++i)
        o.append(QColor(int(i * 255 / 15), int(i * 176 / 15), 0));
    return o;
}
QVector<QColor> createGreenTerminal(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(16);
    for (int i = 0; i < 16; ++i)
        o.append(QColor(0, int(i * 255 / 15), 0));
    return o;
}
QVector<QColor> createHeat16(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(16);
    for (int i = 0; i < 16; ++i) {
        const double t = i / 15.0;
        const int r = qMin(255, int(t * 3 * 255));
        const int g = qMin(255, int(qMax(0.0, t * 3 - 1) * 255));
        const int b = qMin(255, int(qMax(0.0, t * 3 - 2) * 255));
        o.append(QColor(r, g, b));
    }
    return o;
}
QVector<QColor> createRainbow16(const QVariantMap &)
{
    QVector<QColor> o;
    o.reserve(16);
    const double twoPi = 2.0 * M_PI;
    for (int i = 0; i < 16; ++i) {
        const double t = twoPi * i / 16.0;
        o.append(QColor(
            int(255.0 * (0.5 + 0.5 * std::cos(t))),
            int(255.0 * (0.5 + 0.5 * std::cos(t - twoPi / 3.0))),
            int(255.0 * (0.5 + 0.5 * std::cos(t - 2.0 * twoPi / 3.0)))));
    }
    return o;
}

// --- "Custom" ------------------------------------------------------------
QVector<QColor> createCustom(const QVariantMap &params)
{
    const QString defStr = params.value("default_color", "#000000").toString();
    QColor def(defStr);
    if (!def.isValid())
        def = QColor(0, 0, 0);

    QVector<QColor> out(PM_DEFAULT_PALETTE_SIZE, def);

    const QVariantList rules = params.value("rules").toList();
    for (const QVariant &rv : rules) {
        const QVariantMap r = rv.toMap();
        const int start = qBound(0, r.value("start", 0).toInt(), PM_DEFAULT_PALETTE_SIZE - 1);
        const int end   = qBound(0, r.value("end", start).toInt(), PM_DEFAULT_PALETTE_SIZE - 1);
        QColor c(r.value("color", "#000000").toString());
        if (!c.isValid())
            continue;
        for (int i = start; i <= end; ++i)
            out[i] = c;
    }
    return out;
}

// --- "From file" ---------------------------------------------------------
QVector<QColor> createFromFile(const QVariantMap &params)
{
    const QByteArray data = params.value("data").toByteArray();
    const int recSize     = qMax(1, params.value("record_size", 1).toInt());
    const int bpp         = params.value("bpp", 8).toInt();
    const QString fmt     = params.value("format", "auto").toString();
    const QString end     = params.value("endianness", "big").toString();
    int count             = params.value("count", 0).toInt();

    if (count <= 0) {
        if (bpp <= 4)
            count = 1 << bpp;
        else if (bpp <= 8)
            count = PM_DEFAULT_PALETTE_SIZE;
        else
            count = data.isEmpty() ? PM_DEFAULT_PALETTE_SIZE
                                   : qMax(1, data.size() / recSize);
    }

    count = qMin(count, PM_FROM_FILE_COUNT_MAX);
    auto fallback = [count]() -> QVector<QColor> {
        return grayPalette(qMax(1, count));
    };

    if (data.isEmpty())
        return fallback();

    QVector<QColor> out;
    out.reserve(count);
    for (int i = 0; i < count; ++i) {
        const qint64 start = qint64(i) * recSize;
        if (start + recSize > data.size())
            break;
        const QByteArray rec = data.mid(int(start), recSize);
        out.append(PixelDecoder::interpretPaletteRecord(rec, fmt, recSize, end));
    }
    return out.isEmpty() ? fallback() : out;
}

// --- "Byte frequency" ----------------------------------------------------
QVector<QColor> createByteFrequency(const QVariantMap &params)
{
    const QVariantList freqVar = params.value("freq").toList();
    qint64 freq[PM_DEFAULT_PALETTE_SIZE];
    qint64 maxFreq = 1;

    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        freq[i] = (i < freqVar.size()) ? freqVar[i].toLongLong() : 1;
        if (freq[i] > maxFreq)
            maxFreq = freq[i];
    }

    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        const double t = double(freq[i]) / maxFreq;
        const int r = int(255 * t);
        const int g = int(255 * (1 - t));
        o.append(QColor(r, g, 0));
    }
    return o;
}

// --- "Repeats" -----------------------------------------------------------
QVector<QColor> createRepeatsHighlight(const QVariantMap &params)
{
    QSet<int> repeats;
    for (const QVariant &v : params.value("repeats").toList())
        repeats.insert(v.toInt());

    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
        o.append(repeats.contains(i) ? QColor(255, 0, 0) : QColor(i, i, i));
    return o;
}

// -------------------------------------------------------------------------
//  Multibyte placeholder
// -------------------------------------------------------------------------
QVector<QColor> createMultibytePlaceholder(const QVariantMap &)
{
    return grayPalette(PM_DEFAULT_PALETTE_SIZE);
}

// --- Preparation steps ---------------------------------------------------
QVariantMap prepareByteFrequency(const QVariantMap &params,
                                 const QByteArray &fileData,
                                 const quint64 *byteHistogram)
{
    QVariantList list;

    if (byteHistogram) {
        list.reserve(PM_DEFAULT_PALETTE_SIZE);
        for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
            list.append(qint64(byteHistogram[i]));
    } else if (!fileData.isEmpty()) {
        quint64 freq[PM_DEFAULT_PALETTE_SIZE] = {0};
        const uchar *p = reinterpret_cast<const uchar *>(fileData.constData());
        const int n = fileData.size();
        for (int i = 0; i < n; ++i)
            ++freq[p[i]];
        list.reserve(PM_DEFAULT_PALETTE_SIZE);
        for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
            list.append(qint64(freq[i]));
    } else {
        return params;
    }

    QVariantMap out;
    out["freq"] = list;
    return out;
}

QVariantMap prepareRepeats(const QVariantMap &params,
                           const QByteArray &fileData,
                           const quint64 *byteHistogram)
{
    quint64 localCounts[PM_DEFAULT_PALETTE_SIZE] = {0};
    const quint64 *hist = byteHistogram;

    if (!hist && !fileData.isEmpty()) {
        const uchar *p = reinterpret_cast<const uchar *>(fileData.constData());
        const int n = fileData.size();
        for (int i = 0; i < n; ++i)
            ++localCounts[p[i]];
        hist = localCounts;
    }

    if (!hist)
        return params;

    int minCount = params.value("min_count", PM_MIN_COUNT_DEFAULT).toInt();
    if (minCount <= 0) {
        const int avg = PaletteManager::averageRepeatCount(hist);
        minCount = (avg > 0) ? avg : PM_MIN_COUNT_FALLBACK;
    }

    QVariantList list;
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
        if (hist[i] >= quint64(minCount))
            list.append(i);

    QVariantMap out;
    out["repeats"] = list;
    out["min_count"] = minCount;
    return out;
}

QVariantMap prepareFromFile(const QVariantMap &params, const QByteArray &fileData)
{
    const int address    = qMax(0, params.value("address", 0).toInt());
    const int length     = qMax(1, params.value("length", PM_FROM_FILE_LENGTH_DEFAULT).toInt());
    const int recordSize = qMax(1, params.value("record_size", 1).toInt());
    const int bpp        = params.value("bpp", 8).toInt();
    const int explicitCt = params.value("count", 0).toInt();
    const QString fmt    = params.value("format", "auto").toString();
    const QString end    = params.value("endianness", "big").toString();

    if (fileData.isEmpty()) {
        QVariantMap out;
        out["data"] = QByteArray();
        out["count"] = 0;
        out["record_size"] = recordSize;
        out["bpp"] = bpp;
        out["format"] = fmt;
        out["endianness"] = end;
        return out;
    }

    int count;
    if (explicitCt > 0)
        count = explicitCt;
    else if (bpp <= 4)
        count = 1 << bpp;
    else
        count = qMax(1, length / recordSize);

    count = qMin(count, PM_FROM_FILE_COUNT_MAX);

    const qint64 sliceLength = qMin<qint64>(qint64(count) * recordSize,
                                            std::numeric_limits<int>::max());
    QByteArray slice = fileData.mid(address, int(sliceLength));

    QVariantMap out;
    out["data"] = slice;
    out["count"] = count;
    out["record_size"] = recordSize;
    out["bpp"] = bpp;
    out["format"] = fmt;
    out["endianness"] = end;
    return out;
}

// -------------------------------------------------------------------------
//  Default-parameter helpers for configurable palettes
// -------------------------------------------------------------------------
QVariantMap asciiDefaults()
{
    QVariantMap p;
    p["extended"] = false;
    p["printable_color"]     = "#5ad296";
    p["nonprintable_color"]  = "#e65555";
    p["control_color"]       = "#aa78e6";
    p["whitespace_color"]    = "#cecece";
    p["digit_color"]         = "#ffcd56";
    p["uppercase_color"]     = "#5ad296";
    p["lowercase_color"]     = "#5ad296";
    p["symbol_color"]        = "#e1915a";
    p["extended_color"]      = "#e65555";
    return p;
}

QVariantMap thresholdDefaults() { return {{"threshold", PM_THRESHOLD_DEFAULT}}; }
QVariantMap entropyDefaults()   { return {{"step",      PM_ENTROPY_STEP_DEFAULT}}; }
QVariantMap bitplaneDefaults()  { return {{"plane",     PM_BITPLANE_DEFAULT}}; }
QVariantMap repeatsDefaults()   { return {{"min_count", PM_MIN_COUNT_DEFAULT}}; }

QVariantMap fromFileDefaults()
{
    QVariantMap p;
    p["address"]     = 0;
    p["length"]      = PM_FROM_FILE_LENGTH_DEFAULT;
    p["record_size"] = 1;
    p["format"]      = "auto";
    p["endianness"]  = "big";
    p["count"]       = 0;
    return p;
}

QVariantMap customDefaults()
{
    QVariantMap p;
    p["default_color"] = "#000000";
    p["rules"] = PaletteManager::defaultCustomRules();
    return p;
}

} // namespace

// =========================================================================
//  Public statics
// =========================================================================

QString PaletteManager::defaultRuleColor(int index)
{
    static const char *const colors[] = {
        "#FF0000", "#00FF00", "#0000FF", "#FF0080",
        "#80FF00", "#0080FF", "#FF8000", "#8000FF"
    };
    const int count = int(sizeof(colors) / sizeof(colors[0]));
    return QString::fromLatin1(colors[((index % count) + count) % count]);
}

QVariantList PaletteManager::defaultCustomRules()
{
    QVariantList rules;
    for (int i = 0; i < 8; ++i) {
        QVariantMap r;
        r["start"] = i * 32;
        r["end"]   = i * 32 + 31;
        r["color"] = defaultRuleColor(i);
        rules.append(r);
    }
    return rules;
}

int PaletteManager::averageRepeatCount(const quint64 *histogram)
{
    if (!histogram)
        return 0;

    quint64 total = 0;
    int distinct = 0;
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i) {
        if (histogram[i] > 0) {
            total += histogram[i];
            ++distinct;
        }
    }
    if (distinct == 0)
        return 0;

    const quint64 avg = total / quint64(distinct);
    return int(qMin<quint64>(quint64(PM_MIN_COUNT_MAX_VALUE),
                             qMax<quint64>(1, avg)));
}

// =========================================================================
//  Constructor
// =========================================================================

PaletteManager::PaletteManager()
{
    registerBasePalettes();
    registerSampledPalettes();
}

// =========================================================================
//  Registration primitives
// =========================================================================

void PaletteManager::registerPalette(const PaletteDef &def)
{
    // extraGroups is used (rather than calling registerPalette twice) for
    // palettes that appear in more than one bpp list, so this function is
    // expected to be called exactly once per id.
    Q_ASSERT_X(!m_generators.contains(def.id),
               "PaletteManager::registerPalette",
               "palette id registered twice");

    m_generators[def.id] = def.generator;
    m_names[def.id]      = def.name;

    Config c;
    c.configurable  = def.configurable;
    c.defaultParams = def.defaultParams;
    c.bytesPerPixel = bytesPerPixelForBpp(def.bpp);
    m_configs[def.id] = c;

    if (!m_groups[def.bpp].contains(def.id))
        m_groups[def.bpp].append(def.id);

    for (int extra : def.extraGroups) {
        if (!m_groups[extra].contains(def.id))
            m_groups[extra].append(def.id);
    }
}

// =========================================================================
//  Base palette table
//
//  Order matters only inside a given bpp group; it defines the combo-box
//  order in ControlsPanel. Group order follows the previous implementation
//  exactly, and registerSampledPalettes() appends to groups 2/3/4 after this
//  function returns.
// =========================================================================

void PaletteManager::registerBasePalettes()
{
    // ---------------- 1 bpp --------------------------------------------
    registerPalette({ pid(PaletteIds::Mono1),        tr("Mono 1bpp"),     createMono1bpp,    1 });
    registerPalette({ pid(PaletteIds::MonoInverted), tr("Inverted mono"), createMonoInverted, 1 });
    registerPalette({ pid(PaletteIds::MonoAmber),    tr("Amber mono"),    createMonoAmber,    1 });
    registerPalette({ pid(PaletteIds::MonoGreen),    tr("Green mono"),    createMonoGreen,    1 });
    registerPalette({ pid(PaletteIds::MonoRed),      tr("Red mono"),      createMonoRed,      1 });
    registerPalette({ pid(PaletteIds::MonoBlue),     tr("Blue mono"),     createMonoBlue,     1 });

    // ---------------- 2 bpp --------------------------------------------
    registerPalette({ pid(PaletteIds::Gray2),          tr("Grayscale 2bpp"),    createGray2bpp,        2 });
    registerPalette({ pid(PaletteIds::GameboyClassic), tr("Game Boy (classic)"), createGameboyClassic, 2 });
    registerPalette({ pid(PaletteIds::GameboyPocket),  tr("Game Boy (Pocket)"),  createGameboyPocket,  2 });
    registerPalette({ pid(PaletteIds::Cga0),           tr("CGA palette 0"),     createCga0,           2 });
    registerPalette({ pid(PaletteIds::Cga1),           tr("CGA palette 1"),     createCga1,           2 });
    registerPalette({ pid(PaletteIds::CgaHigh),        tr("CGA high"),          createCgaHigh,        2 });
    registerPalette({ pid(PaletteIds::GreenAmber),     tr("Green-amber"),       createGreenAmber2bpp, 2 });
    registerPalette({ pid(PaletteIds::Neon2),          tr("Neon 2bpp"),         createNeon2bpp,       2 });

    // ---------------- 3 bpp --------------------------------------------
    registerPalette({ pid(PaletteIds::Gray3),    tr("Grayscale 3bpp"),   createGray3bpp,  3 });
    registerPalette({ pid(PaletteIds::Rgb332),   tr("RGB 3-3-2"),        createRgb332,    3 });
    registerPalette({ pid(PaletteIds::Rainbow8), tr("Bright rainbow 8"), createRainbow8,  3 });
    registerPalette({ pid(PaletteIds::Warm3),    tr("Warm 3bpp"),        createWarm3bpp,  3 });
    registerPalette({ pid(PaletteIds::Cool3),    tr("Cool 3bpp"),        createCool3bpp,  3 });

    // ---------------- 4 bpp --------------------------------------------
    registerPalette({ pid(PaletteIds::Gray4),         tr("Grayscale 4bpp"),    createGray4bpp,        4 });
    registerPalette({ pid(PaletteIds::Ega16),         tr("EGA 16 colors"),     createEga16,           4 });
    registerPalette({ pid(PaletteIds::EgaAlt),        tr("EGA alternative"),   createEgaAlt,          4 });
    registerPalette({ pid(PaletteIds::Windows16),     tr("Windows 16 colors"), createWindows16,       4 });
    registerPalette({ pid(PaletteIds::Commodore64),   tr("Commodore 64"),      createC64,             4 });
    registerPalette({ pid(PaletteIds::ZxSpectrum),    tr("ZX Spectrum"),       createZx,              4 });
    registerPalette({ pid(PaletteIds::AmberCrt),      tr("Amber CRT"),         createAmberCrt,        4 });
    registerPalette({ pid(PaletteIds::GreenTerminal), tr("Green terminal"),    createGreenTerminal,   4 });
    registerPalette({ pid(PaletteIds::Heat16),        tr("Heat gradient 16"),  createHeat16,          4 });
    registerPalette({ pid(PaletteIds::Rainbow16),     tr("Rainbow 16"),        createRainbow16,       4 });

    // ---------------- 8 bpp --------------------------------------------
    // Non-configurable base
    registerPalette({ pid(PaletteIds::Gray),        tr("Grayscale"),    createGrayscale });
    registerPalette({ pid(PaletteIds::Inverted),    tr("Inverted"),     createInverted });
    registerPalette({ pid(PaletteIds::Heat),        tr("Heat"),         createHeat });
    registerPalette({ pid(PaletteIds::Thermal),     tr("Thermal"),      createThermal });
    registerPalette({ pid(PaletteIds::Viridis),     tr("Viridis"),      createViridis });
    registerPalette({ pid(PaletteIds::Plasma),      tr("Plasma"),       createPlasma });
    registerPalette({ pid(PaletteIds::Rainbow),     tr("Rainbow"),      createRainbow });
    registerPalette({ pid(PaletteIds::Hsv),         tr("HSV"),          createHsv });
    registerPalette({ pid(PaletteIds::Solarize),    tr("Solarize"),     createSolarize });
    registerPalette({ pid(PaletteIds::HashColor),   tr("Hash color"),   createHashColor });
    registerPalette({ pid(PaletteIds::BitScale),    tr("Bit scale"),    createBitScale });
    registerPalette({ pid(PaletteIds::BitRotation), tr("Bit rotation"), createBitRotation });
    registerPalette({ pid(PaletteIds::ZerosOnes),   tr("Zeros/Ones"),   createZerosOnes });
    registerPalette({ pid(PaletteIds::Nonzero),     tr("Non-zero"),     createNonzero });
    registerPalette({ pid(PaletteIds::HighBit),     tr("High bit set"), createHighbit });
    registerPalette({ pid(PaletteIds::LowBit),      tr("Low bit set"),  createLowbit });
    registerPalette({ pid(PaletteIds::Nibbles),     tr("Nibbles"),      createNibbles });
    registerPalette({ pid(PaletteIds::HighNibble),  tr("High nibble"),  createHighNibble });
    registerPalette({ pid(PaletteIds::LowNibble),   tr("Low nibble"),   createLowNibble });

    // Configurable
    registerPalette({ pid(PaletteIds::Threshold), tr("Threshold"),     createSolarized, 8, true, thresholdDefaults() });
    registerPalette({ pid(PaletteIds::Ascii),     tr("ASCII"),         createAscii,     8, true, asciiDefaults() });
    registerPalette({ pid(PaletteIds::Entropy),   tr("Entropy bands"), createEntropy,   8, true, entropyDefaults() });
    registerPalette({ pid(PaletteIds::Bitplane),  tr("Bit plane"),     createBitplane,  8, true, bitplaneDefaults() });

    // Data-driven
    registerPalette({ pid(PaletteIds::ByteFrequency), tr("Byte frequency"), createByteFrequency });
    registerPalette({ pid(PaletteIds::Repeats),       tr("Repeats"),        createRepeatsHighlight, 8, true, repeatsDefaults() });
    registerPalette({ pid(PaletteIds::Custom),        tr("Custom"),         createCustom,           8, true, customDefaults() });

    // from_file is available from both 4 bpp and 8 bpp lists. bpp=8 is the
    // primary registration; extraGroups={4} adds it to the 4 bpp list. This
    // preserves the previous group order (from_file is last in bpp 8 and
    // second-to-last in bpp 4, right before the sampled palettes).
    registerPalette({ pid(PaletteIds::FromFile), tr("From file"),
                      createFromFile, 8, true, fromFileDefaults(),
                      /*extraGroups=*/QVector<int>{4} });

    // ---------------- 16 bpp -------------------------------------------
    registerPalette({ pid(PaletteIds::Rgb565),          tr("RGB565"),                createMultibytePlaceholder, 16 });
    registerPalette({ pid(PaletteIds::Bgr565),          tr("BGR565"),                createMultibytePlaceholder, 16 });
    registerPalette({ pid(PaletteIds::Rgb555),          tr("RGB555"),                createMultibytePlaceholder, 16 });
    registerPalette({ pid(PaletteIds::Argb1555),        tr("ARGB1555"),              createMultibytePlaceholder, 16 });
    registerPalette({ pid(PaletteIds::Integer16),       tr("Integer 16 (unsigned)"), createMultibytePlaceholder, 16, false, {{"signed", false}} });
    registerPalette({ pid(PaletteIds::Integer16Signed), tr("Integer 16 (signed)"),   createMultibytePlaceholder, 16, false, {{"signed", true}} });

    // ---------------- 24 bpp -------------------------------------------
    registerPalette({ pid(PaletteIds::Rgb), tr("RGB"), createMultibytePlaceholder, 24 });
    registerPalette({ pid(PaletteIds::Bgr), tr("BGR"), createMultibytePlaceholder, 24 });
    registerPalette({ pid(PaletteIds::Gbr), tr("GBR"), createMultibytePlaceholder, 24 });
    registerPalette({ pid(PaletteIds::Brg), tr("BRG"), createMultibytePlaceholder, 24 });
    registerPalette({ pid(PaletteIds::Grb), tr("GRB"), createMultibytePlaceholder, 24 });
    registerPalette({ pid(PaletteIds::Rbg), tr("RBG"), createMultibytePlaceholder, 24 });

    // ---------------- 32 bpp -------------------------------------------
    registerPalette({ pid(PaletteIds::Rgba),            tr("RGBA"),                  createMultibytePlaceholder, 32 });
    registerPalette({ pid(PaletteIds::Argb),            tr("ARGB"),                  createMultibytePlaceholder, 32 });
    registerPalette({ pid(PaletteIds::Bgra),            tr("BGRA"),                  createMultibytePlaceholder, 32 });
    registerPalette({ pid(PaletteIds::Abgr),            tr("ABGR"),                  createMultibytePlaceholder, 32 });
    registerPalette({ pid(PaletteIds::Integer32),       tr("Integer 32 (unsigned)"), createMultibytePlaceholder, 32, false, {{"signed", false}} });
    registerPalette({ pid(PaletteIds::Integer32Signed), tr("Integer 32 (signed)"),   createMultibytePlaceholder, 32, false, {{"signed", true}} });
    registerPalette({ pid(PaletteIds::Float32),         tr("Float 32"),              createMultibytePlaceholder, 32 });

    // ---------------- 64 bpp -------------------------------------------
    registerPalette({ pid(PaletteIds::Integer64),       tr("Integer 64 (unsigned)"), createMultibytePlaceholder, 64, false, {{"signed", false}} });
    registerPalette({ pid(PaletteIds::Integer64Signed), tr("Integer 64 (signed)"),   createMultibytePlaceholder, 64, false, {{"signed", true}} });
    registerPalette({ pid(PaletteIds::Float64),         tr("Float 64"),              createMultibytePlaceholder, 64 });
}

// =========================================================================
//  Sampled palettes for 2/3/4 bpp
// =========================================================================

void PaletteManager::registerSampledPalettes()
{
    struct Source {
        const char *id;
        QString     name;
        PaletteGenFn fn;
    };
    const Source sources[] = {
        { PaletteIds::Heat,      tr("Heat"),       createHeat      },
        { PaletteIds::Viridis,   tr("Viridis"),    createViridis   },
        { PaletteIds::Plasma,    tr("Plasma"),     createPlasma    },
        { PaletteIds::Rainbow,   tr("Rainbow"),    createRainbow   },
        { PaletteIds::Solarize,  tr("Solarize"),   createSolarize  },
        { PaletteIds::HashColor, tr("Hash color"), createHashColor },
    };

    for (int bpp : PM_SAMPLED_BPPS) {
        const int n = 1 << bpp;
        for (const Source &s : sources) {
            const QString paletteId =
                QString("%1_%2").arg(s.id).arg(bpp);
            const QString paletteDisplayName =
                QString("%1 %2bpp").arg(s.name).arg(bpp);

            PaletteGenFn src = s.fn;
            PaletteGenFn sampled = [src, n](const QVariantMap &) -> QVector<QColor> {
                const QVector<QColor> full = src(QVariantMap());
                if (full.isEmpty())
                    return {};
                QVector<QColor> out;
                out.reserve(n);
                if (n == 1) {
                    out.append(full.first());
                    return out;
                }
                if (n >= full.size())
                    return full;
                const double step = double(full.size() - 1) / (n - 1);
                for (int i = 0; i < n; ++i) {
                    const int idx = int(std::round(i * step));
                    out.append(full.value(qBound(0, idx, full.size() - 1)));
                }
                return out;
            };

            registerPalette({ paletteId, paletteDisplayName, sampled, bpp });
        }
    }
}

// =========================================================================
//  Public methods
// =========================================================================

QStringList PaletteManager::paletteIdsForBpp(int bpp) const
{
    return m_groups.value(bpp);
}

QStringList PaletteManager::allPaletteIds() const
{
    return m_generators.keys();
}

QString PaletteManager::displayName(const QString &id) const
{
    return m_names.value(id);
}

bool PaletteManager::contains(const QString &id) const
{
    return m_generators.contains(id) && m_configs.contains(id);
}

int PaletteManager::bytesPerPixel(const QString &id) const
{
    return m_configs.value(id).bytesPerPixel;
}

QVariantMap PaletteManager::defaultParams(const QString &id) const
{
    return m_configs.value(id).defaultParams;
}

bool PaletteManager::isConfigurable(const QString &id) const
{
    return m_configs.value(id).configurable;
}

QVector<QColor> PaletteManager::createPalette(const QString &id,
                                              const QVariantMap &paramsIn,
                                              const QByteArray &fileData,
                                              const quint64 *byteHistogram) const
{
    QVariantMap params = paramsIn;

    if (id == QLatin1String(PaletteIds::ByteFrequency)) {
        params = prepareByteFrequency(params, fileData, byteHistogram);
    } else if (id == QLatin1String(PaletteIds::Repeats)) {
        params = prepareRepeats(params, fileData, byteHistogram);
    } else if (id == QLatin1String(PaletteIds::FromFile)) {
        params = prepareFromFile(params, fileData);
    }

    auto it = m_generators.constFind(id);
    if (it != m_generators.constEnd())
        return it.value()(params);

    return grayPalette(PM_DEFAULT_PALETTE_SIZE);
}