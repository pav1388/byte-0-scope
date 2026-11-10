// Byte-0-scope - binary data visualizer
// File: ui_int64spinbox.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_int64spinbox.h"

#include <QFontMetrics>
#include <QLineEdit>
#include <QtGlobal>

// Максимальная длина вводимого числа (цифр без префикса). Для 2^53 хватает
// 16 десятичных / 14 шестнадцатеричных цифр.
#define I64_MAX_DIGITS          16
// Ширина поля рассчитывается на столько символов (как у прежнего QSpinBox
// с диапазоном до 0x7FFFFFFF), длинные значения прокручиваются в поле.
#define I64_SIZEHINT_CHARS      10

namespace {

bool isAsciiDigit(QChar c, bool hex)
{
    const ushort u = c.unicode();
    if (u >= '0' && u <= '9')
        return true;
    return hex && ((u >= 'a' && u <= 'f') || (u >= 'A' && u <= 'F'));
}

// Убирает пробелы и необязательный префикс 0x. Префикс переключает разбор
// в шестнадцатеричный режим независимо от режима поля.
// Возвращает true, если префикс был.
bool splitPrefix(const QString &text, QString *body)
{
    *body = text.trimmed();
    if (body->startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
        *body = body->mid(2);
        return true;
    }
    return false;
}

} // namespace

Int64SpinBox::Int64SpinBox(QWidget *parent)
    : QDoubleSpinBox(parent)
{
    setDecimals(0);
    setSingleStep(1.0);
    QDoubleSpinBox::setRange(0.0, double(maxAllowed()));

    connect(this,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, [this](double v) { emit value64Changed(qint64(qRound64(v))); });
}

qint64 Int64SpinBox::value64() const
{
    return qint64(qRound64(QDoubleSpinBox::value()));
}

qint64 Int64SpinBox::minimum64() const
{
    return qint64(qRound64(QDoubleSpinBox::minimum()));
}

qint64 Int64SpinBox::maximum64() const
{
    return qint64(qRound64(QDoubleSpinBox::maximum()));
}

void Int64SpinBox::setValue64(qint64 v)
{
    QDoubleSpinBox::setValue(double(qBound<qint64>(minimum64(), v, maximum64())));
}

void Int64SpinBox::setRange64(qint64 minimum, qint64 maximum)
{
    const qint64 lo = qBound<qint64>(0, minimum, maxAllowed());
    const qint64 hi = qBound<qint64>(lo, maximum, maxAllowed());
    QDoubleSpinBox::setRange(double(lo), double(hi));
}

void Int64SpinBox::setHexMode(bool hex)
{
    if (m_hex == hex)
        return;
    m_hex = hex;
    // Принудительно перерисовать текст в новом формате (значение не меняется,
    // поэтому valueChanged не испускается).
    if (QLineEdit *edit = lineEdit())
        edit->setText(textFromValue(QDoubleSpinBox::value()));
}

QString Int64SpinBox::textFromValue(double value) const
{
    const qint64 n = qMax<qint64>(0, qint64(qRound64(value)));
    if (m_hex)
        return QStringLiteral("0x") + QString::number(n, 16).toUpper();
    return QString::number(n);
}

double Int64SpinBox::valueFromText(const QString &text) const
{
    QString body;
    const bool hex = splitPrefix(text, &body) || m_hex;
    bool ok = false;
    const qint64 n = body.toLongLong(&ok, hex ? 16 : 10);
    if (!ok || n < 0)
        return QDoubleSpinBox::value();     // непонятный ввод: оставить как есть
    return double(n);                       // диапазон ограничит базовый класс
}

QValidator::State Int64SpinBox::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);

    QString body;
    const bool hex = splitPrefix(input, &body) || m_hex;

    if (body.isEmpty())
        return QValidator::Intermediate;
    if (body.size() > I64_MAX_DIGITS)
        return QValidator::Invalid;
    for (int i = 0; i < body.size(); ++i) {
        if (!isAsciiDigit(body.at(i), hex))
            return QValidator::Invalid;
    }

    bool ok = false;
    const qint64 n = body.toLongLong(&ok, hex ? 16 : 10);
    if (!ok)
        return QValidator::Invalid;
    // Больше максимума: пока "промежуточное" - при завершении ввода значение
    // будет ограничено диапазоном.
    return (n > maximum64()) ? QValidator::Intermediate : QValidator::Acceptable;
}

// Базовый sizeHint рассчитан на самый длинный текст (до 2^53 = 16 символов),
// что раздвинуло бы панель управления. Сужаем до прежних ~10 символов.
static QSize narrowed(QSize s, const QFont &font)
{
    const QFontMetrics fm(font);
    const int full   = fm.boundingRect(QString(I64_MAX_DIGITS, QLatin1Char('0'))).width();
    const int wanted = fm.boundingRect(QString(I64_SIZEHINT_CHARS, QLatin1Char('0'))).width();
    s.setWidth(qMax(wanted, s.width() - qMax(0, full - wanted)));
    return s;
}

QSize Int64SpinBox::sizeHint() const
{
    return narrowed(QDoubleSpinBox::sizeHint(), font());
}

QSize Int64SpinBox::minimumSizeHint() const
{
    return narrowed(QDoubleSpinBox::minimumSizeHint(), font());
}
