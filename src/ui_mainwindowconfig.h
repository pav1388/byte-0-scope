// Byte-0-scope - binary data visualizer
// File: ui_mainwindowconfig.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>
#include <QVariantMap>

//  MainWindow configuration layer
struct StartupDisplayDefaults {
    bool hasAny = false;

    int bppMode = 8;
    QString paletteId;
    QString bitOrder;
    QString byteOrder;
    QString pixelOrder;
    int rowPaddingBits = 0;

    int scale = 1;
    QString widthModeKey;
    int fixedWidth = 256;
    int widthStep = 8;
    int bytesPerRow = 16;

    bool autoScale = false;
    bool fitToWindow = false;
    bool byteTooltip = false;
};

StartupDisplayDefaults loadStartupDisplayDefaults(const QVariantMap &ini);
QVariantMap startupDefaultsToConfig(const StartupDisplayDefaults &s);
bool readConfigFile(const QString &path, QJsonObject *out, QString *error);
