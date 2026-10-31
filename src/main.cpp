// Byte-0-scope - binary data visualizer
// File: main.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QStyleFactory>
#include <QFont>
#include <QIcon>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>

#ifdef QT_STATIC
#  include <QtPlugin>
#  ifdef Q_OS_WIN
   Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#  endif
#  ifndef BS_DROP_QICO
   Q_IMPORT_PLUGIN(QICOPlugin)
#  endif
#endif

#include "mainwindow.h"

// --- Настройки приложения (префикс MAIN_) ---
#define MAIN_APP_STYLE          "Fusion"
#define MAIN_APP_FONT_PT        9
#define MAIN_APP_NAME           "byte-0-scope"
#define MAIN_APP_ORG            "pav13"
#define MAIN_APP_ICON_PATH      ":/icon.ico"
#define MAIN_APP_CMD_DELAY_MS   0

// --- Локализация ---
#define MAIN_SETTINGS_FILE      "byte0scope-settings.ini"
#define MAIN_TRANSLATIONS_DIR   "translations"
#define MAIN_TR_APP_PREFIX      "byte0scope_"
#define MAIN_TR_QT_PREFIX       "qtbase_"
#define MAIN_TR_RES_PREFIX      ":/translations/"
#define MAIN_LANG_DEFAULT       "en"

#ifndef APP_VERSION
#  define APP_VERSION           "er-dev"
#endif

static QString settingsFilePath()
{
    QString iniPath = QDir(QCoreApplication::applicationDirPath())
                          .filePath(QStringLiteral(MAIN_SETTINGS_FILE));
    QFileInfo fi(iniPath);
    const bool writable = fi.exists() ? fi.isWritable()
                                      : QFileInfo(QCoreApplication::applicationDirPath()).isWritable();
    if (!writable) {
        const QString fallbackDir =
            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(fallbackDir);
        iniPath = fallbackDir + QLatin1Char('/') + QStringLiteral(MAIN_SETTINGS_FILE);
    }
    return iniPath;
}

// Пытается загрузить <prefix><lang> из каталога рядом с exe, затем из ресурсов.
// Возвращает установленный QTranslator или nullptr.
static QTranslator *loadTranslator(QApplication &app,
                                   const QString &prefix,
                                   const QString &lang,
                                   const QString &externalDir)
{
    const QStringList candidates = {
        prefix + lang,
        prefix + lang.left(2)   // откат "ru_RU" -> "ru"
    };

    for (const QString &name : candidates) {
        // 1) внешний файл рядом с exe
        if (!externalDir.isEmpty()) {
            QTranslator *tr = new QTranslator(&app);
            if (tr->load(name, externalDir)) {
                app.installTranslator(tr);
                return tr;
            }
            delete tr;
        }
        // 2) системный каталог переводов Qt (для qtbase_*)
        QTranslator *tr = new QTranslator(&app);
        if (tr->load(name, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
            app.installTranslator(tr);
            return tr;
        }
        delete tr;
        // 3) встроенный ресурс
        tr = new QTranslator(&app);
        if (tr->load(QStringLiteral(MAIN_TR_RES_PREFIX) + name)) {
            app.installTranslator(tr);
            return tr;
        }
        delete tr;
    }
    return nullptr;
}

static void installTranslators(QApplication &app)
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);

    QString language;
    if (settings.contains(QStringLiteral("language")))
        language = settings.value(QStringLiteral("language")).toString();
    if (language.isEmpty())
        language = QStringLiteral(MAIN_LANG_DEFAULT);

    if (language == QLatin1String(MAIN_LANG_DEFAULT))
        return;

    const QString appTrDir = QDir(QCoreApplication::applicationDirPath())
                                 .filePath(QStringLiteral(MAIN_TRANSLATIONS_DIR));

    loadTranslator(app, QStringLiteral(MAIN_TR_QT_PREFIX),  language, appTrDir);
    loadTranslator(app, QStringLiteral(MAIN_TR_APP_PREFIX), language, appTrDir);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle(QStyleFactory::create(QStringLiteral(MAIN_APP_STYLE)));

    QFont f = app.font();
    f.setPointSize(MAIN_APP_FONT_PT);
    app.setFont(f);

    app.setApplicationName(QStringLiteral(MAIN_APP_NAME));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));
    app.setOrganizationName(QStringLiteral(MAIN_APP_ORG));

    Q_INIT_RESOURCE(resources);
    app.setWindowIcon(QIcon(QStringLiteral(MAIN_APP_ICON_PATH)));

    installTranslators(app);

    MainWindow w;
    w.show();

    const QStringList args = QCoreApplication::arguments();
    if (args.size() > 1) {
        const QString path = args.at(1);
        if (!path.startsWith('-')) {
            QTimer::singleShot(MAIN_APP_CMD_DELAY_MS, &w, [&w, path]() {
                w.openFileFromCommandLine(path);
            });
        }
    }

    return app.exec();
}