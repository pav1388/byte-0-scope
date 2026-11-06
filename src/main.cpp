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

#include "appversion.h"
#include "mainwindow.h"
#include "settingsmanager.h"

// --- Настройки приложения (префикс MAIN_) ---
#define MAIN_APP_STYLE          "Fusion"
#define MAIN_APP_FONT_PT        9
#define MAIN_APP_NAME           "byte-0-scope"
#define MAIN_APP_ORG            "pav13"
#define MAIN_APP_ICON_PATH      ":/icon.ico"
#define MAIN_APP_CMD_DELAY_MS   0

// --- Локализация ---
#define MAIN_TRANSLATIONS_DIR   "translations"
#define MAIN_TR_APP_PREFIX      "byte0scope_"
#define MAIN_TR_QT_PREFIX       "qtbase_"
#define MAIN_TR_RES_PREFIX      ":/translations/"
#define MAIN_LANG_DEFAULT       "en"

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
    // Путь и кодировка INI — те же, что у MainWindow (SettingsManager); раньше
    // логика выбора файла настроек была продублирована здесь.
    const QString language = SettingsManager().load()
        .value(QStringLiteral("language")).toString();
    // Язык по умолчанию — встроенные строки, переводчики не нужны.
    if (language.isEmpty() || language == QLatin1String(MAIN_LANG_DEFAULT))
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

    // Все аргументы-файлы (кроме опций «-...»). Один файл открывается, два и
    // более — первые два загружаются в режим сравнения XOR.
    QStringList files;
    const QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (!args.at(i).startsWith(QLatin1Char('-')))
            files << args.at(i);
    }
    if (!files.isEmpty()) {
        QTimer::singleShot(MAIN_APP_CMD_DELAY_MS, &w, [&w, files]() {
            w.openFiles(files);
        });
    }

    return app.exec();
}
