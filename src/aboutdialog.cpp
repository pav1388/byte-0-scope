// Byte-0-scope - binary data visualizer
// File: aboutdialog.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aboutdialog.h"

#include <QCoreApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStringList>
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>

#ifndef APP_VERSION
#  define APP_VERSION "dev"
#endif

// --- Настройки модуля AboutDialog (префикс AD_) ---

#define AD_DIALOG_WIDTH         640
#define AD_DIALOG_HEIGHT        480
#define AD_HEADER_ICON_SIZE     64

#define AD_HOMEPAGE_URL         "https://github.com/pav1388/byte-0-scope"

static const char *kAboutTitle = QT_TRANSLATE_NOOP("AboutDialog",
    "Byte-0-scope v%1 by pav13");

static const char *kAboutSubtitle = QT_TRANSLATE_NOOP("AboutDialog",
    "Interactive binary data visualizer.\n"
    "License: GPL-3.0-or-later");

static const char *kAboutFeatures = QT_TRANSLATE_NOOP("AboutDialog",
    "Key features:\n"
    "  * Visualization of files of any size (KB – GB)\n"
    "  * Palettes for 1/2/3/4/8/16/24/32/64 bits per pixel\n"
    "  * Multi-byte formats: RGB, RGBA, ABGR, RGB565/555, integers, float\n"
    "  * Zooming and panning, panorama overview\n"
    "  * Info panel with byte and pixel details\n"
    "  * PNG export\n"
    "  * Compare two files via XOR\n"
    "  * Save/load display settings to .b0s next to the file\n"
    "  * Dark and light themes\n"
    "  * Horizontal and vertical mirroring\n"
    "  * Configurable tile size and cache limit");

static const char *kAboutControls = QT_TRANSLATE_NOOP("AboutDialog",
    "Controls:\n"
    "  * Left-click a pixel — lock the byte\n"
    "  * Right-click + drag — pan\n"
    "  * Wheel — zoom\n"
    "  * Ctrl + wheel — row width (in manual mode)\n"
    "  * Ctrl+O — open file\n"
    "  * Ctrl+S — save .b0s next to the file\n"
    "  * Ctrl+L — load .b0s\n"
    "  * Ctrl+R — reset display settings\n"
    "  * Ctrl+E — export to PNG\n"
    "  * Ctrl+X — compare two files (XOR)\n"
    "  * Ctrl+W — close current file\n"
    "  * Drag & drop — open a file by dragging");

static const char *kAboutFormats = QT_TRANSLATE_NOOP("AboutDialog",
    "Formats and settings:\n"
    "  * b0s-settings.ini — global settings (theme, window geometry,\n"
    "    performance, recent files). Stored next to the\n"
    "    executable; if writing is not possible —\n"
    "    in the user configuration directory.\n"
    "  * *.b0s — display settings for a specific file (JSON).\n"
    "    Stored next to the source file and picked up\n"
    "    automatically on open.");

static const char *kAboutPerformance = QT_TRANSLATE_NOOP("AboutDialog",
    "Performance:\n"
    "  * Memory-mapping (mmap) for large files\n"
    "  * Tile rendering of the visible region only\n"
    "  * LRU tile cache with automatic eviction\n"
    "  * Prefetching of neighboring tiles\n"
    "  * Tile size ***-**** px, cache limit ***-**** MB");

static const char *kAboutAi = QT_TRANSLATE_NOOP("AboutDialog",
    "Note: part of the code was generated or refined\n"
    "using AI.");

void showAboutDialog(QWidget *parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(
        QCoreApplication::translate("AboutDialog", "About"));
    dlg.setWindowIcon(parent ? parent->windowIcon() : QIcon());
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dlg.resize(AD_DIALOG_WIDTH, AD_DIALOG_HEIGHT);

    auto *layout = new QVBoxLayout(&dlg);

    // --- Шапка: иконка + название/лицензия + кликабельная ссылка ---
    auto *headerRow = new QHBoxLayout();

    auto *iconLabel = new QLabel(&dlg);
    const QIcon appIcon = parent ? parent->windowIcon() : QIcon();
    if (!appIcon.isNull()) {
        iconLabel->setPixmap(
            appIcon.pixmap(AD_HEADER_ICON_SIZE, AD_HEADER_ICON_SIZE));
        iconLabel->setFixedSize(AD_HEADER_ICON_SIZE, AD_HEADER_ICON_SIZE);
        iconLabel->setScaledContents(true);
    }
    headerRow->addWidget(iconLabel, 0, Qt::AlignTop);

    auto *headerColumn = new QVBoxLayout();

    auto *titleLabel = new QLabel(
        QStringLiteral("<b>%1</b>")
            .arg(QCoreApplication::translate("AboutDialog", kAboutTitle)
                     .arg(QStringLiteral(APP_VERSION))),
        &dlg);
    titleLabel->setTextFormat(Qt::RichText);
    titleLabel->setWordWrap(true);
    headerColumn->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel(
        QCoreApplication::translate("AboutDialog", kAboutSubtitle), &dlg);
    subtitleLabel->setWordWrap(true);
    headerColumn->addWidget(subtitleLabel);

    auto *linkLabel = new QLabel(&dlg);
    linkLabel->setTextFormat(Qt::RichText);
    linkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    linkLabel->setOpenExternalLinks(true);
    linkLabel->setText(QStringLiteral("<a href=\"%1\">%1</a>")
                           .arg(QStringLiteral(AD_HOMEPAGE_URL)));
    headerColumn->addWidget(linkLabel);

    headerColumn->addStretch();
    headerRow->addLayout(headerColumn, 1);

    layout->addLayout(headerRow);

    // --- Основной текст ---
    auto *txt = new QPlainTextEdit(&dlg);
    txt->setReadOnly(true);

    const QString aboutText = QStringList{
        QCoreApplication::translate("AboutDialog", kAboutFeatures),
        QCoreApplication::translate("AboutDialog", kAboutControls),
        QCoreApplication::translate("AboutDialog", kAboutFormats),
        QCoreApplication::translate("AboutDialog", kAboutPerformance),
        QCoreApplication::translate("AboutDialog", kAboutAi),
    }.join("\n\n");

    txt->setPlainText(aboutText);
    layout->addWidget(txt);

    // --- Кнопка «Закрыть» ---
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    if (auto *b = bb->button(QDialogButtonBox::Close))
        b->setText(QCoreApplication::translate("AboutDialog", "Close"));
    QObject::connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(bb);

    dlg.exec();
}