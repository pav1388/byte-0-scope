// Byte-0-scope - binary data visualizer
// File: controlspanel.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>
#include <QVariantMap>

class QComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;
class QPushButton;
class MainWindow;
class CanvasWidget;

// Левая панель управления (формат пикселей, палитры, регион, отображение).
class ControlsPanel : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(ControlsPanel)

public:
    explicit ControlsPanel(MainWindow *parent);

    // Текущие значения
    int     bppMode() const;
    QString currentPaletteId() const;
    QString bitOrder() const;
    QString pixelOrder() const;
    int     rowPaddingBits() const;
    int     widthMode() const;
    int     fixedWidth() const;
    int     widthStep() const;
    int     scaleValue() const;
    bool    autoScale() const;
    bool    fitToWindow() const;
    quint64 offset() const;
    quint64 length() const;
    bool    lengthMode() const;
    int     bytesPerRow() const;

    QVariantMap currentConfig() const;

    void setConfig(const QVariantMap &cfg);

    void setScaleSilent(int s);
    void setWidthModeSilent(int idx);
    void setWidthSilent(int w);
    void setCanvasWidget(CanvasWidget *canvas);

signals:
    void settingsChanged();
    void resetDisplayRequested();

private slots:
    void onBppChanged();
    void onPaletteChanged();
    void onHexModeChanged(bool checked);
    void onLengthModeChanged(bool checked);
    void onWidthModeChanged(int index);
    void resetRegionToDefault();
    void startRegionPick(bool startBoundary);
    void cancelRegionPick();
    void onRegionAddressCaptured(qint64 address);

private:
    void buildUi();
    void refillPaletteList(int bpp);
    void updateScaleSpinEnabled();
    void updateBitSettingsEnabled(int bpp);
    void applyHexModeToSpins(bool useHex);

    // Владелец
    MainWindow *m_main = nullptr;

    // Формат пикселей
    QComboBox *m_bppCombo = nullptr;
    QComboBox *m_bitOrderCombo = nullptr;
    QComboBox *m_pixelOrderCombo = nullptr;
    QSpinBox  *m_rowPadSpin = nullptr;

    // Палитра
    QComboBox *m_paletteCombo = nullptr;

    // Регион
    QSpinBox    *m_offsetSpin = nullptr;
    QSpinBox    *m_endSpin = nullptr;
    QLabel      *m_endLabel = nullptr;
    QPushButton *m_resetRegionBtn = nullptr;
    QPushButton *m_pickStartBtn = nullptr;
    QPushButton *m_pickEndBtn = nullptr;
    QCheckBox   *m_lengthModeCheck = nullptr;
    QCheckBox   *m_hexModeCheck = nullptr;
    int          m_regionPickTarget = 0;  // 0=none, 1=start, 2=end

    // Отображение
    QComboBox   *m_widthModeCombo = nullptr;
    QSpinBox    *m_widthSpin = nullptr;
    QSpinBox    *m_widthStepSpin = nullptr;
    QSpinBox    *m_scaleSpin = nullptr;
    QCheckBox   *m_autoScaleCheck = nullptr;
    QCheckBox   *m_fitCheck = nullptr;
    QSpinBox    *m_bytesPerRowSpin = nullptr;
    QLabel      *m_bytesPerRowLabel = nullptr;
    QPushButton *m_resetDisplayBtn = nullptr;

    bool m_updatingConfig = false;
};