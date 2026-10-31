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

namespace {

QString hexU(qint64 v, int width)
{
    return QString::number(v, 16).toUpper().rightJustified(width, QChar('0'));
}

QString emptyInfoText()
{
    return QStringLiteral("&nbsp;<br>&nbsp;<br>&nbsp;<br>&nbsp;<br>&nbsp;");
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

QString ByteInfoWidget::formatByteInfo(qint64 addr, quint8 byteVal)
{
    QStringList lines;
    lines << QString("&nbsp;Addr:&nbsp;<b>0x%1</b>&nbsp;(%2)")
                 .arg(hexU(addr, 8)).arg(addr);

    QVector<int> ctx;
    for (int off = -2; off <= 2; ++off) {
        const qint64 pos = addr + off;
        if (m_main && pos >= 0 && pos < qint64(m_main->fileSize()))
            ctx.append(int(m_main->readByte(pos)));
        else
            ctx.append(-1);
    }

    QStringList hexParts;
    for (int i = 0; i < ctx.size(); ++i) {
        if (i == 2)
            hexParts << QString("<b>%1</b>").arg(hexU(byteVal, 2));
        else if (ctx[i] >= 0)
            hexParts << hexU(ctx[i], 2);
        else
            hexParts << QStringLiteral("--");
    }
    const QString hexStr = hexParts.mid(0, 2).join("&nbsp;")
                         + "&nbsp;>&nbsp;" + hexParts[2]
                         + "&nbsp;>&nbsp;" + hexParts.mid(3).join("&nbsp;");
    lines << "&nbsp;&nbsp;Hex: " + hexStr;

    QStringList asciiParts;
    for (int v : ctx) {
        if (v >= 32 && v <= 126)
            asciiParts << QString(QChar(v)).toHtmlEscaped();
        else
            asciiParts << QStringLiteral("&nbsp;");
    }
    const QString asciiStr = QString("%1&nbsp;&nbsp;%2&nbsp;&nbsp;>&nbsp;<b>%3</b>&nbsp;&nbsp;>&nbsp;%4&nbsp;&nbsp;%5")
        .arg(asciiParts[0], asciiParts[1], asciiParts[2], asciiParts[3], asciiParts[4]);
    lines << "ASCII:&nbsp;" + asciiStr;
    lines << QString("&nbsp;&nbsp;Bin:&nbsp;<b>%1</b>&nbsp;&nbsp;&nbsp;Dec:&nbsp;<b>%2</b>")
                 .arg(QString::number(byteVal, 2).rightJustified(8, QChar('0')))
                 .arg(byteVal);
    return lines.join("<br>");
}

QString ByteInfoWidget::formatMultibyteInfo(qint64 addr, const QByteArray &dataBytes,
                                            const QString &paletteId,
                                            const QVariantMap &params)
{
    QStringList lines;
    lines << QString("&nbsp;Addr:&nbsp;<b>0x%1</b>&nbsp;(%2)").arg(hexU(addr, 8)).arg(addr);

    QStringList hexParts;
    for (int i = 0; i < dataBytes.size(); ++i)
        hexParts << hexU(quint8(dataBytes.at(i)), 2);
    lines << QString("&nbsp;Hex:&nbsp;<b>%1</b>").arg(hexParts.join(" "));

    const QString desc = PixelDecoder::describe(dataBytes, paletteId, params);
    if (!desc.isEmpty())
        lines << desc;

    return lines.join("<br>");
}

void ByteInfoWidget::updateInfo(qint64 addr)
{
    if (!m_main || addr < 0 || addr >= qint64(m_main->fileSize())) {
        m_label->setText(emptyInfoText());
        m_popup->hide();
        return;
    }

    QString text;

    if (m_main->isBitpacked()) {
        const quint8 v = m_main->readByte(addr);
        text = QString("Addr:&nbsp;<b>0x%1</b>&nbsp;(%2)<br>"
                       "&nbsp;Hex:&nbsp;<b>0x%3</b><br>"
                       "&nbsp;Bin:&nbsp;<b>%4</b>&nbsp;&nbsp;&nbsp;Dec:&nbsp;<b>%5</b>")
            .arg(hexU(addr, 8)).arg(addr)
            .arg(hexU(v, 2))
            .arg(QString::number(v, 2).rightJustified(8, QChar('0')))
            .arg(v);
    } else {
        const int bp = qMax(1, m_main->bytesPerPixel());
        QByteArray data = m_main->readRange(addr, bp);
        if (data.isEmpty()) {
            m_label->setText(emptyInfoText());
            m_popup->hide();
            return;
        }
        if (bp == 1) {
            text = formatByteInfo(addr, quint8(data.at(0)));
        } else {
            const QString pal = m_main->currentPaletteId();
            const QVariantMap params = m_main->paletteParams().value(pal).toMap();
            text = formatMultibyteInfo(addr, data, pal, params);
        }
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