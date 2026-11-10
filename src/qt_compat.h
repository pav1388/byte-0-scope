// Byte-0-scope - binary data visualizer
// File: qt_compat.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Тонкий слой совместимости: один и тот же код собирается и со статическим
// Qt 5.6.3 (профиль xp32), и с Qt 5.15 (профиль win64).
// Правило: любая #if QT_VERSION, нужная только ради устаревшего API, живёт
// здесь, а не в прикладном коде.

#include <QByteArray>
#include <QImage>
#include <QPoint>
#include <QRegion>
#include <QVector>
#include <QWheelEvent>
#include <QtGlobal>

#include <limits>

namespace QtCompat {

// Обход прямоугольников региона. QRegion::rects() объявлен устаревшим
// в Qt 5.15 (заменён на begin()/end()); сам begin()/end() появился
// только в Qt 5.8, поэтому 5.6.x ещё требует rects().
template <typename Func>
inline void forEachRegionRect(const QRegion &region, Func func)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    for (const QRect &r : region)
        func(r);
#else
    const QVector<QRect> rects = region.rects();
    for (const QRect &r : rects)
        func(r);
#endif
}

// Максимальный размер QByteArray в байтах.
// В Qt 5 размер QByteArray - int (QByteArray::maxSize() в Qt 5.x нет,
// появился только в Qt 6.8).
inline qint64 byteArrayMaxSize()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    return qint64(QByteArray::maxSize());
#else
    return qint64(std::numeric_limits<int>::max());
#endif
}

// Размер данных QImage в байтах.
// QImage::byteCount() (int) объявлен устаревшим в Qt 5.10 и удалён в Qt 6.
inline qint64 imageBytes(const QImage &image)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    return qint64(image.sizeInBytes());
#else
    return qint64(image.byteCount());
#endif
}

// Позиция курсора в событии колеса мыши (в координатах виджета).
// QWheelEvent::pos() объявлен устаревшим в Qt 5.14/5.15.
inline QPoint wheelPos(const QWheelEvent *e)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return e->position().toPoint();
#else
    return e->pos();
#endif
}

// true, если процесс 32-битный (адресное пространство 2-4 ГБ).
inline bool is32BitProcess()
{
    return sizeof(void *) <= 4;
}

} // namespace QtCompat
