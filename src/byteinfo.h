// Byte-0-scope - binary data visualizer
// File: byteinfo.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>
#include <QLabel>

class MainWindow;

class ByteInfoWidget : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(ByteInfoWidget)

public:
    explicit ByteInfoWidget(MainWindow *parent);

    QLabel *popupLabel() const { return m_popup; }

    bool isPopupEnabled() const { return m_popupEnabled; }
    void setPopupEnabled(bool enabled) { m_popupEnabled = enabled; }

    void updateTheme();

public slots:
    void updateInfo(qint64 addr);

private:
    QString formatSingleByte(qint64 addr, quint8 rawByte);
    QString formatBitpacked(qint64 addr, quint8 rawByte);
    QString formatMultibyte(qint64 addr, const QByteArray &dataBytes,
                            const QString &paletteId,
                            const QVariantMap &params);

    MainWindow *m_main = nullptr;
    QLabel     *m_label = nullptr;
    QLabel     *m_popup = nullptr;
    bool        m_popupEnabled = false;
};