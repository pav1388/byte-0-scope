// Byte-0-scope - binary data visualizer
// File: ui_byteconverter.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_byteconverter.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>

namespace {

QString hexText(quint32 v)
{
    return QStringLiteral("%1").arg(uint(v), 8, 16, QLatin1Char('0')).toUpper();
}

} // namespace

ByteConverterWidget::ByteConverterWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    setValue(0);
}

void ByteConverterWidget::buildUi()
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(3);

    auto addLabel = [&](int col, const QString &text) {
        auto *lbl = new QLabel(text, this);
        lbl->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
        grid->addWidget(lbl, 0, col);
    };

    auto addEdit = [&](int col, QLineEdit *&edit,
                       const QString &regex, int maxLen, int minWidth) {
        edit = new QLineEdit(this);
        edit->setValidator(new QRegularExpressionValidator(
            QRegularExpression(regex), edit));
        edit->setAlignment(Qt::AlignRight);
        edit->setMaxLength(maxLen);
        edit->setMinimumWidth(minWidth);
        edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        grid->addWidget(edit, 1, col);
        connect(edit, &QLineEdit::textChanged, this,
                [this, edit]() { updateFromSource(edit); });
    };

    addLabel(0, tr("Dec:"));
    addLabel(1, tr("Hex:"));

    addEdit(0, m_decEdit,
            QStringLiteral("[0-9]{0,10}"), 10, 90);
    addEdit(1, m_hexEdit,
            QStringLiteral("[0-9A-Fa-f]{0,8}"), 8, 80);

    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void ByteConverterWidget::showValue(const QLineEdit *skip)
{
    if (skip != m_decEdit) {
        QSignalBlocker b(m_decEdit);
        m_decEdit->setText(QString::number(m_value));
    }
    if (skip != m_hexEdit) {
        QSignalBlocker b(m_hexEdit);
        m_hexEdit->setText(hexText(m_value));
    }
}

void ByteConverterWidget::setValue(quint32 v)
{
    const bool changed = (v != m_value);
    m_value = v;
    showValue(nullptr);
    if (changed)
        emit valueChanged(m_value);
}

void ByteConverterWidget::updateFromSource(QLineEdit *source)
{
    const bool isDec = (source == m_decEdit);

    const QString text = source->text();
    if (text.isEmpty()) {
        QLineEdit *other = isDec ? m_hexEdit : m_decEdit;
        QSignalBlocker b(other);
        other->clear();
        return;
    }

    bool ok = false;
    const qulonglong parsed = text.toULongLong(&ok, isDec ? 10 : 16);
    if (!ok || parsed > 0xFFFFFFFFull)
        return;

    const bool changed = (quint32(parsed) != m_value);
    m_value = quint32(parsed);
    showValue(source);
    if (changed)
        emit valueChanged(m_value);
}
