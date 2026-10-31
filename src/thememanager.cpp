// Byte-0-scope - binary data visualizer
// File: thememanager.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thememanager.h"

#include <QApplication>
#include <QMap>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QWidget>
#include <QtGlobal>

// ---------- Тёмная палитра ----------
static const QMap<QString, QColor> kDarkPal = {
    {"window",                   QColor(43, 43, 43)},
    {"window_text",              QColor(220, 220, 220)},
    {"base",                     QColor(30, 30, 30)},
    {"alternate_base",           QColor(48, 48, 48)},
    {"text",                     QColor(220, 220, 220)},
    {"button",                   QColor(74, 74, 74)},
    {"button_text",              QColor(220, 220, 220)},
    {"highlight",                QColor(42, 130, 218)},
    {"highlighted_text",         QColor(255, 255, 255)},
    {"widget_background",        QColor(43, 43, 43)},
    {"panorama_bg",              QColor(30, 30, 30)},
    {"panorama_image",           QColor(74, 74, 74)},
    {"panorama_image_border",    QColor(110, 110, 110)},
    {"panorama_viewport",        QColor(255, 255, 255, 180)},
    {"panorama_viewport_border", QColor(255, 0, 0)},
};

// ---------- Светлая палитра ----------
static const QMap<QString, QColor> kLightPal = {
    {"window",                   QColor(239, 239, 239)},
    {"window_text",              QColor(0, 0, 0)},
    {"base",                     QColor(255, 255, 255)},
    {"alternate_base",           QColor(247, 247, 247)},
    {"text",                     QColor(0, 0, 0)},
    {"button",                   QColor(239, 239, 239)},
    {"button_text",              QColor(0, 0, 0)},
    {"highlight",                QColor(48, 140, 198)},
    {"highlighted_text",         QColor(255, 255, 255)},
    {"widget_background",        QColor(239, 239, 239)},
    {"panorama_bg",              QColor(220, 220, 220)},
    {"panorama_image",           QColor(180, 180, 180)},
    {"panorama_image_border",    QColor(150, 150, 150)},
    {"panorama_viewport",        QColor(0, 0, 0, 120)},
    {"panorama_viewport_border", QColor(0, 0, 255)},
};

namespace {

const char *kDarkQss = R"(
QWidget {
    background-color: #2b2b2b;
    color: #dcdcdc;
}
QComboBox, QLineEdit, QSpinBox, QCheckBox, QGroupBox, QToolTip,
QPlainTextEdit, QTextEdit, QAbstractSpinBox {
    background-color: #2b2b2b;
    color: #dcdcdc;
    border: 1px solid #555;
    border-radius: 3px;
    padding: 2px;
}
QComboBox::drop-down { border: 0px; width: 18px; }
QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #dcdcdc;
    width: 0px; height: 0px;
    margin-right: 4px;
}
QComboBox QAbstractItemView {
    background-color: #2b2b2b;
    color: #dcdcdc;
    selection-background-color: #3c7fb1;
    selection-color: #ffffff;
    border: 1px solid #555;
}
QSpinBox::up-button, QSpinBox::down-button,
QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {
    background-color: #3a3a3a;
    border: 1px solid #555;
    width: 14px;
}
QSpinBox::up-arrow, QAbstractSpinBox::up-arrow {
    image: none;
    border-left: 3px solid transparent;
    border-right: 3px solid transparent;
    border-bottom: 4px solid #dcdcdc;
    width: 0px; height: 0px;
}
QSpinBox::down-arrow, QAbstractSpinBox::down-arrow {
    image: none;
    border-left: 3px solid transparent;
    border-right: 3px solid transparent;
    border-top: 4px solid #dcdcdc;
    width: 0px; height: 0px;
}
QPushButton {
    background-color: #3a3a3a;
    color: #dcdcdc;
    border: 1px solid #555;
    border-radius: 3px;
    padding: 4px 8px;
}
QPushButton:hover   { background-color: #4a4a4a; }
QPushButton:pressed { background-color: #2a2a2a; }
QPushButton:checked {
    background-color: #3c7fb1;
    color: #ffffff;
    border: 1px solid #5aa0d8;
}
QPushButton:checked:hover { background-color: #4a90c5; }
QPushButton:disabled {
    background-color: #2f2f2f;
    color: #777;
    border-color: #444;
}
QGroupBox {
    border: 1px solid #555;
    border-radius: 3px;
    margin-top: 8px;
    padding-top: 8px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 3px 0 3px;
    color: #dcdcdc;
}
QCheckBox::indicator {
    width: 14px; height: 14px;
    border: 1px solid #555;
    border-radius: 3px;
    background-color: #2b2b2b;
}
QCheckBox::indicator:checked {
    background-color: #3c7fb1;
    border-color: #5aa0d8;
}
QMenuBar { background-color: #2b2b2b; color: #dcdcdc; }
QMenuBar::item:selected { background-color: #3c7fb1; color: #ffffff; }
QMenu { background-color: #2b2b2b; color: #dcdcdc; border: 1px solid #555; }
QMenu::item:selected { background-color: #3c7fb1; color: #ffffff; }
QStatusBar { background-color: #2b2b2b; color: #dcdcdc; }
QScrollBar:vertical, QScrollBar:horizontal {
    background: #2b2b2b;
    border: 1px solid #444;
}
QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background: #555;
    border-radius: 3px;
    min-height: 20px; min-width: 20px;
}
QScrollBar::handle:hover { background: #666; }
QScrollBar::add-line, QScrollBar::sub-line { background: #2b2b2b; }
QSlider::groove:horizontal {
    background: #444; height: 6px; border-radius: 3px;
}
QSlider::handle:horizontal {
    background: #3c7fb1; border: 1px solid #5aa0d8;
    width: 14px; margin: -5px 0; border-radius: 7px;
}
QTabWidget::pane { border: 1px solid #555; }
QTabBar::tab {
    background: #3a3a3a; color: #dcdcdc;
    padding: 4px 10px; border: 1px solid #555;
    border-bottom: 0;
}
QTabBar::tab:selected { background: #3c7fb1; color: #ffffff; }
)";

const char *kLightQss = "";

void repaintAll(QApplication *app)
{
    const QWidgetList all = app->allWidgets();
    for (QWidget *w : all) {
        if (!w)
            continue;
        w->update();
        const QList<QWidget *> kids = w->findChildren<QWidget *>();
        for (QWidget *k : kids)
            if (k) k->update();
    }
}

} // namespace

ThemeManager::ThemeManager()
{
    m_current = build(m_isDark);
}

void ThemeManager::toggle()
{
    m_isDark = !m_isDark;
    m_current = build(m_isDark);
}

void ThemeManager::setDark(bool dark)
{
    if (m_isDark == dark)
        return;
    m_isDark = dark;
    m_current = build(dark);
}

Theme ThemeManager::build(bool dark) const
{
    Theme t;
    t.name   = dark ? "dark" : "light";
    t.colors = dark ? kDarkPal : kLightPal;

    if (dark) {
        t.styles["byte_info"] =
            "background:#2b2b2b;border:1px solid #555;padding:9px;"
            "color:#dcdcdc;";
        t.styles["hint"]       = "font-size:9pt;";
        t.styles["info_label"] = "font-size:8pt;color:#dcdcdc;"
                                 "background-color:transparent;";
    } else {
        t.styles["byte_info"] =
            "background:#ffffff;border:1px solid #aaa;padding:9px;"
            "color:#000000;";
        t.styles["hint"]       = "font-size:9pt;";
        t.styles["info_label"] = "font-size:8pt;background-color:transparent;";
    }
    return t;
}

void ThemeManager::apply(QApplication *app)
{
    if (!app)
        app = qApp;
    if (!app)
        return;

    if (m_isDark) {
        app->setStyleSheet(QString::fromLatin1(kDarkQss));
    } else {
        app->setStyleSheet(QString::fromLatin1(kLightQss));
    }

    QPalette p;
    if (m_isDark) {
        p.setColor(QPalette::Window,          m_current.colors.value("window"));
        p.setColor(QPalette::WindowText,      m_current.colors.value("window_text"));
        p.setColor(QPalette::Base,            m_current.colors.value("base"));
        p.setColor(QPalette::AlternateBase,   m_current.colors.value("alternate_base"));
        p.setColor(QPalette::Text,            m_current.colors.value("text"));
        p.setColor(QPalette::Button,          m_current.colors.value("button"));
        p.setColor(QPalette::ButtonText,      m_current.colors.value("button_text"));
        p.setColor(QPalette::Highlight,       m_current.colors.value("highlight"));
        p.setColor(QPalette::HighlightedText, m_current.colors.value("highlighted_text"));

        p.setColor(QPalette::ToolTipBase,     QColor(43, 43, 43));
        p.setColor(QPalette::ToolTipText,     QColor(220, 220, 220));
        p.setColor(QPalette::BrightText,      Qt::red);
        p.setColor(QPalette::Link,            QColor(42, 130, 218));

        p.setColor(QPalette::Light,           QColor(86, 86, 86));
        p.setColor(QPalette::Midlight,        QColor(64, 64, 64));
        p.setColor(QPalette::Mid,             QColor(52, 52, 52));
        p.setColor(QPalette::Dark,            QColor(28, 28, 28));
        p.setColor(QPalette::Shadow,          QColor(0, 0, 0));
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        p.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));
#endif

        p.setColor(QPalette::Disabled, QPalette::WindowText,      QColor(120, 120, 120));
        p.setColor(QPalette::Disabled, QPalette::Text,            QColor(120, 120, 120));
        p.setColor(QPalette::Disabled, QPalette::ButtonText,      QColor(120, 120, 120));
        p.setColor(QPalette::Disabled, QPalette::Highlight,       QColor(80, 80, 80));
        p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(160, 160, 160));
    } else {
        p = QPalette();
        p.setColor(QPalette::Window,          m_current.colors.value("window"));
        p.setColor(QPalette::WindowText,      m_current.colors.value("window_text"));
        p.setColor(QPalette::Base,            m_current.colors.value("base"));
        p.setColor(QPalette::AlternateBase,   m_current.colors.value("alternate_base"));
        p.setColor(QPalette::Text,            m_current.colors.value("text"));
        p.setColor(QPalette::Button,          m_current.colors.value("button"));
        p.setColor(QPalette::ButtonText,      m_current.colors.value("button_text"));
        p.setColor(QPalette::Highlight,       m_current.colors.value("highlight"));
        p.setColor(QPalette::HighlightedText, m_current.colors.value("highlighted_text"));
        p.setColor(QPalette::ToolTipBase,     QColor(255, 255, 220));
        p.setColor(QPalette::ToolTipText,     QColor(0, 0, 0));
        p.setColor(QPalette::BrightText,      Qt::red);
        p.setColor(QPalette::Link,            QColor(0, 0, 255));
        p.setColor(QPalette::Light,           QColor(255, 255, 255));
        p.setColor(QPalette::Midlight,        QColor(227, 227, 227));
        p.setColor(QPalette::Mid,             QColor(160, 160, 160));
        p.setColor(QPalette::Dark,            QColor(128, 128, 128));
        p.setColor(QPalette::Shadow,          QColor(0, 0, 0));
    }
    app->setPalette(p);

    repaintAll(app);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}