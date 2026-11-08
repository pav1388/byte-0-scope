// Byte-0-scope - binary data visualizer
// File: ui_byteconverter.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>
#include <QtGlobal>

class QLineEdit;

// Компактный конвертер: Dec / Hex с взаимной синхронизацией,
// диапазон значений 0 .. 0xFFFFFFFF.
class ByteConverterWidget : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(ByteConverterWidget)

public:
    explicit ByteConverterWidget(QWidget *parent = nullptr);

    quint32 value() const { return m_value; }

public slots:
    void setValue(quint32 v);

signals:
    void valueChanged(quint32 value);

private:
    void buildUi();
    void updateFromSource(QLineEdit *source);

    QLineEdit *m_decEdit = nullptr;
    QLineEdit *m_hexEdit = nullptr;

    quint32 m_value    = 0;
    bool    m_updating = false;
};