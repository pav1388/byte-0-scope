// Byte-0-scope - binary data visualizer
// File: app_thememanager.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app_thememanager.h"
#include "qt_compat.h"

#include <QApplication>
#include <QPalette>
#include <QWidget>

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
    padding: 2px 4px;
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
QScrollBar:vertical {
    background: #2b2b2b;
    width: 16px;
    margin: 0;
    border: none;
}
QScrollBar:horizontal {
    background: #2b2b2b;
    height: 16px;
    margin: 0;
    border: none;
}
QScrollBar::handle:vertical {
    background: #555;
    min-height: 28px;
    margin: 2px;
    border-radius: 5px;
}
QScrollBar::handle:horizontal {
    background: #555;
    min-width: 28px;
    margin: 2px;
    border-radius: 5px;
}
QScrollBar::handle:hover   { background: #6a6a6a; }
QScrollBar::handle:pressed { background: #7a7a7a; }
/* Arrow buttons are unused: zero size, otherwise light stubs remain. */
QScrollBar::add-line:vertical {
    height: 0px; subcontrol-position: bottom; subcontrol-origin: margin;
}
QScrollBar::sub-line:vertical {
    height: 0px; subcontrol-position: top; subcontrol-origin: margin;
}
QScrollBar::add-line:horizontal {
    width: 0px; subcontrol-position: right; subcontrol-origin: margin;
}
QScrollBar::sub-line:horizontal {
    width: 0px; subcontrol-position: left; subcontrol-origin: margin;
}
QScrollBar::add-page, QScrollBar::sub-page { background: none; }
/* Square where the horizontal and vertical scrollbars meet. */
QAbstractScrollArea::corner { background: #2b2b2b; border: none; }
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

void repaintAll(QApplication *app)
{
    // allWidgets() уже содержит и дочерние виджеты.
    const QWidgetList all = app->allWidgets();
    for (QWidget *w : all)
        w->update();
}

} // namespace

ThemeManager::ThemeManager()
{
    m_current = build(m_isDark);
}

void ThemeManager::toggle()
{
    setDark(!m_isDark);
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
    t.colors = dark ? kDarkPal : kLightPal;

    if (dark) {
        t.styles["byte_info"] =
            "background:#2b2b2b;border:1px solid #555;padding:9px;"
            "color:#dcdcdc;";
        t.styles["info_label"] = "font-size:8pt;color:#dcdcdc;"
                                 "background-color:transparent;";
    } else {
        t.styles["byte_info"] =
            "background:#ffffff;border:1px solid #aaa;padding:9px;"
            "color:#000000;";
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

    app->setStyleSheet(m_isDark ? QString::fromLatin1(kDarkQss) : QString());

    static const struct { QPalette::ColorRole role; const char *key; } kRoles[] = {
        { QPalette::Window,          "window" },
        { QPalette::WindowText,      "window_text" },
        { QPalette::Base,            "base" },
        { QPalette::AlternateBase,   "alternate_base" },
        { QPalette::Text,            "text" },
        { QPalette::Button,          "button" },
        { QPalette::ButtonText,      "button_text" },
        { QPalette::Highlight,       "highlight" },
        { QPalette::HighlightedText, "highlighted_text" },
    };

    QPalette p;
    for (const auto &r : kRoles)
        p.setColor(r.role, m_current.colors.value(QLatin1String(r.key)));
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Shadow,     QColor(0, 0, 0));

    if (m_isDark) {
        p.setColor(QPalette::ToolTipBase,     QColor(43, 43, 43));
        p.setColor(QPalette::ToolTipText,     QColor(220, 220, 220));
        p.setColor(QPalette::Link,            QColor(42, 130, 218));

        p.setColor(QPalette::Light,           QColor(86, 86, 86));
        p.setColor(QPalette::Midlight,        QColor(64, 64, 64));
        p.setColor(QPalette::Mid,             QColor(52, 52, 52));
        p.setColor(QPalette::Dark,            QColor(28, 28, 28));
        QtCompat::setPlaceholderTextColor(p,  QColor(140, 140, 140));

        p.setColor(QPalette::Disabled, QPalette::WindowText,      QColor(120, 120, 120));
        p.setColor(QPalette::Disabled, QPalette::Text,            QColor(120, 120, 120));
        p.setColor(QPalette::Disabled, QPalette::ButtonText,      QColor(120, 120, 120));
        p.setColor(QPalette::Disabled, QPalette::Highlight,       QColor(80, 80, 80));
        p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(160, 160, 160));
    } else {
        p.setColor(QPalette::ToolTipBase,     QColor(255, 255, 220));
        p.setColor(QPalette::ToolTipText,     QColor(0, 0, 0));
        p.setColor(QPalette::Link,            QColor(0, 0, 255));
        p.setColor(QPalette::Light,           QColor(255, 255, 255));
        p.setColor(QPalette::Midlight,        QColor(227, 227, 227));
        p.setColor(QPalette::Mid,             QColor(160, 160, 160));
        p.setColor(QPalette::Dark,            QColor(128, 128, 128));
    }
    app->setPalette(p);

    repaintAll(app);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}
