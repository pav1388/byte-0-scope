// Byte-0-scope - binary data visualizer
// File: pal_palettesettings.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pal_palettesettings.h"
#include "ui_mainwindow.h"
#include "definitions.h"
#include "pal_palettemanager.h"
#include "app_thememanager.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <climits>

// --- PaletteSettingsPanel module settings (prefix PS_) ---

#define PS_SMALL_BUTTON_WIDTH       30
#define PS_COLOR_BUTTON_HEIGHT      20
#define PS_RULE_BUTTON_WIDTH        25
#define PS_RULE_BUTTON_HEIGHT       25
#define PS_RULE_SPIN_WIDTH          70
#define PS_RULE_NUM_LABEL_WIDTH     20
#define PS_CONTENT_MARGIN           6
#define PS_CONTENT_SPACING          4

#define PS_BYTE_MAX                 255
#define PS_PLANE_MAX                7
#define PS_ADDR_MAX                 0x7FFFFFFF
#define PS_RECORD_SIZE_MAX          16
#define PS_COUNT_MAX                65536
#define PS_MIN_COUNT_MIN            2
#define PS_MIN_COUNT_MAX            100000000
#define PS_STEP_MIN                 1
#define PS_STEP_MAX                 256

namespace {

QStringList formatOptions()
{
    return {
        "auto", "grayscale", "rgb332",
        "rgb565", "bgr565", "rgb555", "argb1555",
        "rgb", "bgr", "gbr", "brg", "grb", "rbg",
        "rgba", "argb", "bgra", "abgr"
    };
}

void clearLayoutDeep(QLayout *layout)
{
    if (!layout)
        return;
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            w->hide();
            w->setParent(nullptr);
            w->deleteLater();
            delete item;
        } else if (QLayout *sub = item->layout()) {
            clearLayoutDeep(sub);
            delete sub;
        } else {
            delete item;
        }
    }
}

} // namespace

PaletteSettingsPanel::PaletteSettingsPanel(MainWindow *parent)
    : QWidget(parent), m_main(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(PS_CONTENT_SPACING);

    m_contentWidget = new QWidget(this);
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(PS_CONTENT_MARGIN, PS_CONTENT_MARGIN,
                                        PS_CONTENT_MARGIN, PS_CONTENT_MARGIN);
    m_contentLayout->setSpacing(PS_CONTENT_SPACING);

    root->addWidget(m_contentWidget, 0);

    auto *btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(0, 0, 0, 0);
    m_resetBtn = new QPushButton("X", this);
    m_resetBtn->setFixedWidth(PS_SMALL_BUTTON_WIDTH);
    m_resetBtn->setToolTip(tr("Reset palette settings"));
    connect(m_resetBtn, &QPushButton::clicked, this, [this]() {
        resetRulesToDefault();
    });
    m_autoApplyCheck = new QCheckBox(tr("Auto-apply settings"), this);
    m_autoApplyCheck->setChecked(true);
    connect(m_autoApplyCheck, &QCheckBox::toggled,
            this, &PaletteSettingsPanel::autoApplyToggled);
    btnRow->addWidget(m_resetBtn);
    btnRow->addWidget(m_autoApplyCheck);
    btnRow->addStretch();
    root->addLayout(btnRow);

    setVisible(false);
}

bool PaletteSettingsPanel::autoApplyChecked() const
{
    return m_autoApplyCheck && m_autoApplyCheck->isChecked();
}

void PaletteSettingsPanel::clearContent()
{
    m_clearing = true;

    m_ruleWidgets.clear();
    m_defaultColorBtn      = nullptr;
    m_rulesLayout          = nullptr;
    m_rulesContainer       = nullptr;
    m_stepSpin             = nullptr;
    m_planeSpin            = nullptr;
    m_thresholdSpin        = nullptr;
    m_minCountSpin         = nullptr;
    m_addrSpin             = nullptr;
    m_lenSpin              = nullptr;
    m_formatCombo          = nullptr;
    m_recordSizeSpin       = nullptr;
    m_endianCombo          = nullptr;
    m_countSpin            = nullptr;
    m_asciiExtendedCheck   = nullptr;
    m_asciiColorButtons.clear();
    m_asciiBasicColorButtons.clear();
    m_asciiExtendedColorButtons.clear();

    clearLayoutDeep(m_contentLayout);

    m_clearing = false;
}

void PaletteSettingsPanel::clear()
{
    clearContent();
    m_currentPaletteId.clear();
    setVisible(false);
}

void PaletteSettingsPanel::buildFor(const QString &paletteId)
{
    const QString idCopy = paletteId;

    clearContent();
    m_currentPaletteId = idCopy;

    if (!m_main || !m_main->paletteManager()) {
        setVisible(false);
        return;
    }

    PaletteManager *pm = m_main->paletteManager();
    QVariantMap params = m_main->paletteParams().value(idCopy).toMap();
    if (params.isEmpty())
        params = pm->defaultParams(idCopy);

    const bool configurable = pm->isConfigurable(idCopy);

    m_resetBtn->setVisible(true);
    m_autoApplyCheck->setVisible(true);

    if (idCopy == QLatin1String(PaletteIds::Custom)) {
        buildCustomUi(params);
    } else if (idCopy == QLatin1String(PaletteIds::Ascii)) {
        buildAsciiUi(params);
    } else if (idCopy == QLatin1String(PaletteIds::Entropy)) {
        buildEntropyUi(params);
    } else if (idCopy == QLatin1String(PaletteIds::Bitplane)) {
        buildPlaneUi(params);
    } else if (idCopy == QLatin1String(PaletteIds::Threshold)) {
        buildThresholdUi(params);
    } else if (idCopy == QLatin1String(PaletteIds::Repeats)) {
        buildRepeatsUi(params);
    } else if (idCopy == QLatin1String(PaletteIds::FromFile)) {
        buildFromFileUi(params);
    } else {
        setVisible(false);
        return;
    }

    setVisible(configurable);
    m_contentLayout->addStretch();
}

void PaletteSettingsPanel::addRow(const QString &label, QWidget *w)
{
    auto *row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->addWidget(new QLabel(label, w));
    row->addWidget(w);
    row->addStretch();
    m_contentLayout->addLayout(row);
}

void PaletteSettingsPanel::addInfoLabel(const QString &text)
{
    auto *label = new QLabel(text);
    label->setWordWrap(true);
    if (m_main && m_main->themeManager()) {
        label->setStyleSheet(
            m_main->themeManager()->current().styles.value("info_label"));
    }
    m_contentLayout->addWidget(label);
}

void PaletteSettingsPanel::addSeparator()
{
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    m_contentLayout->addWidget(sep);
}

// =========================================================================
//  Sections
// =========================================================================

void PaletteSettingsPanel::buildAsciiUi(const QVariantMap &params)
{
    addAsciiColorRow(tr("Printable (20–7E)"),
                     "printable_color", "#5ad296", params, false);
    addAsciiColorRow(tr("Other:"),
                     "nonprintable_color", "#e65555", params, false);

    addSeparator();

    m_asciiExtendedCheck = new QCheckBox(
        tr("Extended colors"), m_contentWidget);
    m_asciiExtendedCheck->setChecked(params.value("extended", false).toBool());
    m_contentLayout->addWidget(m_asciiExtendedCheck);

    addAsciiColorRow(tr("Controls (00-1F, 7F)"),
                     "control_color", "#aa78e6", params, true);
    addAsciiColorRow(tr("Whitespace (09-0D, 20)"),
                     "whitespace_color", "#cecece", params, true);
    addAsciiColorRow(tr("Digits (30-39)"),
                     "digit_color", "#ffcd56", params, true);
    addAsciiColorRow(tr("Uppercase (41-5A)"),
                     "uppercase_color", "#5ad296", params, true);
    addAsciiColorRow(tr("Lowercase (61-7A)"),
                     "lowercase_color", "#5ad296", params, true);
    addAsciiColorRow(tr("Symbols(21-2F,3A-40,5B-60,7B-7E)"),
                     "symbol_color", "#e1915a", params, true);
    addAsciiColorRow(tr("Extended (80-FF)"),
                     "extended_color", "#e65555", params, true);

    const bool extended = m_asciiExtendedCheck->isChecked();

    for (QPushButton *button : m_asciiBasicColorButtons)
        button->setEnabled(!extended);

    for (QPushButton *button : m_asciiExtendedColorButtons)
        button->setEnabled(extended);

    connect(m_asciiExtendedCheck, &QCheckBox::toggled, this,
            [this](bool enabled) {
        for (QPushButton *button : m_asciiBasicColorButtons)
            button->setEnabled(!enabled);

        for (QPushButton *button : m_asciiExtendedColorButtons)
            button->setEnabled(enabled);

        notifyChanged();
    });
}

void PaletteSettingsPanel::addAsciiColorRow(const QString &label,
                                             const QString &paramKey,
                                             const QString &defaultColor,
                                             const QVariantMap &params,
                                             bool extendedOnly)
{
    QPushButton *button = new QPushButton(m_contentWidget);
    button->setFixedSize(PS_SMALL_BUTTON_WIDTH, PS_COLOR_BUTTON_HEIGHT);

    QString color = params.value(paramKey, defaultColor).toString();
    if (color.isEmpty())
        color = defaultColor;

    updateColorButton(button, color);
    button->setProperty("param_key", paramKey);
    button->setToolTip(tr("Choose color (%1)").arg(color));

    connect(button, &QPushButton::clicked, this,
            [this, button]() { pickColorForButton(button); });

    QHBoxLayout *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);
    row->addWidget(button);
    row->addWidget(new QLabel(label, m_contentWidget));
    row->addStretch();

    m_contentLayout->addLayout(row);

    m_asciiColorButtons.append(button);
    if (extendedOnly)
        m_asciiExtendedColorButtons.append(button);
    else
        m_asciiBasicColorButtons.append(button);
}

void PaletteSettingsPanel::buildEntropyUi(const QVariantMap &params)
{
    m_stepSpin = new QSpinBox();
    m_stepSpin->setRange(PS_STEP_MIN, PS_STEP_MAX);
    m_stepSpin->setValue(params.value("step", 16).toInt());
    watch(m_stepSpin);
    addRow(tr("Step (bands):"), m_stepSpin);
    addInfoLabel(tr("High values - fewer bands,\nlow values - more."));
}

void PaletteSettingsPanel::buildPlaneUi(const QVariantMap &params)
{
    m_planeSpin = new QSpinBox();
    m_planeSpin->setRange(0, PS_PLANE_MAX);
    m_planeSpin->setValue(params.value("plane", 0).toInt());
    watch(m_planeSpin);
    addRow(tr("Bit plane (0–7):"), m_planeSpin);
    addInfoLabel(tr("Select bit plane."));
}

void PaletteSettingsPanel::buildThresholdUi(const QVariantMap &params)
{
    m_thresholdSpin = new QSpinBox();
    m_thresholdSpin->setRange(0, PS_BYTE_MAX);
    m_thresholdSpin->setValue(params.value("threshold", 64).toInt());
    watch(m_thresholdSpin);
    addRow(tr("Threshold (0–255):"), m_thresholdSpin);
}

void PaletteSettingsPanel::buildRepeatsUi(const QVariantMap &params)
{
    m_minCountSpin = new QSpinBox();
    m_minCountSpin->setRange(PS_MIN_COUNT_MIN, PS_MIN_COUNT_MAX);

    int value = params.value("min_count", 0).toInt();
    if (value <= 0 && m_main)
        value = m_main->averageByteRepeatCount();
    if (value < PS_MIN_COUNT_MIN)
        value = PS_MIN_COUNT_MIN;
    if (value > PS_MIN_COUNT_MAX)
        value = PS_MIN_COUNT_MAX;
    m_minCountSpin->setValue(value);

    m_minCountSpin->setToolTip(tr("Bytes appearing at least N times\n"
                                  "are highlighted in red."));
    watch(m_minCountSpin);

    addRow(tr("Min. repeat count:"), m_minCountSpin);
    addInfoLabel(tr("Byte values that occur at least N times in the file\n"
                    "are highlighted in red. The rest — grayscale.\n"
                    "Counted over the whole file, not the visible region.\n"
                    "Default = average count of distinct byte values\n"
                    "in the current file."));
}

void PaletteSettingsPanel::buildFromFileUi(const QVariantMap &params)
{
    m_addrSpin = new QSpinBox();
    m_addrSpin->setRange(0, PS_ADDR_MAX);
    m_addrSpin->setValue(params.value("address", 0).toInt());
    m_addrSpin->setGroupSeparatorShown(false);
    watch(m_addrSpin);
    addRow(tr("Start address:"), m_addrSpin);

    m_lenSpin = new QSpinBox();
    m_lenSpin->setRange(1, PS_ADDR_MAX);
    const qint64 rawLen = qint64(params.value("length", 256).toLongLong());
    m_lenSpin->setValue(qBound(1,
                               int(qMin<qint64>(rawLen, qint64(PS_ADDR_MAX))),
                               PS_ADDR_MAX));
    m_lenSpin->setGroupSeparatorShown(false);
    watch(m_lenSpin);
    addRow(tr("Max length (bytes):"), m_lenSpin);

    m_formatCombo = new QComboBox();
    m_formatCombo->addItems(formatOptions());
    m_formatCombo->setCurrentText(params.value("format", "auto").toString());
    watch(m_formatCombo);
    addRow(tr("Color format:"), m_formatCombo);

    m_recordSizeSpin = new QSpinBox();
    m_recordSizeSpin->setRange(1, PS_RECORD_SIZE_MAX);
    m_recordSizeSpin->setValue(params.value("record_size", 1).toInt());
    m_recordSizeSpin->setToolTip(tr("Size of one palette record in bytes.\n"
                                    "RGB — 3, RGB565 — 2, RGBA — 4."));
    watch(m_recordSizeSpin);
    addRow(tr("Entry size (bytes):"), m_recordSizeSpin);

    m_endianCombo = new QComboBox();
    m_endianCombo->addItems({"big", "little"});
    m_endianCombo->setCurrentText(params.value("endianness", "big").toString());
    watch(m_endianCombo);
    addRow(tr("Byte order:"), m_endianCombo);

    m_countSpin = new QSpinBox();
    m_countSpin->setRange(0, PS_COUNT_MAX);
    m_countSpin->setValue(params.value("count", 0).toInt());
    m_countSpin->setToolTip(tr("Number of palette records.\n"
                               "0 = auto: 1<<bpp for bpp<=4, otherwise 256."));
    watch(m_countSpin);
    addRow(tr("Entries (0=auto):"), m_countSpin);

    const int bpp = m_main ? m_main->bppMode() : 8;
    const QString autoHint = (bpp <= 4)
        ? tr("%1 entries (from bpp)").arg(1 << bpp)
        : tr("256 entries");

    addInfoLabel(
        tr("Format 'auto' by record size:\n"
           "1 → grayscale, 2 → RGB565,\n"
           "3 → RGB, 4+ → RGBA.\n"
           "Auto-count: %1.").arg(autoHint));
}

// =========================================================================
//  "Custom"
// =========================================================================

void PaletteSettingsPanel::buildCustomUi(const QVariantMap &params)
{
    m_defaultColorBtn = new QPushButton();
    updateColorButton(m_defaultColorBtn,
                      params.value("default_color", "#000000").toString());
    m_defaultColorBtn->setFixedHeight(PS_COLOR_BUTTON_HEIGHT);
    connect(m_defaultColorBtn, &QPushButton::clicked,
            this, [this]() { pickColorForButton(m_defaultColorBtn); });
    addRow(tr("Color outside ranges:"), m_defaultColorBtn);

    addSeparator();

    auto *header = new QLabel(tr("<b>Range rules</b>"));
    m_contentLayout->addWidget(header);

    m_rulesContainer = new QWidget();
    m_rulesLayout = new QVBoxLayout(m_rulesContainer);
    m_rulesLayout->setSpacing(PS_CONTENT_MARGIN);
    m_rulesLayout->setContentsMargins(2, 2, 2, 2);
    m_contentLayout->addWidget(m_rulesContainer);

    QVariantList rules = params.value("rules").toList();
    if (rules.isEmpty())
        rules = PaletteManager::defaultCustomRules();
    for (const QVariant &rv : rules) {
        const QVariantMap r = rv.toMap();
        addRuleWidget(r.value("start", 0).toInt(),
                      r.value("end", 0).toInt(),
                      r.value("color", "#FF0000").toString());
    }

    auto *btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(0, 0, 0, 0);
    auto *addBtn = new QPushButton(tr("+ Add rule"));
    connect(addBtn, &QPushButton::clicked, this, [this]() {
        int newStart = 0, newEnd = 31;
        if (!m_ruleWidgets.isEmpty()) {
            const int lastEnd = m_ruleWidgets.last().end->value();
            newStart = qMin(PS_BYTE_MAX, lastEnd + 1);
            newEnd   = qMin(PS_BYTE_MAX, newStart + 31);
        }
        addRuleWidget(newStart, newEnd,
                      PaletteManager::defaultRuleColor(m_ruleWidgets.size()));
        notifyChanged();
    });
    btnRow->addWidget(addBtn);
    btnRow->addStretch();
    m_contentLayout->addLayout(btnRow);
}

PaletteSettingsPanel::RuleWidget PaletteSettingsPanel::addRuleWidget(
    int start, int end, const QString &hexColor)
{
    RuleWidget rw;
    rw.container = new QWidget();
    rw.container->setAutoFillBackground(false);

    auto *lay = new QHBoxLayout(rw.container);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(PS_CONTENT_MARGIN);

    auto *numLabel = new QLabel(QString("%1.").arg(m_ruleWidgets.size() + 1));
    numLabel->setFixedWidth(PS_RULE_NUM_LABEL_WIDTH);
    numLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    numLabel->setStyleSheet(isDarkTheme() ? "color: #ccc;" : "color: #666;");
    lay->addWidget(numLabel);

    rw.start = new QSpinBox();
    rw.start->setRange(0, PS_BYTE_MAX);
    rw.start->setValue(start);
    rw.start->setFixedWidth(PS_RULE_SPIN_WIDTH);
    lay->addWidget(rw.start);

    auto *dash = new QLabel("–");
    dash->setStyleSheet(isDarkTheme() ? "color: #888;" : "color: #666;");
    lay->addWidget(dash);

    rw.end = new QSpinBox();
    rw.end->setRange(0, PS_BYTE_MAX);
    rw.end->setValue(end);
    rw.end->setFixedWidth(PS_RULE_SPIN_WIDTH);
    lay->addWidget(rw.end);

    rw.colorBtn = new QPushButton();
    updateColorButton(rw.colorBtn, hexColor);
    rw.colorBtn->setFixedSize(PS_SMALL_BUTTON_WIDTH, PS_RULE_BUTTON_HEIGHT);
    lay->addWidget(rw.colorBtn);

    auto *delBtn = new QPushButton("×");
    delBtn->setFixedSize(PS_RULE_BUTTON_WIDTH, PS_RULE_BUTTON_HEIGHT);
    delBtn->setStyleSheet(
        QString("QPushButton { color: %1; font-weight: bold; font-size: 12pt; "
                "border: none; background: transparent; } "
                "QPushButton:hover { background: %2; border-radius: 3px; }")
            .arg(isDarkTheme() ? "red" : "darkred")
            .arg(isDarkTheme() ? "#444" : "#ddd"));
    lay->addWidget(delBtn);
    lay->addStretch();

    QWidget *container = rw.container;
    QPushButton *colorBtnPtr = rw.colorBtn;

    connect(colorBtnPtr, &QPushButton::clicked, this,
            [this, colorBtnPtr]() { pickColorForButton(colorBtnPtr); });
    connect(delBtn, &QPushButton::clicked, this,
            [this, container]() { deleteRuleWidget(container); });

    watch(rw.start);
    watch(rw.end);

    m_rulesLayout->addWidget(rw.container);
    m_ruleWidgets.append(rw);
    return rw;
}

void PaletteSettingsPanel::deleteRuleWidget(QWidget *container)
{
    for (int i = 0; i < m_ruleWidgets.size(); ++i) {
        if (m_ruleWidgets[i].container == container) {
            if (m_rulesLayout)
                m_rulesLayout->removeWidget(container);
            container->hide();
            container->setParent(nullptr);
            container->deleteLater();
            m_ruleWidgets.remove(i);
            break;
        }
    }
    renumberRules();
    notifyChanged();
}

void PaletteSettingsPanel::renumberRules()
{
    for (int i = 0; i < m_ruleWidgets.size(); ++i) {
        QWidget *c = m_ruleWidgets[i].container;
        if (!c)
            continue;
        auto *lay = c->layout();
        if (!lay || lay->count() == 0)
            continue;
        auto *lbl = qobject_cast<QLabel *>(lay->itemAt(0)->widget());
        if (lbl)
            lbl->setText(QString("%1.").arg(i + 1));
    }
}

void PaletteSettingsPanel::resetRulesToDefault()
{
    if (!m_main || m_currentPaletteId.isEmpty())
        return;

    const QString paletteId = m_currentPaletteId;

    PaletteManager *pm = m_main->paletteManager();
    QVariantMap defaults = pm->defaultParams(paletteId);

    if (paletteId == QLatin1String(PaletteIds::FromFile)) {
        const quint64 fs = m_main->fileSize();
        defaults["address"]     = 0;
        defaults["length"]      = int(qMax<quint64>(1, qMin<quint64>(fs, PS_ADDR_MAX)));
        defaults["record_size"] = 1;
        defaults["format"]      = "auto";
        defaults["endianness"]  = "big";
        defaults["count"]       = 0;
        defaults["bpp"]         = m_main->bppMode();
    }

    m_main->setPaletteParams(paletteId, defaults);
    buildFor(paletteId);
    notifyChanged();
}

// =========================================================================
//  Color buttons
// =========================================================================

void PaletteSettingsPanel::updateColorButton(QPushButton *btn, const QString &hex)
{
    QString color = hex;
    if (!color.startsWith('#'))
        color = "#000000";
    btn->setProperty("color_hex", color);

    QString border;
    if (m_main && m_main->themeManager()) {
        const bool special = (color == "#000000" || color == "#ffffff");
        border = m_main->themeManager()->isDark()
            ? (special ? "#aaa" : "#888")
            : (special ? "#666" : "#444");
    } else {
        border = "#888";
    }

    btn->setStyleSheet(
        QString("QPushButton { background-color: %1; border: 1px solid %2; "
                "border-radius: 3px; } "
                "QPushButton:hover { border: 2px solid #ccc; }")
            .arg(color, border));
}

void PaletteSettingsPanel::pickColorForButton(QPushButton *btn)
{
    if (!btn)
        return;
    QString current = btn->property("color_hex").toString();
    if (current.isEmpty())
        current = "#000000";
    QColor col(current);
    if (!col.isValid())
        col = Qt::black;

    QColorDialog dlg(col, this);
    dlg.setOption(QColorDialog::ShowAlphaChannel, false);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QColor chosen = dlg.selectedColor();
    if (!chosen.isValid())
        return;

    const QString hexNew = chosen.name().toLower();
    updateColorButton(btn, hexNew);
    if (btn != m_defaultColorBtn)
        btn->setToolTip(tr("Color: %1").arg(hexNew));

    notifyChanged();
}

// =========================================================================
//  Collect parameters
// =========================================================================

QVariantMap PaletteSettingsPanel::collectCurrentParams() const
{
    QVariantMap p;

    if (m_currentPaletteId == QLatin1String(PaletteIds::Custom)) {
        if (m_defaultColorBtn) {
            const QString def = m_defaultColorBtn->property("color_hex").toString();
            if (!def.isEmpty())
                p["default_color"] = def;
        }
        QVariantList rules;
        for (const RuleWidget &rw : m_ruleWidgets) {
            if (!rw.start || !rw.end || !rw.colorBtn)
                continue;
            const int s = rw.start->value();
            const int e = rw.end->value();
            if (s > e)
                continue;
            QString c = rw.colorBtn->property("color_hex").toString();
            if (c.isEmpty())
                c = "#000000";
            QVariantMap r;
            r["start"] = s;
            r["end"]   = e;
            r["color"] = c;
            rules.append(r);
        }
        p["rules"] = rules;
        return p;
    }

    if (m_currentPaletteId == QLatin1String(PaletteIds::Ascii)) {
        p["extended"] = m_asciiExtendedCheck && m_asciiExtendedCheck->isChecked();
        for (QPushButton *button : m_asciiColorButtons) {
            if (!button)
                continue;
            const QString key = button->property("param_key").toString();
            const QString color = button->property("color_hex").toString();
            if (!key.isEmpty() && !color.isEmpty())
                p[key] = color;
        }
    } else if (m_currentPaletteId == QLatin1String(PaletteIds::Entropy)
               && m_stepSpin) {
        p["step"] = m_stepSpin->value();
    } else if (m_currentPaletteId == QLatin1String(PaletteIds::Bitplane)
               && m_planeSpin) {
        p["plane"] = m_planeSpin->value();
    } else if (m_currentPaletteId == QLatin1String(PaletteIds::Threshold)
               && m_thresholdSpin) {
        p["threshold"] = m_thresholdSpin->value();
    } else if (m_currentPaletteId == QLatin1String(PaletteIds::Repeats)
               && m_minCountSpin) {
        int v = m_minCountSpin->value();
        if (m_main) {
            const int avg = m_main->averageByteRepeatCount();
            if (avg > 0 && v == avg)
                v = 0;
        }
        p["min_count"] = v;
    } else if (m_currentPaletteId == QLatin1String(PaletteIds::FromFile)
               && m_addrSpin) {
        p["address"]     = m_addrSpin->value();
        p["length"]      = m_lenSpin->value();
        p["record_size"] = m_recordSizeSpin->value();
        p["format"]      = m_formatCombo->currentText();
        p["endianness"]  = m_endianCombo->currentText();
        p["count"]       = m_countSpin->value();
        p["bpp"]         = m_main ? m_main->bppMode() : 8;
    }

    return p;
}

bool PaletteSettingsPanel::isDarkTheme() const
{
    return m_main && m_main->themeManager() && m_main->themeManager()->isDark();
}

void PaletteSettingsPanel::watch(QSpinBox *spin)
{
    connect(spin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });
}

void PaletteSettingsPanel::watch(QComboBox *combo)
{
    connect(combo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) { notifyChanged(); });
}

void PaletteSettingsPanel::notifyChanged()
{
    if (m_clearing)
        return;
    if (m_currentPaletteId.isEmpty())
        return;
    emit paramsChanged(collectCurrentParams());
}