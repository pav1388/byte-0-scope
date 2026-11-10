// Byte-0-scope - binary data visualizer
// File: ui_aboutdialog.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_aboutdialog.h"
#include "definitions.h"

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

// --- Настройки модуля AboutDialog (префикс AD_) ---

#define AD_DIALOG_WIDTH         640
#define AD_DIALOG_HEIGHT        480
#define AD_HEADER_ICON_SIZE     64

#define AD_HOMEPAGE_URL         "https://github.com/pav1388/byte-0-scope"

static const char *kAboutTitle = QT_TRANSLATE_NOOP("AboutDialog",
    "Byte-0-scope v%1 by pav13 (2026)");

static const char *kAboutSubtitle = QT_TRANSLATE_NOOP("AboutDialog",
    "Interactive binary data visualizer.\n"
    "License: GPL-3.0-or-later");

static const char *kAboutFeatures = QT_TRANSLATE_NOOP("AboutDialog",
    "Key features:\n"
    "  * Visualization of files of any size (KB – GB)\n"
    "  * Palettes for 1/2/3/4/8/16/24/32/64 bits per pixel\n"
    "  * Multi-byte formats: RGB, RGBA, ABGR, RGB565/555, integers, float\n"
    "  * Zooming and panning, panorama overview\n"
    "  * Hex view (bytes + ASCII) with configurable layout\n"
    "  * Info panel with byte and pixel details\n"
    "  * Optional byte tooltip that follows the cursor\n"
    "  * Dec / Hex byte converter\n"
    "  * Debug info panel\n"
    "  * PNG export (visible area / 1:1 / current scale)\n"
    "  * Compare two files via XOR\n"
    "  * Save/load display settings to .b0s next to the file\n"
    "  * Dark and light themes\n"
    "  * Horizontal and vertical mirroring\n"
    "  * Configurable tile size and cache limit\n"
    "  * Built-in tutorial demo (\"Training\" mode)");

static const char *kAboutControls = QT_TRANSLATE_NOOP("AboutDialog",
    "Controls:\n"
    "  * Left-click a pixel — lock the byte\n"
    "  * Right-click + drag — pan\n"
    "  * Wheel — zoom\n"
    "  * Ctrl + wheel — row width (switches to manual width mode)\n"
    "  * Left-click in the hex view — select a byte (synced with canvas)\n"
    "  * Drag the top edge of the hex view — resize it\n"
    "  * \"|+|\" / \"+|\" buttons — pick region start / end by click\n"
    "  * Ctrl+O — open file\n"
    "  * Ctrl+S — save .b0s next to the file\n"
    "  * Ctrl+L — load .b0s\n"
    "  * Ctrl+R — reset display settings\n"
    "  * Ctrl+E — export to PNG (full image, 1:1)\n"
    "  * Ctrl+X — compare two files (XOR)\n"
    "  * Ctrl+W — close current file\n"
    "  * Drag & drop — open a file (two files -> XOR comparison)");

static const char *kAboutFormats = QT_TRANSLATE_NOOP("AboutDialog",
    "Formats and settings:\n"
    "  * byte0scope-settings.ini — global settings (theme, language,\n"
    "    window geometry, hex-view state, performance,\n"
    "    recent files, default display parameters). Stored next\n"
    "    to the executable; if writing is not possible —\n"
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
    "  * Tile size 256–2048 px, cache limit 180–1024 MB\n"
    "    (up to 400 MB on 32-bit builds)");

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

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    if (auto *b = bb->button(QDialogButtonBox::Close))
        b->setText(QCoreApplication::translate("AboutDialog", "Close"));
    QObject::connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(bb);

    dlg.exec();
}