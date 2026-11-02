// Byte-0-scope - binary data visualizer
// File: byteinfo.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "byteinfo.h"
#include "mainwindow.h"
#include "pixeldecoder.h"
#include "thememanager.h"

#include <QCursor>
#include <QFont>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <algorithm>

namespace {

QString hexU(qint64 v, int width)
{
    return QString::number(v, 16).toUpper().rightJustified(width, QChar('0'));
}

QString emptyInfoText()
{
    return QStringLiteral("&nbsp;<br>&nbsp;<br>&nbsp;<br>&nbsp;<br>&nbsp;");
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
    mono.setPointSize(10);
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

    // Контекст ±2 байта — сразу в интерпретированном виде.
    QVector<int> ctx;
    for (int off = -2; off <= 2; ++off) {
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

    // Hex: байты в том виде, в котором их видит декодер.
    QStringList hexParts;
    for (int i = 0; i < ctx.size(); ++i) {
        const QString h = (ctx[i] >= 0) ? hexU(ctx[i], 2) : QStringLiteral("--");
        hexParts << (i == 2 ? QString("<b>%1</b>").arg(h) : h);
    }
    lines << "&nbsp;&nbsp;Hex:&nbsp;"
          + hexParts.mid(0, 2).join("&nbsp;")
          + "&nbsp;>&nbsp;" + hexParts[2]
          + "&nbsp;>&nbsp;" + hexParts.mid(3).join("&nbsp;");

    // ASCII: тоже по интерпретированным значениям.
    QStringList asciiParts;
    for (int v : ctx) {
        if (v >= 32 && v <= 126)
            asciiParts << QString(QChar(v)).toHtmlEscaped();
        else
            asciiParts << QStringLiteral("&nbsp;");
    }
    lines << QString("ASCII:&nbsp;%1&nbsp;&nbsp;%2&nbsp;&nbsp;>&nbsp;<b>%3</b>&nbsp;&nbsp;>&nbsp;%4&nbsp;&nbsp;%5")
        .arg(asciiParts[0], asciiParts[1], asciiParts[2],
             asciiParts[3], asciiParts[4]);

    // Текущее значение центрального байта (уже bit-reversed при lsb).
    const quint8 value = quint8(ctx[2] < 0 ? rawByte : ctx[2]);

    lines << QString("&nbsp;&nbsp;Bin:&nbsp;<b>%1</b>&nbsp;&nbsp;&nbsp;Dec:&nbsp;<b>%2</b>")
                 .arg(byteToBin(value))
                 .arg(value);

    return lines.join("<br>");
}

// =========================================================================
//  isBitpacked() (1/2/3/4 bpp)
// =========================================================================

QString ByteInfoWidget::formatBitpacked(qint64 addr, quint8 rawByte)
{
    QStringList lines;

    const bool bitLsb = (m_main->bitOrder() == QLatin1String("lsb"));

    lines << QString("Addr:&nbsp;<b>0x%1</b>&nbsp;(%2)")
                 .arg(hexU(addr, 8)).arg(addr);

    // Байт в интерпретированном виде: при bit_order == lsb биты
    // реверсируются (так BitPackedReader их и читает).
    const quint8 value = bitLsb
        ? PixelDecoder::reverseBits8(rawByte)
        : rawByte;

	lines << QString("&nbsp;Hex:&nbsp;<b>%1</b>").arg(hexU(value, 2));
	lines << QString("&nbsp;Bin:&nbsp;<b>%1</b>&nbsp;&nbsp;Dec:&nbsp;<b>%2</b>")
				 .arg(byteToBin(value)).arg(value);

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
    lines << QString("&nbsp;&nbsp;Hex:&nbsp;<b>%1</b>").arg(hexU(qint64(value), digits));
	lines << QString("&nbsp;&nbsp;Dec:&nbsp;<b>%1</b>").arg(value);

    // Декодированные каналы/поля.
    // Передаём исходные байты (file order): decodeImpl сам применяет
    // byteOrder и bitOrder. pixelBytes тут не подходит — иначе
    // порядок применится дважды.
    const QString desc = PixelDecoder::describe(dataBytes, paletteId,
                                                bOrd, byteOrd, params);
    if (!desc.isEmpty())
        lines << desc;

    return lines.join("<br>");
}

// =========================================================================
//  Публичный слот
// =========================================================================

void ByteInfoWidget::updateInfo(qint64 addr)
{
    if (!m_main || addr < 0 || addr >= qint64(m_main->fileSize())) {
        m_label->setText(emptyInfoText());
        m_popup->hide();
        return;
    }

    QString text;

    if (m_main->isBitpacked()) {
        text = formatBitpacked(addr, m_main->readByte(addr));
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