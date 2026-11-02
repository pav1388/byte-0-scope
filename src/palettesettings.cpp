// Byte-0-scope - binary data visualizer
// File: palettesettings.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "palettesettings.h"
#include "mainwindow.h"
#include "palettemanager.h"
#include "thememanager.h"

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

// --- Настройки модуля PaletteSettingsPanel (префикс PS_) ---

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
#define PS_MIN_COUNT_MAX            1000
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

    if (idCopy == "custom") {
        buildCustomUi(params);
    } else if (idCopy == "entropy") {
        buildEntropyUi(params);
    } else if (idCopy == "bitplane") {
        buildPlaneUi(params);
    } else if (idCopy == "threshold") {
        buildThresholdUi(params);
    } else if (idCopy == "repeats") {
        buildRepeatsUi(params);
    } else if (idCopy == "from_file") {
        buildFromFileUi(params);
    } else {
        // Не конфигурируемая палитра — панель не показываем.
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
//  Секции
// =========================================================================

void PaletteSettingsPanel::buildEntropyUi(const QVariantMap &params)
{
    m_stepSpin = new QSpinBox();
    m_stepSpin->setRange(PS_STEP_MIN, PS_STEP_MAX);
    m_stepSpin->setValue(params.value("step", 16).toInt());
    connect(m_stepSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });
    addRow(tr("Step (bands):"), m_stepSpin);
    addInfoLabel(tr("High values - fewer bands,\nlow values - more."));
}

void PaletteSettingsPanel::buildPlaneUi(const QVariantMap &params)
{
    m_planeSpin = new QSpinBox();
    m_planeSpin->setRange(0, PS_PLANE_MAX);
    m_planeSpin->setValue(params.value("plane", 0).toInt());
    connect(m_planeSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });
    addRow(tr("Bit plane (0–7):"), m_planeSpin);
    addInfoLabel(tr("Select bit plane."));
}

void PaletteSettingsPanel::buildThresholdUi(const QVariantMap &params)
{
    m_thresholdSpin = new QSpinBox();
    m_thresholdSpin->setRange(0, PS_BYTE_MAX);
    m_thresholdSpin->setValue(params.value("threshold", 64).toInt());
    connect(m_thresholdSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });
    addRow(tr("Threshold (0–255):"), m_thresholdSpin);
}

void PaletteSettingsPanel::buildRepeatsUi(const QVariantMap &params)
{
    m_minCountSpin = new QSpinBox();
    m_minCountSpin->setRange(PS_MIN_COUNT_MIN, PS_MIN_COUNT_MAX);
    m_minCountSpin->setValue(params.value("min_count", 50).toInt());
    m_minCountSpin->setToolTip(tr("Bytes appearing at least N times\n"
                                  "are highlighted in red."));
    connect(m_minCountSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });

    addRow(tr("Min. repeat count:"), m_minCountSpin);
	addInfoLabel(tr("Byte values that occur at least N times in the file\n"
					"are highlighted in red. The rest — grayscale.\n"
					"Counted over the whole file, not the visible region."));
}

void PaletteSettingsPanel::buildFromFileUi(const QVariantMap &params)
{
    m_addrSpin = new QSpinBox();
    m_addrSpin->setRange(0, PS_ADDR_MAX);
    m_addrSpin->setValue(params.value("address", 0).toInt());
    m_addrSpin->setGroupSeparatorShown(false);
    connect(m_addrSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });
    addRow(tr("Start address:"), m_addrSpin);

    m_lenSpin = new QSpinBox();
    m_lenSpin->setRange(1, PS_ADDR_MAX);
    const qint64 rawLen = qint64(params.value("length", 256).toLongLong());
    m_lenSpin->setValue(qBound(1,
                               int(qMin<qint64>(rawLen, qint64(PS_ADDR_MAX))),
                               PS_ADDR_MAX));
    m_lenSpin->setGroupSeparatorShown(false);
    connect(m_lenSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });
    addRow(tr("Max length (bytes):"), m_lenSpin);

    m_formatCombo = new QComboBox();
    m_formatCombo->addItems(formatOptions());
    m_formatCombo->setCurrentText(params.value("format", "auto").toString());
    connect(m_formatCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) { notifyChanged(); });
    addRow(tr("Color format:"), m_formatCombo);

    m_recordSizeSpin = new QSpinBox();
    m_recordSizeSpin->setRange(1, PS_RECORD_SIZE_MAX);
    m_recordSizeSpin->setValue(params.value("record_size", 1).toInt());
    m_recordSizeSpin->setToolTip(tr("Size of one palette record in bytes.\n"
                                    "RGB — 3, RGB565 — 2, RGBA — 4."));
    connect(m_recordSizeSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });
    addRow(tr("Entry size (bytes):"), m_recordSizeSpin);

    m_endianCombo = new QComboBox();
    m_endianCombo->addItems({"big", "little"});
    m_endianCombo->setCurrentText(params.value("endianness", "big").toString());
    connect(m_endianCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) { notifyChanged(); });
    addRow(tr("Byte order:"), m_endianCombo);

    m_countSpin = new QSpinBox();
    m_countSpin->setRange(0, PS_COUNT_MAX);
    m_countSpin->setValue(params.value("count", 0).toInt());
    m_countSpin->setToolTip(tr("Number of palette records.\n"
                               "0 = auto: 1<<bpp for bpp<=4, otherwise 256."));
    connect(m_countSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });
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
//  «Пользовательская»
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
        static const char *colors[] = {
            "#FF0000", "#00FF00", "#0000FF", "#FF0080",
            "#80FF00", "#0080FF", "#FF8000", "#8000FF"
        };
        const int ci = m_ruleWidgets.size() % 8;
        addRuleWidget(newStart, newEnd, colors[ci]);
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
    rw.container->setStyleSheet("background-color: transparent;");

    auto *lay = new QHBoxLayout(rw.container);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(PS_CONTENT_MARGIN);

    auto *numLabel = new QLabel(QString("%1.").arg(m_ruleWidgets.size() + 1));
    numLabel->setFixedWidth(PS_RULE_NUM_LABEL_WIDTH);
    numLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    numLabel->setStyleSheet(m_main && m_main->themeManager()
        && m_main->themeManager()->isDark() ? "color: #ccc;" : "color: #666;");
    lay->addWidget(numLabel);

    rw.start = new QSpinBox();
    rw.start->setRange(0, PS_BYTE_MAX);
    rw.start->setValue(start);
    rw.start->setFixedWidth(PS_RULE_SPIN_WIDTH);
    lay->addWidget(rw.start);

    auto *dash = new QLabel("–");
    dash->setStyleSheet(m_main && m_main->themeManager()
        && m_main->themeManager()->isDark() ? "color: #888;" : "color: #666;");
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
            .arg(m_main && m_main->themeManager()
                 && m_main->themeManager()->isDark() ? "red" : "darkred")
            .arg(m_main && m_main->themeManager()
                 && m_main->themeManager()->isDark() ? "#444" : "#ddd"));
    lay->addWidget(delBtn);
    lay->addStretch();

    QWidget *container = rw.container;
    QPushButton *colorBtnPtr = rw.colorBtn;

    connect(colorBtnPtr, &QPushButton::clicked, this,
            [this, colorBtnPtr]() { pickColorForButton(colorBtnPtr); });
    connect(delBtn, &QPushButton::clicked, this,
            [this, container]() { deleteRuleWidget(container); });

    connect(rw.start,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });
    connect(rw.end,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });

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

    if (paletteId == "from_file") {
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
//  Цветовые кнопки
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
//  Сбор параметров
// =========================================================================

QVariantMap PaletteSettingsPanel::collectCurrentParams() const
{
    QVariantMap p;

    if (m_currentPaletteId == "custom") {
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

    if (m_currentPaletteId == "entropy" && m_stepSpin) {
        p["step"] = m_stepSpin->value();
    } else if (m_currentPaletteId == "bitplane" && m_planeSpin) {
        p["plane"] = m_planeSpin->value();
    } else if (m_currentPaletteId == "threshold" && m_thresholdSpin) {
        p["threshold"] = m_thresholdSpin->value();
    } else if (m_currentPaletteId == "repeats" && m_minCountSpin) {
        p["min_count"] = m_minCountSpin->value();
    } else if (m_currentPaletteId == "from_file" && m_addrSpin) {
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

void PaletteSettingsPanel::notifyChanged()
{
    if (m_clearing)
        return;
    if (m_currentPaletteId.isEmpty())
        return;
    emit paramsChanged(collectCurrentParams());
}