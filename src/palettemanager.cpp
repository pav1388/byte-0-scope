// Byte-0-scope - binary data visualizer
// File: palettemanager.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "palettemanager.h"
#include "pixeldecoder.h"

#include <QCoreApplication>
#include <QSet>
#include <QtMath>
#include <algorithm>
#include <cmath>

// --- Настройки модуля PaletteManager (префикс PM_) ---

// Базовый размер 8-битных и мультибайтовых палитр
#define PM_DEFAULT_PALETTE_SIZE     256

// Палитра-фолбэк, если запрошенный id не найден
#define PM_GRAY_FALLBACK_ID         "gray"

// Дефолтные параметры конфигурируемых палитр
#define PM_THRESHOLD_DEFAULT        64
#define PM_ENTROPY_STEP_DEFAULT     16
#define PM_BITPLANE_DEFAULT         0
#define PM_MIN_COUNT_DEFAULT        50
#define PM_FROM_FILE_LENGTH_DEFAULT 256

// bpp, для которых генерируются сэмплированные варианты базовых палитр
#define PM_SAMPLED_BPPS             { 2, 3, 4 }


// =========================================================================
//  8-битные базовые палитры
// =========================================================================
namespace {
	
inline QString tr(const char *s)
{
    return QCoreApplication::translate("PaletteManager", s);
}


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

// --- Базовые 8-битные ---
QVector<QColor> createGrayscale(const QVariantMap &)
{
    return grayPalette(PM_DEFAULT_PALETTE_SIZE);
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

QVector<QColor> createSolarized(const QVariantMap &params) // «Порог»
{
    const int threshold = params.value("threshold", PM_THRESHOLD_DEFAULT).toInt();
    QVector<QColor> o;
    o.reserve(PM_DEFAULT_PALETTE_SIZE);
    for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
        o.append(i < threshold ? QColor(7, 54, 101) : QColor(88, 110, 122));
    return o;
}

QVector<QColor> createEntropy(const QVariantMap &params) // «Полосы энтропии»
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

QVector<QColor> createSolarize(const QVariantMap &) // «Соляризация»
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

// --- 1 bpp ---
QVector<QColor> createMono1bpp(const QVariantMap &)     { return { QColor(0, 0, 0),     QColor(255, 255, 255) }; }
QVector<QColor> createMonoInverted(const QVariantMap &) { return { QColor(255, 255, 255), QColor(0, 0, 0)     }; }
QVector<QColor> createMonoAmber(const QVariantMap &)    { return { QColor(0, 0, 0),     QColor(255, 176, 0)   }; }
QVector<QColor> createMonoGreen(const QVariantMap &)    { return { QColor(0, 0, 0),     QColor(0, 255, 0)     }; }
QVector<QColor> createMonoRed(const QVariantMap &)      { return { QColor(0, 0, 0),     QColor(255, 0, 0)     }; }
QVector<QColor> createMonoBlue(const QVariantMap &)     { return { QColor(0, 0, 0),     QColor(80, 160, 255)  }; }

// --- 2 bpp ---
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

// --- 3 bpp ---
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

// --- 4 bpp ---
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

// --- «Пользовательская» ---
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

// --- «Из файла» ---
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

    auto fallback = [count]() -> QVector<QColor> {
        const int n = qMax(1, count);
        if (n <= 1)
            return { QColor(0, 0, 0) };
        QVector<QColor> o;
        o.reserve(n);
        for (int i = 0; i < n; ++i) {
            const int v = int(i * 255 / (n - 1));
            o.append(QColor(v, v, v));
        }
        return o;
    };

    if (data.isEmpty())
        return fallback();

    QVector<QColor> out;
    out.reserve(count);
    for (int i = 0; i < count; ++i) {
        const int start = i * recSize;
        if (start + recSize > data.size())
            break;
        QByteArray rec = data.mid(start, recSize);
        out.append(PixelDecoder::interpretPaletteRecord(rec, fmt, recSize, end));
    }
    return out.isEmpty() ? fallback() : out;
}

// --- «Частота байтов» ---
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

// --- «Повторы» ---
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

// --- Подготовительные шаги ---
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
    const int minCount = params.value("min_count", PM_MIN_COUNT_DEFAULT).toInt();
    QVariantList list;

    if (byteHistogram) {
        for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
            if (byteHistogram[i] >= quint64(minCount))
                list.append(i);
    } else if (!fileData.isEmpty()) {
        quint64 counts[PM_DEFAULT_PALETTE_SIZE] = {0};
        const uchar *p = reinterpret_cast<const uchar *>(fileData.constData());
        const int n = fileData.size();
        for (int i = 0; i < n; ++i)
            ++counts[p[i]];
        for (int i = 0; i < PM_DEFAULT_PALETTE_SIZE; ++i)
            if (counts[i] >= quint64(minCount))
                list.append(i);
    } else {
        return params;
    }

    QVariantMap out;
    out["repeats"] = list;
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

    const int endPos = address + count * recordSize;
    QByteArray slice = fileData.mid(address, endPos - address);

    QVariantMap out;
    out["data"] = slice;
    out["count"] = count;
    out["record_size"] = recordSize;
    out["bpp"] = bpp;
    out["format"] = fmt;
    out["endianness"] = end;
    return out;
}

} // namespace

// =========================================================================
//  Публичные статические
// =========================================================================

QVariantList PaletteManager::defaultCustomRules()
{
    static const char *colors[] = {
        "#FF0000", "#00FF00", "#0000FF", "#FF0080",
        "#80FF00", "#0080FF", "#FF8000", "#8000FF"
    };
    QVariantList rules;
    for (int i = 0; i < 8; ++i) {
        QVariantMap r;
        r["start"] = i * 32;
        r["end"]   = i * 32 + 31;
        r["color"] = QString(colors[i]);
        rules.append(r);
    }
    return rules;
}

QVariantList PaletteManager::samplePalette(const QVariantList &source, int n)
{
    QVariantList out;
    if (source.isEmpty() || n <= 0)
        return out;

    if (n == 1) {
        out.append(source.first());
        return out;
    }

    const int len = source.size();
    if (n >= len)
        return source;

    const double step = double(len - 1) / (n - 1);
    for (int i = 0; i < n; ++i) {
        const int idx = int(std::round(i * step));
        out.append(source.value(qBound(0, idx, len - 1)));
    }
    return out;
}

// =========================================================================
//  Конструктор
// =========================================================================

PaletteManager::PaletteManager()
{
    registerBaseGenerators();
    registerBaseConfigs();
    registerSampledPalettes();
}

// =========================================================================
//  Регистрация генераторов
// =========================================================================

void PaletteManager::registerBaseGenerators()
{
    auto add = [this](const QString &id, GenFn f) { m_generators[id] = f; };

    // 8 bpp
    add("gray",             createGrayscale);
    add("inverted",         createInverted);
    add("heat",             createHeat);
    add("thermal",          createThermal);
    add("viridis",          createViridis);
    add("plasma",           createPlasma);
    add("rainbow",          createRainbow);
    add("hsv",              createHsv);
    add("solarize",         createSolarize);
    add("hash_color",       createHashColor);
    add("bit_scale",        createBitScale);
    add("bit_rotation",     createBitRotation);
    add("zeros_ones",       createZerosOnes);
    add("nonzero",          createNonzero);
    add("highbit",          createHighbit);
    add("lowbit",           createLowbit);
    add("nibbles",          createNibbles);
    add("high_nibble",      createHighNibble);
    add("low_nibble",       createLowNibble);
    add("threshold",        createSolarized);
    add("entropy",          createEntropy);
    add("bitplane",         createBitplane);
    add("custom",           createCustom);
    add("byte_frequency",   createByteFrequency);
    add("repeats",          createRepeatsHighlight);
    add("from_file",        createFromFile);

    // 1 bpp
    add("mono_1",           createMono1bpp);
    add("mono_inverted",    createMonoInverted);
    add("mono_amber",       createMonoAmber);
    add("mono_green",       createMonoGreen);
    add("mono_red",         createMonoRed);
    add("mono_blue",        createMonoBlue);

    // 2 bpp
    add("gray_2",           createGray2bpp);
    add("gameboy_classic",  createGameboyClassic);
    add("gameboy_pocket",   createGameboyPocket);
    add("cga_0",            createCga0);
    add("cga_1",            createCga1);
    add("cga_high",         createCgaHigh);
    add("green_amber",      createGreenAmber2bpp);
    add("neon_2",           createNeon2bpp);

    // 3 bpp
    add("gray_3",           createGray3bpp);
    add("rgb_332",          createRgb332);
    add("rainbow_8",        createRainbow8);
    add("warm_3",           createWarm3bpp);
    add("cool_3",           createCool3bpp);

    // 4 bpp
    add("gray_4",           createGray4bpp);
    add("ega_16",           createEga16);
    add("ega_alt",          createEgaAlt);
    add("windows_16",       createWindows16);
    add("commodore_64",     createC64);
    add("zx_spectrum",      createZx);
    add("amber_crt",        createAmberCrt);
    add("green_terminal",   createGreenTerminal);
    add("heat_16",          createHeat16);
    add("rainbow_16",       createRainbow16);

    // --- Мультибайтовые форматы ---
    auto mb = [](const QVariantMap &) { return grayPalette(PM_DEFAULT_PALETTE_SIZE); };

    // 16 bpp
    add("rgb565",           mb);
    add("bgr565",           mb);
    add("rgb555",           mb);
    add("argb1555",         mb);
    add("integer_16",       mb);

    // 24 bpp
    add("rgb",              mb);
    add("bgr",              mb);
    add("gbr",              mb);
    add("brg",              mb);
    add("grb",              mb);
    add("rbg",              mb);

    // 32 bpp
    add("rgba",             mb);
    add("argb",             mb);
    add("bgra",             mb);
    add("abgr",             mb);
    add("integer_32",       mb);
    add("float_32",         mb);

    // 64 bpp
    add("integer_64",       mb);
    add("float_64",         mb);
}

// =========================================================================
//  Регистрация конфигов + групп
// =========================================================================

void PaletteManager::registerBaseConfigs()
{
    // Display names. Keys are stable IDs, not localizable values.
    m_names.insert(QStringLiteral("gray"),             tr("Grayscale"));
    m_names.insert(QStringLiteral("inverted"),         tr("Inverted"));
    m_names.insert(QStringLiteral("heat"),             tr("Heat"));
    m_names.insert(QStringLiteral("thermal"),          tr("Thermal"));
    m_names.insert(QStringLiteral("viridis"),          tr("Viridis"));
    m_names.insert(QStringLiteral("plasma"),           tr("Plasma"));
    m_names.insert(QStringLiteral("rainbow"),          tr("Rainbow"));
    m_names.insert(QStringLiteral("hsv"),              tr("HSV"));
    m_names.insert(QStringLiteral("solarize"),         tr("Solarize"));
    m_names.insert(QStringLiteral("hash_color"),       tr("Hash color"));
    m_names.insert(QStringLiteral("bit_scale"),        tr("Bit scale"));
    m_names.insert(QStringLiteral("bit_rotation"),     tr("Bit rotation"));
    m_names.insert(QStringLiteral("zeros_ones"),       tr("Zeros/Ones"));
    m_names.insert(QStringLiteral("nonzero"),          tr("Non-zero"));
    m_names.insert(QStringLiteral("highbit"),          tr("High bit set"));
    m_names.insert(QStringLiteral("lowbit"),           tr("Low bit set"));
    m_names.insert(QStringLiteral("nibbles"),          tr("Nibbles"));
    m_names.insert(QStringLiteral("high_nibble"),      tr("High nibble"));
    m_names.insert(QStringLiteral("low_nibble"),       tr("Low nibble"));
    m_names.insert(QStringLiteral("threshold"),        tr("Threshold"));
    m_names.insert(QStringLiteral("entropy"),          tr("Entropy bands"));
    m_names.insert(QStringLiteral("bitplane"),         tr("Bit plane"));
    m_names.insert(QStringLiteral("custom"),           tr("Custom"));
    m_names.insert(QStringLiteral("byte_frequency"),   tr("Byte frequency"));
    m_names.insert(QStringLiteral("repeats"),          tr("Repeats"));
    m_names.insert(QStringLiteral("from_file"),        tr("From file"));
    m_names.insert(QStringLiteral("mono_1"),           tr("Mono 1bpp"));
    m_names.insert(QStringLiteral("mono_inverted"),    tr("Inverted mono"));
    m_names.insert(QStringLiteral("mono_amber"),       tr("Amber mono"));
    m_names.insert(QStringLiteral("mono_green"),       tr("Green mono"));
    m_names.insert(QStringLiteral("mono_red"),         tr("Red mono"));
    m_names.insert(QStringLiteral("mono_blue"),        tr("Blue mono"));
    m_names.insert(QStringLiteral("gray_2"),           tr("Grayscale 2bpp"));
    m_names.insert(QStringLiteral("gameboy_classic"),  tr("Game Boy (classic)"));
    m_names.insert(QStringLiteral("gameboy_pocket"),   tr("Game Boy (Pocket)"));
    m_names.insert(QStringLiteral("cga_0"),            tr("CGA palette 0"));
    m_names.insert(QStringLiteral("cga_1"),            tr("CGA palette 1"));
    m_names.insert(QStringLiteral("cga_high"),         tr("CGA high"));
    m_names.insert(QStringLiteral("green_amber"),      tr("Green-amber"));
    m_names.insert(QStringLiteral("neon_2"),           tr("Neon 2bpp"));
    m_names.insert(QStringLiteral("gray_3"),           tr("Grayscale 3bpp"));
    m_names.insert(QStringLiteral("rgb_332"),          tr("RGB 3-3-2"));
    m_names.insert(QStringLiteral("rainbow_8"),        tr("Bright rainbow 8"));
    m_names.insert(QStringLiteral("warm_3"),           tr("Warm 3bpp"));
    m_names.insert(QStringLiteral("cool_3"),           tr("Cool 3bpp"));
    m_names.insert(QStringLiteral("gray_4"),           tr("Grayscale 4bpp"));
    m_names.insert(QStringLiteral("ega_16"),           tr("EGA 16 colors"));
    m_names.insert(QStringLiteral("ega_alt"),          tr("EGA alternative"));
    m_names.insert(QStringLiteral("windows_16"),       tr("Windows 16 colors"));
    m_names.insert(QStringLiteral("commodore_64"),     tr("Commodore 64"));
    m_names.insert(QStringLiteral("zx_spectrum"),      tr("ZX Spectrum"));
    m_names.insert(QStringLiteral("amber_crt"),        tr("Amber CRT"));
    m_names.insert(QStringLiteral("green_terminal"),   tr("Green terminal"));
    m_names.insert(QStringLiteral("heat_16"),          tr("Heat gradient 16"));
    m_names.insert(QStringLiteral("rainbow_16"),       tr("Rainbow 16"));
    m_names.insert(QStringLiteral("rgb565"),           tr("RGB565"));
    m_names.insert(QStringLiteral("bgr565"),           tr("BGR565"));
    m_names.insert(QStringLiteral("rgb555"),           tr("RGB555"));
    m_names.insert(QStringLiteral("argb1555"),         tr("ARGB1555"));
    m_names.insert(QStringLiteral("integer_16"),       tr("Integer 16"));
    m_names.insert(QStringLiteral("rgb"),              tr("RGB"));
    m_names.insert(QStringLiteral("bgr"),              tr("BGR"));
    m_names.insert(QStringLiteral("gbr"),              tr("GBR"));
    m_names.insert(QStringLiteral("brg"),              tr("BRG"));
    m_names.insert(QStringLiteral("grb"),              tr("GRB"));
    m_names.insert(QStringLiteral("rbg"),              tr("RBG"));
    m_names.insert(QStringLiteral("rgba"),             tr("RGBA"));
    m_names.insert(QStringLiteral("argb"),             tr("ARGB"));
    m_names.insert(QStringLiteral("bgra"),             tr("BGRA"));
    m_names.insert(QStringLiteral("abgr"),             tr("ABGR"));
    m_names.insert(QStringLiteral("integer_32"),       tr("Integer 32"));
    m_names.insert(QStringLiteral("float_32"),         tr("Float 32"));
    m_names.insert(QStringLiteral("integer_64"),       tr("Integer 64"));
    m_names.insert(QStringLiteral("float_64"),         tr("Float 64"));

    // --- группы по bpp ---
    m_groups[1]  = QStringList{"mono_1", "mono_inverted", "mono_amber",
                               "mono_green", "mono_red", "mono_blue"};
    m_groups[2]  = QStringList{"gray_2", "gameboy_classic",
                               "gameboy_pocket", "cga_0", "cga_1",
                               "cga_high", "green_amber", "neon_2"};
    m_groups[3]  = QStringList{"gray_3", "rgb_332", "rainbow_8",
                               "warm_3", "cool_3"};
    m_groups[4]  = QStringList{"gray_4", "ega_16", "ega_alt",
                               "windows_16", "commodore_64", "zx_spectrum",
                               "amber_crt", "green_terminal", "heat_16",
                               "rainbow_16"};
    m_groups[8]  = QStringList{"gray", "inverted", "heat", "thermal",
                               "viridis", "plasma", "rainbow", "hsv", "solarize",
                               "hash_color", "bit_scale", "bit_rotation",
                               "zeros_ones", "nonzero", "highbit",
                               "lowbit", "nibbles",
                               "high_nibble", "low_nibble",
                               "threshold", "entropy", "bitplane",
                               "byte_frequency", "repeats", "custom", "from_file"};
    m_groups[16] = QStringList{"rgb565", "bgr565", "rgb555", "argb1555", "integer_16"};
    m_groups[24] = QStringList{"rgb", "bgr", "gbr", "brg", "grb", "rbg"};
    m_groups[32] = QStringList{"rgba", "argb", "bgra", "abgr", "integer_32", "float_32"};
    m_groups[64] = QStringList{"integer_64", "float_64"};

    // --- конфиги для конфигурируемых палитр ---
    auto addCfg = [this](const QString &name, bool conf,
                         const QVariantMap &def, int bpp) {
        Config c;
        c.configurable   = conf;
        c.defaultParams  = def;
        c.bytesPerPixel  = bpp;
        m_configs[name] = c;
    };

    // Из файла
    {
        QVariantMap p;
        p["address"] = 0;
        p["length"] = PM_FROM_FILE_LENGTH_DEFAULT;
        p["record_size"] = 1;
        p["format"] = "auto";
        p["endianness"] = "big";
        p["count"] = 0;
        addCfg("from_file", true, p, 1);
    }

    // 8-bit configurable
    addCfg("threshold", true, {{"threshold", PM_THRESHOLD_DEFAULT}}, 1);
    addCfg("entropy",   true, {{"step", PM_ENTROPY_STEP_DEFAULT}}, 1);
    addCfg("bitplane",  true, {{"plane", PM_BITPLANE_DEFAULT}}, 1);
    addCfg("repeats",   true, {{"min_count", PM_MIN_COUNT_DEFAULT}}, 1);

    // Пользовательская
    {
        QVariantMap p;
        p["default_color"] = "#000000";
        p["rules"] = defaultCustomRules();
        addCfg("custom", true, p, 1);
    }

    // Мультибайтовые
    addCfg("rgb565",     true, {{"endianness", "big"}}, 2);
    addCfg("bgr565",     true, {{"endianness", "big"}}, 2);
    addCfg("rgb555",     true, {{"endianness", "big"}}, 2);
    addCfg("argb1555",   true, {{"endianness", "big"}}, 2);
    addCfg("integer_16", true, {{"endianness", "big"}, {"signed", false}}, 2);

    // 24 bpp — 6 отдельных неконфигурируемых палитр
    addCfg("rgb", false, QVariantMap(), 3);
    addCfg("bgr", false, QVariantMap(), 3);
    addCfg("gbr", false, QVariantMap(), 3);
    addCfg("brg", false, QVariantMap(), 3);
    addCfg("grb", false, QVariantMap(), 3);
    addCfg("rbg", false, QVariantMap(), 3);

    // 32 bpp — 4 перестановки RGBA
    addCfg("rgba", false, QVariantMap(), 4);
    addCfg("argb", false, QVariantMap(), 4);
    addCfg("bgra", false, QVariantMap(), 4);
    addCfg("abgr", false, QVariantMap(), 4);
    addCfg("integer_32", true, {{"endianness", "big"}, {"signed", false}}, 4);
    addCfg("float_32",   true, {{"endianness", "big"}}, 4);

    // 64 bpp
    addCfg("integer_64", true, {{"endianness", "big"}, {"signed", false}}, 8);
    addCfg("float_64",   true, {{"endianness", "big"}}, 8);

    // Не-конфигурируемые — чтобы bytesPerPixel/defaultParams были доступны.
    for (auto it = m_groups.constBegin(); it != m_groups.constEnd(); ++it) {
        for (const QString &n : it.value()) {
            if (m_configs.contains(n))
                continue;
            addCfg(n, false, QVariantMap(), 1);
        }
    }
}

// =========================================================================
//  Сэмплированные палитры для 2/3/4 bpp
// =========================================================================

void PaletteManager::registerSampledPalettes()
{
    struct Source {
        const char *id;
        QString     name;
        GenFn       fn;
    };
    const Source sources[] = {
        { "heat",       tr("Heat"),       createHeat      },
        { "viridis",    tr("Viridis"),    createViridis   },
        { "plasma",     tr("Plasma"),     createPlasma    },
        { "rainbow",    tr("Rainbow"),    createRainbow   },
        { "solarize",   tr("Solarize"),   createSolarize  },
        { "hash_color", tr("Hash color"), createHashColor },
    };

    for (int bpp : PM_SAMPLED_BPPS) {
        const int n = 1 << bpp;
        for (const Source &s : sources) {
            const QString paletteId =
                QString("%1_%2").arg(s.id).arg(bpp);
            const QString paletteDisplayName =
                QString("%1 %2bpp").arg(s.name).arg(bpp);

            GenFn src = s.fn;
            GenFn sampled = [src, n](const QVariantMap &) -> QVector<QColor> {
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

            m_generators[paletteId] = sampled;
            m_names[paletteId] = paletteDisplayName;

            if (!m_groups[bpp].contains(paletteId))
                m_groups[bpp].append(paletteId);

            Config c;
            c.configurable   = false;
            c.defaultParams  = QVariantMap();
            c.bytesPerPixel  = 1;
            m_configs[paletteId] = c;
        }
    }
}

// =========================================================================
//  Публичные методы
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

    if (id == "byte_frequency") {
        params = prepareByteFrequency(params, fileData, byteHistogram);
    } else if (id == "repeats") {
        params = prepareRepeats(params, fileData, byteHistogram);
    } else if (id == "from_file") {
        params = prepareFromFile(params, fileData);
    }

    auto it = m_generators.constFind(id);
    if (it != m_generators.constEnd())
        return it.value()(params);

    return grayPalette(PM_DEFAULT_PALETTE_SIZE);
}