// Byte-0-scope - binary data visualizer
// File: fileassoc.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileassoc.h"

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
#define FA_APP_DESC             "Byte-0-scope config file"

// Внешние утилиты и таймаут
#define FA_TOOL_UPDATE_MIME     "update-mime-database"
#define FA_TOOL_XDG_MIME        "xdg-mime"
#define FA_TOOL_TIMEOUT_MS      5000

namespace {

QString extensionWithoutDot(const QString &ext)
{
    QString s = ext;
    if (s.startsWith('.'))
        s = s.mid(1);
    return s;
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

bool registerWindows(const QString &ext, QString *err)
{
    const QString exePath = QCoreApplication::applicationFilePath();
    if (exePath.isEmpty()) {
        if (err)
            *err = QCoreApplication::translate(
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
        if (err)
            *err = QCoreApplication::translate(
                "FileAssoc", "Failed to write to the registry.");
        return false;
    }

    notifyShell();
    return true;
}

bool unregisterWindows(const QString &ext, QString *err)
{
    const QString progId = progIdForExt(ext);

    QSettings classes(QStringLiteral(FA_REG_CLASSES_KEY), QSettings::NativeFormat);

    // FIX: раньше вызывался classes.remove(ext), что стирало подраздел .ext
    // целиком — включая OpenWithProgids других программ. Убираем только наши
    // записи и только если мы всё ещё назначены по умолчанию.
    if (classes.value(ext + "/.").toString() == progId)
        classes.remove(ext + "/.");
    classes.remove(ext + "/OpenWithProgids/" + progId);
    classes.remove(progId);

    classes.sync();
    if (classes.status() != QSettings::NoError) {
        if (err)
            *err = QCoreApplication::translate(
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

bool registerLinux(const QString &ext, QString *err)
{
    const QString exePath = QCoreApplication::applicationFilePath();
    if (exePath.isEmpty()) {
        if (err)
            *err = QCoreApplication::translate(
                "FileAssoc", "Executable path not found.");
        return false;
    }

    const QString cleanExt = extensionWithoutDot(ext);
    const QString mimeType = mimeTypeForExt(ext);
    const QString baseDir =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

    // 1. MIME XML
    const QString mimeDir = baseDir + QStringLiteral(FA_MIME_PKG_SUBDIR);
    QDir().mkpath(mimeDir);
    const QString xmlPath = mimeDir + QStringLiteral(FA_MIME_XML_PREFIX) +
                            cleanExt + QStringLiteral(FA_MIME_XML_SUFFIX);
    {
        QFile xml(xmlPath);
        if (!xml.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (err)
                *err = QCoreApplication::translate(
                    "FileAssoc", "Failed to create MIME XML.");
            return false;
        }
        QTextStream out(&xml);
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            << "<mime-info xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\">\n"
            << "  <mime-type type=\"" << mimeType << "\">\n"
            << "    <comment>" << FA_APP_DESC << "</comment>\n"
            << "    <glob pattern=\"*." << cleanExt << "\"/>\n"
            << "  </mime-type>\n"
            << "</mime-info>\n";
    }

    // Best-effort: утилита может отсутствовать — не считаем фатальным,
    // сам MIME XML уже создан.
    QProcess::execute(QStringLiteral(FA_TOOL_UPDATE_MIME),
                      { baseDir + QStringLiteral(FA_MIME_ROOT_SUBDIR) });

    // 2. .desktop файл
    const QString appsDir =
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(appsDir);
    const QString desktopPath = appsDir + QStringLiteral(FA_DESKTOP_PREFIX) +
                                cleanExt + QStringLiteral(FA_DESKTOP_SUFFIX);
    {
        QFile desk(desktopPath);
        if (!desk.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (err)
                *err = QCoreApplication::translate(
                    "FileAssoc", "Failed to create .desktop file.");
            return false;
        }
        QTextStream out(&desk);
        out << "[Desktop Entry]\n"
            << "Type=Application\n"
            << "Name=" << FA_APP_NAME << "\n"
            << "Comment=Binary file visualizer\n"
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
    if (!xdgMime.waitForFinished(FA_TOOL_TIMEOUT_MS)) {
        if (err)
            *err = QCoreApplication::translate(
                "FileAssoc", "xdg-mime did not respond within the timeout.");
        return false;
    }

    return true;
}

bool unregisterLinux(const QString &ext, QString *err)
{
    // Best-effort: функция всегда возвращает true, err намеренно не заполняется.
    Q_UNUSED(err);

    const QString cleanExt = extensionWithoutDot(ext);
    const QString baseDir =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString appsDir =
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);

    QFile::remove(baseDir + QStringLiteral(FA_MIME_PKG_SUBDIR) +
                  QStringLiteral(FA_MIME_XML_PREFIX) + cleanExt +
                  QStringLiteral(FA_MIME_XML_SUFFIX));
    QFile::remove(appsDir + QStringLiteral(FA_DESKTOP_PREFIX) + cleanExt +
                  QStringLiteral(FA_DESKTOP_SUFFIX));

    QProcess::execute(QStringLiteral(FA_TOOL_UPDATE_MIME),
                      { baseDir + QStringLiteral(FA_MIME_ROOT_SUBDIR) });
    return true;
}
#endif // Q_OS_LINUX

} // namespace

namespace FileAssoc {

bool registerFileAssociation(const QString &ext, QWidget *parent)
{
    if (ext.isEmpty())
        return false;

    QString error;
    bool ok = false;

#ifdef Q_OS_WIN
    ok = registerWindows(ext, &error);
#elif defined(Q_OS_LINUX)
    ok = registerLinux(ext, &error);
#else
    error = QCoreApplication::translate("FileAssoc",
        "File association registration is not supported on this platform.");
#endif

    if (parent) {
        if (ok) {
            QMessageBox::information(parent,
                QCoreApplication::translate("FileAssoc", "Association"),
                QCoreApplication::translate("FileAssoc",
                    "Extension %1 has been registered to this program.").arg(ext));
        } else {
            QMessageBox::warning(parent,
                QCoreApplication::translate("FileAssoc", "Error"),
                QCoreApplication::translate("FileAssoc",
                    "Failed to register %1.\n%2").arg(ext, error));
        }
    }
    return ok;
}

bool unregisterFileAssociation(const QString &ext, QWidget *parent)
{
    if (ext.isEmpty())
        return false;

    QString error;
    bool ok = false;

#ifdef Q_OS_WIN
    ok = unregisterWindows(ext, &error);
#elif defined(Q_OS_LINUX)
    ok = unregisterLinux(ext, &error);
#else
    error = QCoreApplication::translate("FileAssoc",
        "File association removal is not supported on this platform.");
#endif

    if (parent) {
        if (ok) {
            QMessageBox::information(parent,
                QCoreApplication::translate("FileAssoc", "Association"),
                QCoreApplication::translate("FileAssoc",
                    "Association for %1 has been removed.").arg(ext));
        } else {
            QMessageBox::warning(parent,
                QCoreApplication::translate("FileAssoc", "Error"),
                QCoreApplication::translate("FileAssoc",
                    "Failed to remove %1.\n%2").arg(ext, error));
        }
    }
    return ok;
}

} // namespace FileAssoc