// Byte-0-scope - binary data visualizer
// File: fileassoc.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

class QWidget;

namespace FileAssoc {

bool registerFileAssociation(const QString &ext, QWidget *parent = nullptr);
bool unregisterFileAssociation(const QString &ext, QWidget *parent = nullptr);

} // namespace FileAssoc