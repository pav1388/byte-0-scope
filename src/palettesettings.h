// Byte-0-scope - binary data visualizer
// File: palettesettings.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>
#include <QVariantMap>
#include <QVector>

class QVBoxLayout;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QPushButton;
class QLabel;
class MainWindow;

class PaletteSettingsPanel : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(PaletteSettingsPanel)

public:
    explicit PaletteSettingsPanel(MainWindow *parent);

    void buildFor(const QString &paletteId);
    void clear();

    QString currentPaletteId() const { return m_currentPaletteId; }
    bool    autoApplyChecked() const;

signals:
    void paramsChanged(const QVariantMap &newParams);
    void autoApplyToggled(bool enabled);

private:
    struct RuleWidget {
        QWidget     *container = nullptr;
        QSpinBox    *start     = nullptr;
        QSpinBox    *end       = nullptr;
        QPushButton *colorBtn  = nullptr;
    };

    void buildCustomUi(const QVariantMap &params);
    void buildEntropyUi(const QVariantMap &params);
    void buildPlaneUi(const QVariantMap &params);
    void buildThresholdUi(const QVariantMap &params);
    void buildRepeatsUi(const QVariantMap &params);
    void buildFromFileUi(const QVariantMap &params);
    void buildRgb565Ui(const QString &name, const QVariantMap &params);
    void buildIntegerUi(const QString &name, const QVariantMap &params);
    void buildFloatUi(const QString &name, const QVariantMap &params);

    void addRow(const QString &label, QWidget *w);
    void addInfoLabel(const QString &text);
    void addSeparator();
    void clearContent();

    RuleWidget addRuleWidget(int start, int end, const QString &hexColor);
    void       deleteRuleWidget(QWidget *container);
    void       resetRulesToDefault();
    void       renumberRules();

    void updateColorButton(QPushButton *btn, const QString &hex);
    void pickColorForButton(QPushButton *btn);

    QVariantMap collectCurrentParams() const;
    void        notifyChanged();

    MainWindow *m_main = nullptr;
    QString     m_currentPaletteId;

    QWidget     *m_contentWidget = nullptr;
    QVBoxLayout *m_contentLayout = nullptr;

    QPushButton *m_resetBtn       = nullptr;
    QCheckBox   *m_autoApplyCheck = nullptr;

    QVector<RuleWidget> m_ruleWidgets;
    QPushButton *m_defaultColorBtn = nullptr;
    QVBoxLayout *m_rulesLayout     = nullptr;
    QWidget     *m_rulesContainer  = nullptr;

    QSpinBox  *m_stepSpin       = nullptr;
    QSpinBox  *m_planeSpin      = nullptr;
    QSpinBox  *m_thresholdSpin  = nullptr;
    QSpinBox  *m_minCountSpin   = nullptr;
    QSpinBox  *m_addrSpin       = nullptr;
    QSpinBox  *m_lenSpin        = nullptr;
    QComboBox *m_formatCombo    = nullptr;
    QSpinBox  *m_recordSizeSpin = nullptr;
    QComboBox *m_endianCombo    = nullptr;
    QSpinBox  *m_countSpin      = nullptr;
    QComboBox *m_rgb565EndianCombo = nullptr;

    QComboBox *m_int16Endian = nullptr;  QCheckBox *m_int16Signed = nullptr;
    QComboBox *m_int32Endian = nullptr;  QCheckBox *m_int32Signed = nullptr;
    QComboBox *m_int64Endian = nullptr;  QCheckBox *m_int64Signed = nullptr;
    QComboBox *m_float32Endian = nullptr;
    QComboBox *m_float64Endian = nullptr;

    bool m_clearing = false;
};