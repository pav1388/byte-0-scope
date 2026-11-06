// Byte-0-scope - binary data visualizer
// File: byteinfo.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "byteinfo.h"
#include "mainwindow.h"
#include "pixeldecoder.h"
#include "pixelmath.h"
#include "thememanager.h"

#include <QCursor>
#include <QFont>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <algorithm>

#define BI_FONT_SIZE 9

namespace {

// Беззнаковый hex: 64-битные значения со старшим битом не должны выводиться
// как отрицательные числа.
QString hexU(quint64 v, int width)
{
    return QString::number(v, 16).toUpper().rightJustified(width, QChar('0'));
}

QString emptyInfoText()
{
    return QStringLiteral("<br><br><br><br>");
}

int hexDigitsForBpp(int bpp)
{
    if (bpp <= 8)   return 2;
    if (bpp <= 16)  return 4;
    if (bpp <= 24)  return 6;
    if (bpp <= 32)  return 8;
    return 16;
}

// Перестановка байтов пикселя в «пиксельный порядок».
// little — reverse, big — как есть.
QByteArray reorderPixelBytes(const QByteArray &raw, const QString &byteOrder)
{
    if (byteOrder == QLatin1String("little")) {
        QByteArray r = raw;
        std::reverse(r.begin(), r.end());
        return r;
    }
    return raw;
}

// Bit-reverse всех байтов (для bit_order = lsb в многобайтовых форматах).
QByteArray reverseBitsPerByte(const QByteArray &raw)
{
    QByteArray r = raw;
    for (int i = 0; i < r.size(); ++i)
        r[i] = char(PixelDecoder::reverseBits8(quint8(r.at(i))));
    return r;
}

QString bytesToHex(const QByteArray &b)
{
    QStringList parts;
    parts.reserve(b.size());
    for (int i = 0; i < b.size(); ++i)
        parts << hexU(quint8(b.at(i)), 2);
    return parts.join(' ');
}

QString byteToBin(quint8 v)
{
    return QString::number(v, 2).rightJustified(8, QChar('0'));
}

} // namespace

ByteInfoWidget::ByteInfoWidget(MainWindow *parent)
    : QWidget(parent), m_main(parent)
{
    QFont mono(QStringLiteral("Monospace"));
    mono.setStyleHint(QFont::TypeWriter, QFont::PreferQuality);
    mono.setPointSize(BI_FONT_SIZE);
    mono.setFixedPitch(true);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_label = new QLabel(this);
    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_label->setWordWrap(true);
    m_label->setFont(mono);
    m_label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_label->setMinimumHeight(100);
    m_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_label->setText(emptyInfoText());
    lay->addWidget(m_label);
    lay->addStretch();

    m_popup = new QLabel(this, Qt::Tool | Qt::FramelessWindowHint);
    m_popup->setAttribute(Qt::WA_ShowWithoutActivating);
    m_popup->setFont(mono);
    m_popup->hide();

    updateTheme();
}

void ByteInfoWidget::updateTheme()
{
    if (!m_main || !m_main->themeManager())
        return;
    m_popup->setStyleSheet(
        m_main->themeManager()->current().styles.value("byte_info"));
}

// =========================================================================
//  bp == 1 (8bpp single byte)
// =========================================================================

QString ByteInfoWidget::formatSingleByte(qint64 addr, quint8 rawByte)
{
    QStringList lines;

    lines << QString("&nbsp;Addr:&nbsp;<b>0x%1</b>&nbsp;(%2)")
                 .arg(hexU(addr, 8)).arg(addr);

    const bool rev = (m_main->bitOrder() == QLatin1String("lsb"));

    QVector<int> ctx;
    ctx.reserve(11);
    for (int off = -5; off <= 5; ++off) {
        const qint64 pos = addr + off;
        if (m_main && pos >= 0 && pos < qint64(m_main->fileSize())) {
            quint8 b = m_main->readByte(pos);
            if (rev)
                b = PixelDecoder::reverseBits8(b);
            ctx.append(int(b));
        } else {
            ctx.append(-1);
        }
    }

    QStringList hexParts;
    hexParts.reserve(ctx.size());
    for (int i = 0; i < ctx.size(); ++i) {
        const QString h = (ctx[i] >= 0) ? hexU(ctx[i], 2) : QStringLiteral("--");
        hexParts << (i == 5 ? QString("<b>%1</b>").arg(h) : h);
    }
    lines << "&nbsp;&nbsp;Hex:&nbsp;"
          + hexParts.mid(3, 2).join("&nbsp;")
          + "&nbsp;>&nbsp;" + hexParts[5]
          + "&nbsp;>&nbsp;" + hexParts.mid(6, 2).join("&nbsp;");

    QStringList asciiParts;
    asciiParts.reserve(ctx.size());
    for (int v : ctx) {
        if (v >= 33 && v <= 126)
            asciiParts << QString(QChar(v)).toHtmlEscaped();
        else
            asciiParts << QStringLiteral("&nbsp;");
    }

    const QString asciiLeft  = asciiParts.mid(0, 5).join("");
    const QString asciiRight = asciiParts.mid(6, 5).join("");
    lines << QString("ASCII:&nbsp;%1&nbsp;>&nbsp;<b>%2</b>&nbsp;&nbsp;>&nbsp;%3")
                 .arg(asciiLeft, asciiParts.value(5), asciiRight);

    quint8 value;
    if (ctx[5] >= 0) {
        value = quint8(ctx[5]);
    } else {
        value = rawByte;
        if (rev)
            value = PixelDecoder::reverseBits8(value);
    }

    lines << QString("&nbsp;&nbsp;Bin:&nbsp;<b>%1</b>&nbsp;&nbsp;&nbsp;Dec:&nbsp;<b>%2</b>")
                 .arg(byteToBin(value))
                 .arg(value);

    return lines.join("<br>");
}

// =========================================================================
//  isBitpacked() (1/2/3/4 bpp)
// =========================================================================

// 1–4 bpp: курсор выбирает не байт, а конкретный пиксель (бит/ниббл) внутри
// него. bitOffset — позиция первого бита пикселя в байте addr (0..7) в
// порядке чтения потока, поэтому она же индекс символа в строке Bin.
QString ByteInfoWidget::formatBitpacked(qint64 addr, int bitOffset)
{
    const QByteArray raw = m_main->readRange(addr, 2);
    if (raw.isEmpty())
        return emptyInfoText();

    const int bpp = qBound(1, m_main->bppMode(), 8);
    const bool bitLsb = (m_main->bitOrder() == QLatin1String("lsb"));
    const bool pixLsb = (m_main->pixelOrder() == QLatin1String("lsb"));
    const int offset = qBound(0, bitOffset, 7);

    const uchar *bytes = reinterpret_cast<const uchar *>(raw.constData());
    const unsigned pixel = PixelMath::bitpackedValue(
        bytes, raw.size(), offset, bpp, bitLsb, pixLsb);

    // Байт в интерпретированном виде: при bit_order == lsb биты
    // реверсируются (так BitPackedReader их и читает).
    auto interpret = [bitLsb](quint8 v) -> quint8 {
        return bitLsb ? PixelDecoder::reverseBits8(v) : v;
    };

    // Бинарная строка байта с подчёркнутыми битами выбранного пикселя
    // (from..to включительно, индексы символов слева направо).
    auto binMarked = [](quint8 v, int from, int to) -> QString {
        const QString bin = byteToBin(v);
        if (from > to)
            return bin;
        return bin.left(from) + QStringLiteral("<u><b>")
             + bin.mid(from, to - from + 1) + QStringLiteral("</b></u>")
             + bin.mid(to + 1);
    };

    const int lastBit = offset + bpp - 1;           // может выйти за байт (3 bpp)
    const bool spans = lastBit > 7 && raw.size() > 1;
    const quint8 value = interpret(quint8(bytes[0]));

    QStringList lines;
    lines << QString("Addr:&nbsp;<b>0x%1</b>&nbsp;(%2)")
                 .arg(hexU(addr, 8)).arg(addr);
    lines << QString("&nbsp;Hex:&nbsp;<b>%1</b>").arg(hexU(value, 2));
    lines << QString("&nbsp;Bin:&nbsp;<b>%1</b>&nbsp;&nbsp;Dec:&nbsp;<b>%2</b>")
                 .arg(binMarked(value, offset, qMin(7, lastBit)))
                 .arg(value);

    if (spans) {
        const quint8 next = interpret(quint8(bytes[1]));
        lines << QString("Next:&nbsp;<b>0x%1</b>&nbsp;Bin:&nbsp;<b>%2</b>")
                     .arg(hexU(addr + 1, 8))
                     .arg(binMarked(next, 0, lastBit - 8));
    }

    const QString bits = (bpp == 1)
        ? QString::number(offset)
        : QString("%1-%2").arg(offset).arg(lastBit);
    lines << QString("Pixel:&nbsp;<b>%1</b>&nbsp;(%2&nbsp;bpp,&nbsp;bit&nbsp;%3)")
                 .arg(pixel).arg(bpp).arg(bits);

    return lines.join("<br>");
}

// =========================================================================
//  bp > 1 (16/24/32/64)
// =========================================================================

QString ByteInfoWidget::formatMultibyte(qint64 addr,
                                        const QByteArray &dataBytes,
                                        const QString &paletteId,
                                        const QVariantMap &params)
{
    QStringList lines;

    const int bp = dataBytes.size();
    const QString bOrd = m_main->bitOrder();
    const QString byteOrd = m_main->byteOrder();
    const bool bitLsb = (bOrd == QLatin1String("lsb"));

    lines << QString("&nbsp;Addr:&nbsp;<b>0x%1</b>&nbsp;(%2)")
                 .arg(hexU(addr, 8)).arg(addr);

    // Байты в том виде, в котором их видит декодер:
    // 1) byte_order: little → reverse порядка байтов;
    // 2) bit_order:  lsb    → побитовый reverse каждого байта.
    QByteArray pixelBytes = reorderPixelBytes(dataBytes, byteOrd);
    if (bitLsb)
        pixelBytes = reverseBitsPerByte(pixelBytes);

    // Целое значение, собранное из интерпретированных байтов (BE).
    quint64 value = 0;
    for (int i = 0; i < pixelBytes.size() && i < 8; ++i)
        value = (value << 8) | quint8(pixelBytes.at(i));

    const int digits = hexDigitsForBpp(bp * 8);
    lines << QString("&nbsp;&nbsp;Hex:&nbsp;<b>%1</b>").arg(hexU(value, digits));
    lines << QString("&nbsp;&nbsp;Dec:&nbsp;<b>%1</b>").arg(value);

    // Декодированные каналы/поля.
    const QString desc = PixelDecoder::describe(dataBytes, paletteId,
                                                bOrd, byteOrd, params);
    if (!desc.isEmpty())
        lines << desc;

    return lines.join("<br>");
}

// =========================================================================
//  Публичный слот
// =========================================================================

void ByteInfoWidget::updateInfo(qint64 addr, int bitOffset)
{
    if (!m_main || addr < 0 || addr >= qint64(m_main->fileSize())) {
        m_label->setText(emptyInfoText());
        m_popup->hide();
        return;
    }

    QString text;

    if (m_main->isBitpacked()) {
        text = formatBitpacked(addr, bitOffset);
    } else if (m_main->bytesPerPixel() == 1) {
        text = formatSingleByte(addr, m_main->readByte(addr));
    } else {
        const int bp = qMax(1, m_main->bytesPerPixel());
        const QByteArray data = m_main->readRange(addr, bp);
        if (data.isEmpty()) {
            m_label->setText(emptyInfoText());
            m_popup->hide();
            return;
        }
        const QString pal = m_main->currentPaletteId();
        const QVariantMap params = m_main->paletteParams().value(pal).toMap();
        text = formatMultibyte(addr, data, pal, params);
    }

    m_label->setText(text);

    if (m_popupEnabled) {
        m_popup->setText(text);
        m_popup->adjustSize();
        const QPoint gp = QCursor::pos();
        m_popup->move(gp.x() + 20, gp.y() + 20);
        m_popup->show();
    } else {
        m_popup->hide();
    }
}
