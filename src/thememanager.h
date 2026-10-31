// Byte-0-scope - binary data visualizer
// File: thememanager.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QMap>
#include <QString>

class QApplication;

struct Theme {
    QString name;
    QMap<QString, QColor>  colors;
    QMap<QString, QString> styles;
};

class ThemeManager {
public:
    ThemeManager();

    bool         isDark()  const { return m_isDark; }
    const Theme &current() const { return m_current; }

    void toggle();
    void setDark(bool dark);
    void apply(QApplication *app = nullptr);

private:
    Theme build(bool dark) const;

    bool  m_isDark = false;
    Theme m_current;
};