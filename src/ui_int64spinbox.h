// Byte-0-scope - binary data visualizer
// File: ui_int64spinbox.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDoubleSpinBox>

// Спинбокс для неотрицательных 64-битных целых (смещения и размеры файлов
// больше 2 ГБ). QSpinBox ограничен int, поэтому здесь используется
// QDoubleSpinBox с 0 знаков после запятой: double точно хранит целые до 2^53
// (8 ПиБ), этого достаточно для любого файла. Отображение и разбор текста
// (десятичный и шестнадцатеричный вид с префиксом 0x) реализованы вручную.
//
// Интерфейс: *64-методы вместо value()/setValue()/setRange(), сигнал
// value64Changed(qint64). Стандартные double-методы базового класса не
// используются снаружи.
class Int64SpinBox : public QDoubleSpinBox {
    Q_OBJECT

public:
    explicit Int64SpinBox(QWidget *parent = nullptr);

    // Наибольшее значение, которое double хранит без потери точности (2^53).
    static qint64 maxAllowed() { return qint64(1) << 53; }

    qint64 value64() const;
    qint64 maximum64() const;

    void setValue64(qint64 v);
    void setRange64(qint64 minimum, qint64 maximum);

    void setHexMode(bool hex);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void value64Changed(qint64 value);

protected:
    QString textFromValue(double value) const override;
    double valueFromText(const QString &text) const override;
    QValidator::State validate(QString &input, int &pos) const override;

private:
    bool m_hex = false;
};
