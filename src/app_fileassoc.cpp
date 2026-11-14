// Byte-0-scope - binary data visualizer
// File: app_fileassoc.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app_fileassoc.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <shlobj.h>
#endif

// --- Настройки модуля FileAssoc (префикс FA_) ---

// Windows: реестр
#define FA_REG_CLASSES_KEY      "HKEY_CURRENT_USER\\Software\\Classes"
#define FA_PROGID_PREFIX        "Byte0scope."
#define FA_PROGID_DESC          "Byte-0-scope config file"

// Linux: XDG MIME + .desktop
#define FA_MIME_PREFIX          "application/x-byte0scope-"
#define FA_MIME_PKG_SUBDIR      "/mime/packages"
#define FA_MIME_ROOT_SUBDIR     "/mime"
#define FA_MIME_XML_PREFIX      "/byte0scope-"
#define FA_MIME_XML_SUFFIX      ".xml"
#define FA_DESKTOP_PREFIX       "/byte0scope-"
#define FA_DESKTOP_SUFFIX       ".desktop"

// Идентичность приложения (используется и в MIME, и в .desktop)
#define FA_APP_NAME             "Byte-0-scope"
#define FA_APP_DESC             FA_PROGID_DESC

// Внешние утилиты и таймаут
#define FA_TOOL_UPDATE_MIME     "update-mime-database"
#define FA_TOOL_XDG_MIME        "xdg-mime"
#define FA_TOOL_TIMEOUT_MS      5000

namespace {

QString extensionWithoutDot(const QString &ext)
{
    return ext.startsWith(QLatin1Char('.')) ? ext.mid(1) : ext;
}

QString progIdForExt(const QString &ext)
{
    return QStringLiteral(FA_PROGID_PREFIX) + extensionWithoutDot(ext);
}

#ifdef Q_OS_WIN
void notifyShell()
{
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

bool registerWindows(const QString &ext, QString &err)
{
    const QString exePath = QCoreApplication::applicationFilePath();
    if (exePath.isEmpty()) {
        err = QCoreApplication::translate(
            "FileAssoc", "Executable path not found.");
        return false;
    }

    const QString progId = progIdForExt(ext);
    const QString exeNative = QDir::toNativeSeparators(exePath);

    QSettings classes(QStringLiteral(FA_REG_CLASSES_KEY), QSettings::NativeFormat);

    classes.setValue(progId + "/.", QStringLiteral(FA_PROGID_DESC));
    classes.setValue(progId + "/FriendlyTypeName", QStringLiteral(FA_PROGID_DESC));
    classes.setValue(progId + "/DefaultIcon/.", "\"" + exeNative + "\",0");
    classes.setValue(progId + "/shell/open/command/.",
                     "\"" + exeNative + "\" \"%1\"");
    classes.setValue(ext + "/OpenWithProgids/" + progId, QByteArray());

    classes.setValue(ext + "/.", progId);
    classes.sync();

    if (classes.status() != QSettings::NoError) {
        err = QCoreApplication::translate(
            "FileAssoc", "Failed to write to the registry.");
        return false;
    }

    notifyShell();
    return true;
}

bool unregisterWindows(const QString &ext, QString &err)
{
    const QString progId = progIdForExt(ext);

    QSettings classes(QStringLiteral(FA_REG_CLASSES_KEY), QSettings::NativeFormat);

    if (classes.value(ext + "/.").toString() == progId)
        classes.remove(ext + "/.");
    classes.remove(ext + "/OpenWithProgids/" + progId);
    classes.remove(progId);

    classes.sync();
    if (classes.status() != QSettings::NoError) {
        err = QCoreApplication::translate(
            "FileAssoc", "Failed to clean up the registry.");
        return false;
    }

    notifyShell();
    return true;
}
#endif // Q_OS_WIN

#ifdef Q_OS_LINUX
QString mimeTypeForExt(const QString &ext)
{
    return QStringLiteral(FA_MIME_PREFIX) + extensionWithoutDot(ext);
}

QString dataDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
}

QString appsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
}

QString mimeXmlPath(const QString &cleanExt)
{
    return dataDir() + QStringLiteral(FA_MIME_PKG_SUBDIR)
         + QStringLiteral(FA_MIME_XML_PREFIX) + cleanExt
         + QStringLiteral(FA_MIME_XML_SUFFIX);
}

QString desktopFilePath(const QString &cleanExt)
{
    return appsDir() + QStringLiteral(FA_DESKTOP_PREFIX) + cleanExt
         + QStringLiteral(FA_DESKTOP_SUFFIX);
}

void updateMimeDatabase()
{
    QProcess::execute(QStringLiteral(FA_TOOL_UPDATE_MIME),
                      { dataDir() + QStringLiteral(FA_MIME_ROOT_SUBDIR) });
}

bool registerLinux(const QString &ext, QString &err)
{
    const QString exePath = QCoreApplication::applicationFilePath();
    if (exePath.isEmpty()) {
        err = QCoreApplication::translate(
            "FileAssoc", "Executable path not found.");
        return false;
    }

    const QString cleanExt = extensionWithoutDot(ext);
    const QString mimeType = mimeTypeForExt(ext);

    // 1. MIME XML
    const QString xmlPath = mimeXmlPath(cleanExt);
    QDir().mkpath(QFileInfo(xmlPath).absolutePath());
    {
        QFile xml(xmlPath);
        if (!xml.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            err = QCoreApplication::translate(
                "FileAssoc", "Failed to create MIME XML.");
            return false;
        }
        QTextStream out(&xml);
        out.setCodec("UTF-8");
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            << "<mime-info xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\">\n"
            << "  <mime-type type=\"" << mimeType.toHtmlEscaped() << "\">\n"
            << "    <comment>" << FA_APP_DESC << "</comment>\n"
            << "    <glob pattern=\"*." << cleanExt.toHtmlEscaped() << "\"/>\n"
            << "  </mime-type>\n"
            << "</mime-info>\n";
    }

    updateMimeDatabase();

    // 2. .desktop файл
    const QString desktopPath = desktopFilePath(cleanExt);
    QDir().mkpath(QFileInfo(desktopPath).absolutePath());
    {
        QFile desk(desktopPath);
        if (!desk.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            err = QCoreApplication::translate(
                "FileAssoc", "Failed to create .desktop file.");
            return false;
        }
        QTextStream out(&desk);
        out.setCodec("UTF-8");
        out << "[Desktop Entry]\n"
            << "Type=Application\n"
            << "Name=" << FA_APP_NAME << "\n"
            << "Comment=Binary data visualizer\n"
            << "Exec=\"" << exePath << "\" \"%f\"\n"
            << "Terminal=false\n"
            << "MimeType=" << mimeType << ";\n"
            << "Categories=Graphics;Viewer;\n";
    }

    // 3. Назначаем приложение по умолчанию для MIME
    QProcess xdgMime;
    xdgMime.start(QStringLiteral(FA_TOOL_XDG_MIME),
                  { QStringLiteral("default"),
                    QFileInfo(desktopPath).fileName(),
                    mimeType });
    if (!xdgMime.waitForStarted(FA_TOOL_TIMEOUT_MS)) {
        err = QCoreApplication::translate(
            "FileAssoc", "xdg-mime could not be started.");
        return false;
    }
    if (!xdgMime.waitForFinished(FA_TOOL_TIMEOUT_MS)) {
        xdgMime.kill();
        xdgMime.waitForFinished(FA_TOOL_TIMEOUT_MS);
        err = QCoreApplication::translate(
            "FileAssoc", "xdg-mime did not respond within the timeout.");
        return false;
    }
    if (xdgMime.exitStatus() != QProcess::NormalExit || xdgMime.exitCode() != 0) {
        err = QCoreApplication::translate(
            "FileAssoc", "xdg-mime failed to set the default application.");
        return false;
    }

    return true;
}

void unregisterLinux(const QString &ext)
{
    const QString cleanExt = extensionWithoutDot(ext);
    QFile::remove(mimeXmlPath(cleanExt));
    QFile::remove(desktopFilePath(cleanExt));
    updateMimeDatabase();
}
#endif // Q_OS_LINUX

// Показывает итог операции, если задан родительский виджет.
void showResult(QWidget *parent, bool ok, const QString &message)
{
    if (!parent)
        return;
    if (ok)
        QMessageBox::information(parent,
            QCoreApplication::translate("FileAssoc", "Association"), message);
    else
        QMessageBox::warning(parent,
            QCoreApplication::translate("FileAssoc", "Error"), message);
}

} // namespace

namespace FileAssoc {

bool registerFileAssociation(const QString &ext, QWidget *parent)
{
    if (ext.isEmpty())
        return false;

    QString error;
    bool ok = false;

#ifdef Q_OS_WIN
    ok = registerWindows(ext, error);
#elif defined(Q_OS_LINUX)
    ok = registerLinux(ext, error);
#else
    error = QCoreApplication::translate("FileAssoc",
        "File association registration is not supported on this platform.");
#endif

    showResult(parent, ok, ok
        ? QCoreApplication::translate("FileAssoc",
              "Extension %1 has been registered to this program.").arg(ext)
        : QCoreApplication::translate("FileAssoc",
              "Failed to register %1.\n%2").arg(ext, error));
    return ok;
}

bool unregisterFileAssociation(const QString &ext, QWidget *parent)
{
    if (ext.isEmpty())
        return false;

    QString error;
    bool ok = false;

#ifdef Q_OS_WIN
    ok = unregisterWindows(ext, error);
#elif defined(Q_OS_LINUX)
    unregisterLinux(ext);
    ok = true;
#else
    error = QCoreApplication::translate("FileAssoc",
        "File association removal is not supported on this platform.");
#endif

    showResult(parent, ok, ok
        ? QCoreApplication::translate("FileAssoc",
              "Association for %1 has been removed.").arg(ext)
        : QCoreApplication::translate("FileAssoc",
              "Failed to remove %1.\n%2").arg(ext, error));
    return ok;
}

} // namespace FileAssoc
