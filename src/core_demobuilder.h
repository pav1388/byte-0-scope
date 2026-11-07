// Byte-0-scope - binary data visualizer
// File: core_demobuilder.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QString>
#include <QVariantMap>

namespace DemoBuilder {

QByteArray buildDefaultDemo(qint64 msecsSinceEpoch = 0);
QByteArray buildTrainingDemo();
QString trainingHintHtml();

} // namespace DemoBuilder