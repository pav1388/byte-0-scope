// Byte-0-scope - binary data visualizer
// File: aboutdialog.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aboutdialog.h"

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

static const char *kAboutTitle =
    "Байт-0-скоп (Byte-0-scope) v" APP_VERSION " by pav13";

static const char *kAboutSubtitle =
    "Интерактивный визуализатор бинарных данных.\n"
    "Лицензия: GPL-3.0-or-later";

static const char *kAboutFeatures =
    "Основные возможности:\n"
    "  * Визуализация файлов любого размера (КБ – ГБ)\n"
    "  * Палитры для 1/2/3/4/8/16/24/32/64 бит на пиксель\n"
    "  * Многобайтовые форматы: RGB, RGBA, ABGR, RGB565/555, целые, float\n"
    "  * Масштабирование и панорамирование, панорамный обзор\n"
    "  * Информационная панель с детализацией байта и пикселя\n"
    "  * Экспорт в PNG\n"
    "  * Сравнение двух файлов через XOR\n"
    "  * Сохранение/загрузка настроек отображение в .b0s рядом с файлом\n"
    "  * Тёмная и светлая темы\n"
    "  * Зеркальное отражение по горизонтали и вертикали\n"
    "  * Настройка размера тайлов и лимита кэша";

static const char *kAboutControls =
    "Управление:\n"
    "  * ЛКМ по пикселю — закрепить байт\n"
    "  * ПКМ + перемещение — панорамирование\n"
    "  * Колесо — масштаб\n"
    "  * Ctrl + колесо — ширина строки (в ручном режиме)\n"
    "  * Ctrl+O — открыть файл\n"
    "  * Ctrl+S — сохранить .b0s рядом с файлом\n"
    "  * Ctrl+L — загрузить .b0s\n"
    "  * Ctrl+R — сбросить настройки отображения\n"
    "  * Ctrl+E — экспорт в PNG\n"
    "  * Ctrl+X — сравнить два файла (XOR)\n"
    "  * Ctrl+W — закрыть текущий файл\n"
    "  * Drag & drop — открыть файл перетаскиванием";

static const char *kAboutFormats =
    "Форматы и настройки:\n"
    "  * b0s-settings.ini — глобальные настройки (тема, геометрия окна,\n"
    "    производительность, последние файлы). Хранится рядом\n"
    "    с исполняемым файлом; если запись невозможна —\n"
    "    в пользовательском каталоге конфигурации.\n"
    "  * *.b0s — настройки отображения конкретного файла (JSON).\n"
    "    Хранится рядом с исходным файлом и подхватывается\n"
    "    автоматически при открытии.";

static const char *kAboutPerformance =
    "Производительность:\n"
    "  * Memory-mapping (mmap) для больших файлов\n"
    "  * Тайловый рендеринг только видимой области\n"
    "  * LRU-кэш тайлов с автоматической очисткой\n"
    "  * Prefetching соседних тайлов\n"
    "  * Размер тайла 128–1024 px, лимит кэша 180–2000 МБ";

static const char *kAboutAi =
    "Примечание: часть кода была сгенерирована или доработана\n"
    "с использованием ИИ.";

void showAboutDialog(QWidget *parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(QObject::tr("О программе"));
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
        QStringLiteral("<b>%1</b>").arg(QString::fromUtf8(kAboutTitle)),
        &dlg);
    titleLabel->setTextFormat(Qt::RichText);
    titleLabel->setWordWrap(true);
    headerColumn->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel(
        QString::fromUtf8(kAboutSubtitle), &dlg);
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
        QString::fromUtf8(kAboutFeatures),
        QString::fromUtf8(kAboutControls),
        QString::fromUtf8(kAboutFormats),
        QString::fromUtf8(kAboutPerformance),
        QString::fromUtf8(kAboutAi),
    }.join("\n\n");

    txt->setPlainText(aboutText);
    layout->addWidget(txt);

    // --- Кнопка «Закрыть» ---
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    if (auto *b = bb->button(QDialogButtonBox::Close))
        b->setText(QObject::tr("Закрыть"));
    QObject::connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(bb);

    dlg.exec();
}