// Byte-0-scope - binary data visualizer
// File: demobuilder.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "demobuilder.h"

#include <QDateTime>
#include <QtMath>

// --- Настройки модуля DemoBuilder (префикс DB_) ---

// Обычный demo
#define DB_DEMO_SIZE            256

// Обучающий файл
#define DB_TR_R                 96
#define DB_TR_H                 96
#define DB_TR_BPP               4
#define DB_TR_PAD_BITS          16
#define DB_TR_BG                7
#define DB_TR_FG                0

namespace DemoBuilder {

namespace {

// =========================================================================
//  Общий 5×5 пиксельный шрифт (используется в обучающем файле)
// =========================================================================
struct Glyph {
    char        ch;
    const char *rows[5];
};

const Glyph kGlyphs[] = {
    {'B', {"11110","10001","11110","10001","11110"}},
    {'O', {"01110","10001","10001","10001","01110"}},
    {'T', {"11111","00100","00100","00100","00100"}},
    {'K', {"10001","10010","11100","10010","10001"}},
    {'M', {"10001","11011","10101","10001","10001"}},
    {'D', {"11110","10001","10001","10001","11110"}},
    {'E', {"11111","10000","11110","10000","11111"}},
    {'A', {"01110","10001","11111","10001","10001"}},
    {'I', {"11111","00100","00100","00100","11111"}},
    {'N', {"10001","11001","10101","10011","10001"}},
    {'G', {"01110","10000","10011","10001","01110"}},
    {'P', {"11110","10001","11110","10000","10000"}},
    {'R', {"11110","10001","11110","10010","10001"}},
    {'S', {"01111","10000","01110","00001","11110"}},
    {'C', {"01110","10001","10000","10001","01110"}},
    {'L', {"10000","10000","10000","10000","11111"}},
    {'4', {"00010","00110","01010","11111","00010"}},
};

const Glyph *findGlyph(char ch)
{
    for (const Glyph &g : kGlyphs)
        if (g.ch == ch)
            return &g;
    return nullptr;
}

// 5×7 крупный шрифт для заголовка «BYTE».
struct Glyph7 {
    const char *rows[7];
};

const Glyph7 gBigB = {{"11110","10001","10001","11110","10001","10001","11110"}};
const Glyph7 gBigY = {{"10001","10001","01010","00100","00100","00100","00100"}};
const Glyph7 gBigT = {{"11111","00100","00100","00100","00100","00100","00100"}};
const Glyph7 gBigE = {{"11111","10000","10000","11110","10000","10000","11111"}};

} // namespace

// =========================================================================
//  Обычный demo
// =========================================================================
QByteArray buildDefaultDemo(qint64 msecsSinceEpoch)
{
    if (msecsSinceEpoch == 0)
        msecsSinceEpoch = QDateTime::currentMSecsSinceEpoch();

    const int which = int(msecsSinceEpoch / 1000) % 3;
    const int S = DB_DEMO_SIZE;

    QByteArray data(S * S, '\0');
    uchar *p = reinterpret_cast<uchar *>(data.data());

    if (which == 0) {
        // Синусоидальный шум
        for (int y = 0; y < S; ++y)
            for (int x = 0; x < S; ++x)
                p[y * S + x] = quint8(128.0 + 127.0 * qSin(x / 12.0) * qCos(y / 8.0));
        return data;
    }

    if (which == 1) {
        // XOR-узор
        for (int y = 0; y < S; ++y)
            for (int x = 0; x < S; ++x)
                p[y * S + x] = quint8((x ^ y) & 0xFF);
        return data;
    }

    // «Иконка» со стрелками и плашками
    const int W = S, H = S;

    for (int i = 0; i < H * W; ++i)
        p[i] = 150;

    auto set = [&](int y0, int y1, int x0, int x1, quint8 val) {
        if (y0 < 0) y0 = 0;
        if (x0 < 0) x0 = 0;
        if (y1 > H) y1 = H;
        if (x1 > W) x1 = W;
        for (int y = y0; y < y1; ++y) {
            uchar *row = p + y * W;
            for (int x = x0; x < x1; ++x)
                row[x] = val;
        }
    };

    set(0, 10, 0, 256, 0);
    set(2, 8,  10, 16, 255);
    set(2, 8,  20, 26, 255);
    set(2, 8,  30, 36, 255);

    for (int i = 20; i < 240; i += 40) {
        set(i,      i + 20, 0, 20, 255);
        set(i + 20, i + 40, 0, 20, 50);
    }

    for (int y = 20; y < 100; y += 20) {
        for (int x = 180; x < 256; x += 20) {
            const quint8 v = (((x + y) / 20) % 2 == 0) ? quint8(255) : quint8(0);
            set(y, y + 10, x, x + 10, v);
        }
    }

    for (int x = 80; x < 180; x += 10) {
        const quint8 v = ((x / 10) % 2 == 0) ? quint8(200) : quint8(80);
        set(100, 180, x, x + 5, v);
    }

    set(180, 256, 180, 256, 30);
    for (int y = 180; y < 256; y += 16)
        for (int x = 180; x < 256; x += 16)
            set(y, y + 8, x, x + 8, 180);

    set(246, 256, 0, 256, 0);
    for (int x = 0; x < 256; x += 40)
        set(248, 252, x, x + 4, 255);

    return data;
}

// =========================================================================
//  Обучающий demo
// =========================================================================
QByteArray buildTrainingDemo()
{
    const int W    = DB_TR_R;
    const int H    = DB_TR_H;
    const int BPP  = DB_TR_BPP;
    const int PADB = DB_TR_PAD_BITS;

    const int rowBits  = W * BPP + PADB;
    const int rowBytes = (rowBits + 7) / 8;

    QByteArray out(rowBytes * H, '\0');
    uchar *buf = reinterpret_cast<uchar *>(out.data());

    // Зеркалим по горизонтали
    auto setPixel = [&](int x, int y, int val) {
        if (x < 0 || y < 0 || x >= W || y >= H)
            return;
        val = qBound(0, val, 15);
        const int px = W - 1 - x;
        const int bitIndex = y * rowBits + px * BPP;
        for (int k = 0; k < BPP; ++k) {
            const int bit = (val >> k) & 1;
            const int globalBit = bitIndex + k;
            const int byteIdx   = globalBit / 8;
            const int bitInByte = globalBit % 8;
            if (bit)
                buf[byteIdx] |= uchar(1u << bitInByte);
            else
                buf[byteIdx] &= uchar(~(1u << bitInByte));
        }
    };

    auto fillRect = [&](int x0, int y0, int x1, int y1, int val) {
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                setPixel(x, y, val);
    };

    auto drawGlyph5x5 = [&](const char *const *rows, int gx, int gy, int scale, int val) {
        for (int r = 0; r < 5; ++r)
            for (int c = 0; c < 5; ++c)
                if (rows[r][c] == '1')
                    fillRect(gx + c * scale, gy + r * scale,
                             gx + (c + 1) * scale, gy + (r + 1) * scale, val);
    };

    auto drawGlyph7 = [&](const Glyph7 &g, int gx, int gy, int scale, int val) {
        for (int r = 0; r < 7; ++r)
            for (int c = 0; c < 5; ++c)
                if (g.rows[r][c] == '1')
                    fillRect(gx + c * scale, gy + r * scale,
                             gx + (c + 1) * scale, gy + (r + 1) * scale, val);
    };

    // =====================================================================
    //  Фон и рамка
    // =====================================================================
    fillRect(0, 0, W, H, DB_TR_BG);

    fillRect(0, 0, W, 2, DB_TR_FG);          // верх
    fillRect(0, H - 2, W, H, DB_TR_FG);      // низ
    fillRect(0, 0, 2, H, DB_TR_FG);          // лево
    fillRect(W - 2, 0, W, H, DB_TR_FG);      // право

    // =====================================================================
    //  Заголовок «BYTE» (7×5 × scale 3 = 21×15 px) — верхняя треть
    // =====================================================================
    {
        const int scale = 3;
        const int y     = 8;
        int x           = 8;
        drawGlyph7(gBigB, x, y, scale, DB_TR_FG); x += 5 * scale + 3;
        drawGlyph7(gBigY, x, y, scale, DB_TR_FG); x += 5 * scale + 3;
        drawGlyph7(gBigT, x, y, scale, DB_TR_FG); x += 5 * scale + 3;
        drawGlyph7(gBigE, x, y, scale, DB_TR_FG);
    }

    // =====================================================================
    //  Круг с градиентом (правый верхний угол)
    //  Центр (W-20, 22), радиус 14 — не пересекается с заголовком
    // =====================================================================
    {
        const int cx = W - 18;
        const int cy = 52;
        const int r  = 14;
        for (int y = cy - r; y <= cy + r; ++y) {
            for (int x = cx - r; x <= cx + r; ++x) {
                const int dx = x - cx, dy = y - cy;
                const int d2 = dx * dx + dy * dy;
                if (d2 <= r * r && d2 >= (r - 2) * (r - 2))
                    setPixel(x, y, DB_TR_FG);              // контур
                else if (d2 < (r - 2) * (r - 2))
                    setPixel(x, y, (dx + dy) & 0xF);       // внутренний градиент
            }
        }
    }

    // =====================================================================
    //  Разделительная горизонтальная линия под заголовком
    // =====================================================================
    fillRect(4, 30, W - 4, 31, DB_TR_FG);

    // =====================================================================
    //  Текстовые строки — 5×5, scale 1, межстрочный интервал 12 px
    // =====================================================================
    struct Line { int y; const char *text; };
    static const Line lines[] = {
        { 38, "BOOT OK" },
        { 50, "TRAINING" },
        { 62, "MODE 4BPP" },
        { 74, "CTRL+R" },
    };

    for (const Line &ln : lines) {
        int x = 8;
        for (const char *pc = ln.text; *pc; ++pc) {
            if (*pc == ' ') { x += 4; continue; }
            const Glyph *g = findGlyph(*pc);
            if (!g) { x += 6; continue; }
            drawGlyph5x5(g->rows, x, ln.y, 1, DB_TR_FG);
            x += 6;
        }
    }

    // =====================================================================
    //  Декоративная полоса снизу — вертикальные штрихи по 4 px
    // =====================================================================
    for (int x = 4; x < W - 4; x += 6)
        fillRect(x, H - 10, x + 2, H - 6, DB_TR_FG);

    return out;
}

// =========================================================================
//  Правильный конфиг для обучения
// =========================================================================
QVariantMap trainingCorrectConfig()
{
    QVariantMap cfg;
    cfg["bpp_mode"]         = DB_TR_BPP;
    cfg["bit_order"]        = "lsb";
    cfg["pixel_order"]      = "lsb";
    cfg["row_padding_bits"] = DB_TR_PAD_BITS;
    cfg["palette"]          = "inverted";
    cfg["width_mode"]       = "fixed";
    cfg["fixed_width"]      = DB_TR_R;
    cfg["scale"]            = 1;
    cfg["auto_scale"]       = false;
    cfg["fit_to_window"]    = false;
    cfg["offset"]           = 0;
    cfg["length"]           = 0;
    cfg["length_mode"]      = false;
    cfg["mirror_horizontal"] = true;
    cfg["mirror_vertical"]   = false;
    return cfg;
}

// =========================================================================
//  HTML подсказки
// =========================================================================
QString trainingHintHtml()
{
    return QStringLiteral(
        "<b>Обучение: восстановите изображение</b><br><br>"
        "Файл загружен, но текущие настройки отображения неверны — "
        "вы видите шум. Приведите параметры к правильным, "
        "и появятся заголовок «BYTE», текст и графика.<br><br>"
        "<b>Правильные параметры:</b><br>"
        "&nbsp;&nbsp;1. <b>Бит на пиксель</b> -> <code>4 bpp</code><br>"
        "&nbsp;&nbsp;2. <b>Порядок битов</b> -> <code>LSB-first (LE)</code><br>"
        "&nbsp;&nbsp;3. <b>Порядок пикселей</b> -> <code>LSB-first</code><br>"
        "&nbsp;&nbsp;4. <b>Паддинг (бит)</b> -> <code>16</code><br>"
        "&nbsp;&nbsp;5. <b>Палитра</b> -> <code>Любая</code><br>"
        "&nbsp;&nbsp;6. <b>Режим ширины</b> -> <code>Ручной</code>, "
        "ширина <code>96</code><br>"
        "&nbsp;&nbsp;7. <b>Вид -> Отразить по горизонтали</b><br><br>");
}

} // namespace DemoBuilder