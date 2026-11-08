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

constexpr quint32 kMaxValue = 0xFFFFFFFFu;

QRegularExpressionValidator *makeValidator(const QString &pattern,
                                           QObject *parent)
{
    return new QRegularExpressionValidator(
        QRegularExpression(pattern), parent);
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
        edit->setValidator(makeValidator(regex, edit));
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

void ByteConverterWidget::setValue(quint32 v)
{
    const bool changed = (v != m_value);
    m_value = v;

    {
        QSignalBlocker b1(m_decEdit), b2(m_hexEdit);
        m_decEdit->setText(QString::number(v));
        m_hexEdit->setText(QStringLiteral("%1")
                               .arg(uint(v), 8, 16, QLatin1Char('0'))
                               .toUpper());
    }

    if (changed)
        emit valueChanged(m_value);
}

void ByteConverterWidget::updateFromSource(QLineEdit *source)
{
    if (m_updating)
        return;

    const QString text = source->text();

    if (text.isEmpty()) {
        m_updating = true;
        if (source != m_decEdit) {
            QSignalBlocker b(m_decEdit);
            m_decEdit->clear();
        }
        if (source != m_hexEdit) {
            QSignalBlocker b(m_hexEdit);
            m_hexEdit->clear();
        }
        m_updating = false;
        return;
    }

    m_updating = true;

    bool parsedOk = false;
    quint32 parsed = 0;

    if (source == m_decEdit) {
        bool ok = false;
        const qulonglong v = text.toULongLong(&ok, 10);
        if (ok && v <= qulonglong(kMaxValue)) {
            parsed = quint32(v);
            parsedOk = true;
        }
    } else if (source == m_hexEdit) {
        bool ok = false;
        const qulonglong v = text.toULongLong(&ok, 16);
        if (ok && v <= qulonglong(kMaxValue)) {
            parsed = quint32(v);
            parsedOk = true;
        }
    }

    if (!parsedOk) {
        m_updating = false;
        return;
    }

    const quint32 v = parsed;
    const bool changed = (v != m_value);
    m_value = v;

    if (source != m_decEdit) {
        QSignalBlocker b(m_decEdit);
        m_decEdit->setText(QString::number(v));
    }
    if (source != m_hexEdit) {
        QSignalBlocker b(m_hexEdit);
        m_hexEdit->setText(QStringLiteral("%1")
                               .arg(uint(v), 8, 16, QLatin1Char('0'))
                               .toUpper());
    }

    m_updating = false;

    if (changed)
        emit valueChanged(v);
}