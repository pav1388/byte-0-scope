// Byte-0-scope - binary data visualizer
// File: ui_controlspanel.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_controlspanel.h"
#include "ui_mainwindow.h"
#include "ui_int64spinbox.h"
#include "pal_palettemanager.h"
#include "render_canvaswidget.h"
#include "definitions.h"


#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QEvent>
#include <QWheelEvent>

// --- Настройки модуля ControlsPanel (префикс CP_) ---

#define CP_PANEL_FIXED_WIDTH        258
#define CP_LAYOUT_MARGIN            2
#define CP_LAYOUT_SPACING           4
#define CP_LAYOUT_COL_MIN_WIDTH     150
#define CP_BUTTON_COMPACT_WIDTH     32
#define CP_LABEL_END                QT_TRANSLATE_NOOP("ControlsPanel", "End:")
#define CP_LABEL_LENGTH             QT_TRANSLATE_NOOP("ControlsPanel", "Length:")

namespace {

class WidthWheelFilter : public QObject {
public:
    WidthWheelFilter(QSpinBox *spin, QSpinBox *step, QObject *parent = nullptr)
        : QObject(parent), m_spin(spin), m_step(step) {}

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (obj == m_spin && ev->type() == QEvent::Wheel) {
            if (!m_spin || !m_spin->isEnabled())
                return false;

            QWheelEvent *we = static_cast<QWheelEvent *>(ev);
            const int wheel = we->angleDelta().y();
            if (wheel == 0)
                return false;
            const int delta = (wheel > 0) ? +1 : -1;
            const int step = qMax(1, m_step ? m_step->value() : 8);
            const int cur = m_spin->value();
            const int nv = qMax(1, ((cur / step) + delta) * step);
            m_spin->setValue(nv);
            we->accept();
            return true;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    QSpinBox *m_spin;
    QSpinBox *m_step;
};

// Находит в комбобоксе элемент с данными value и выбирает его.
bool selectByData(QComboBox *combo, const QVariant &value)
{
    const int idx = combo->findData(value);
    if (idx < 0)
        return false;
    combo->setCurrentIndex(idx);
    return true;
}

} // namespace

// =========================================================================
//  Конструктор
// =========================================================================

ControlsPanel::ControlsPanel(MainWindow *parent)
    : QWidget(parent), m_main(parent)
{
    setFixedWidth(CP_PANEL_FIXED_WIDTH);
    buildUi();
}

// =========================================================================
//  Построение UI
// =========================================================================

void ControlsPanel::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(CP_LAYOUT_MARGIN, CP_LAYOUT_MARGIN,
                               CP_LAYOUT_MARGIN, CP_LAYOUT_MARGIN);
    layout->setSpacing(CP_LAYOUT_SPACING);

    // --- Формат пикселей ---
    auto *fmtGroup = new QGroupBox(tr("Pixel format"), this);
    auto *fmtLay = new QGridLayout(fmtGroup);
    int row = 0;

    fmtLay->addWidget(new QLabel(tr("Bits per pixel:")), row, 0);
    m_bppCombo = new QComboBox(this);
    m_bppCombo->setObjectName("bppCombo");
    for (int bpp : { 64, 32, 24, 16, 8, 4, 3, 2, 1 }) {
        m_bppCombo->addItem(QString::number(bpp) + QStringLiteral(" bpp"), bpp);
    }
    selectByData(m_bppCombo, CFG_DEF_BPP_MODE);
    connect(m_bppCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) { onBppChanged(); });
    fmtLay->addWidget(m_bppCombo, row++, 1);

    m_bitOrderLabel = new QLabel(tr("Bit order:"), this);
    fmtLay->addWidget(m_bitOrderLabel, row, 0);
    m_bitOrderCombo = new QComboBox(this);
    m_bitOrderCombo->setObjectName("bitOrderCombo");
    m_bitOrderCombo->addItem(QStringLiteral("MSB-first"), QStringLiteral("msb"));
    m_bitOrderCombo->addItem(QStringLiteral("LSB-first"), QStringLiteral("lsb"));
    watch(m_bitOrderCombo);
    fmtLay->addWidget(m_bitOrderCombo, row++, 1);

    m_byteOrderLabel = new QLabel(tr("Byte order:"), this);
    fmtLay->addWidget(m_byteOrderLabel, row, 0);
    m_byteOrderCombo = new QComboBox(this);
    m_byteOrderCombo->setObjectName("byteOrderCombo");
    m_byteOrderCombo->addItem(QStringLiteral("Big-endian"), QStringLiteral("big"));
    m_byteOrderCombo->addItem(QStringLiteral("Little-endian"), QStringLiteral("little"));
    m_byteOrderCombo->setToolTip(tr(
        "Byte order within multi-byte pixel values (16/24/32/64 bpp).\n"
        "For RGB/RGBA palettes it also reverses the channel order\n"
        "(e.g. RGB + little = BGR)."));
    watch(m_byteOrderCombo);
    fmtLay->addWidget(m_byteOrderCombo, row++, 1);

    m_rowPadLabel = new QLabel(tr("Padding (bits):"), this);
    fmtLay->addWidget(m_rowPadLabel, row, 0);
    m_rowPadSpin = new QSpinBox(this);
    m_rowPadSpin->setObjectName("rowPadSpin");
    m_rowPadSpin->setRange(0, 1024);
    m_rowPadSpin->setValue(0);
    m_rowPadSpin->setToolTip(tr("Row alignment. 0 — no padding.\n"
                                "8 — rows are multiples of 1 byte.\n"
                                "16 — multiples of 2 bytes.\n"
                                "32 — multiples of 4 bytes."));
    watch(m_rowPadSpin);
    fmtLay->addWidget(m_rowPadSpin, row++, 1);

    m_pixelOrderLabel = new QLabel(tr("Pixel order:"), this);
    fmtLay->addWidget(m_pixelOrderLabel, row, 0);
    m_pixelOrderCombo = new QComboBox(this);
    m_pixelOrderCombo->setObjectName("pixelOrderCombo");
    m_pixelOrderCombo->addItem(QStringLiteral("MSB-first"), QStringLiteral("msb"));
    m_pixelOrderCombo->addItem(QStringLiteral("LSB-first"), QStringLiteral("lsb"));
    watch(m_pixelOrderCombo);
    fmtLay->addWidget(m_pixelOrderCombo, row++, 1);

    layout->addWidget(fmtGroup);

    // --- Палитры ---
    auto *palGroup = new QGroupBox(tr("Palettes"), this);
    auto *palLay = new QVBoxLayout(palGroup);
    m_paletteCombo = new QComboBox(this);
    m_paletteCombo->setObjectName("paletteCombo");
    m_paletteCombo->setMaxVisibleItems(30);
    connect(m_paletteCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) { onPaletteChanged(); });
    palLay->addWidget(m_paletteCombo);
    layout->addWidget(palGroup);

    // --- Регион ---
    auto *regGroup = new QGroupBox(tr("Region"), this);
    auto *regLay = new QGridLayout(regGroup);
    regLay->setColumnMinimumWidth(1, CP_LAYOUT_COL_MIN_WIDTH);
    regLay->setColumnStretch(1, 1);

    regLay->addWidget(new QLabel(tr("Start:")), 0, 0);
    m_offsetSpin = new Int64SpinBox(this);
    m_offsetSpin->setObjectName("offsetSpin");
    m_offsetSpin->setRange64(0, Int64SpinBox::maxAllowed());
    m_offsetSpin->setValue64(0);
    m_offsetSpin->setKeyboardTracking(false);
    m_offsetSpin->setToolTip(tr("Region start (default 0)"));
    connect(m_offsetSpin, &Int64SpinBox::value64Changed,
            this, [this](qint64) {
                if (m_updatingConfig)
                    return;
                clampRegionBounds(true);
                emit settingsChanged();
            });
    regLay->addWidget(m_offsetSpin, 0, 1);

    m_pickStartBtn = new QPushButton(QString("|+"), this);
    m_pickStartBtn->setObjectName("pickStartBtn");
    m_pickStartBtn->setFixedWidth(CP_BUTTON_COMPACT_WIDTH);
    m_pickStartBtn->setCheckable(true);
    m_pickStartBtn->setAutoDefault(false);
    m_pickStartBtn->setToolTip(tr("Pick region start: click, then left-click the desired byte.\n"
                                  "Click again or press Esc to cancel."));
    connect(m_pickStartBtn, &QPushButton::clicked, this, [this]() {
        if (m_regionPickTarget == PickStart)
            cancelRegionPick();
        else
            startRegionPick(true);
    });
    regLay->addWidget(m_pickStartBtn, 0, 2);

    m_endLabel = new QLabel(tr(CP_LABEL_END), this);
    regLay->addWidget(m_endLabel, 1, 0);
    m_endSpin = new Int64SpinBox(this);
    m_endSpin->setObjectName("endSpin");
    m_endSpin->setRange64(0, Int64SpinBox::maxAllowed());
    m_endSpin->setValue64(0);
    m_endSpin->setKeyboardTracking(false);
    m_endSpin->setToolTip(tr("Region end, or length if length mode is enabled"));
    connect(m_endSpin, &Int64SpinBox::value64Changed,
            this, [this](qint64) {
                if (m_updatingConfig)
                    return;
                clampRegionBounds(false);
                emit settingsChanged();
            });
    regLay->addWidget(m_endSpin, 1, 1);

    m_pickEndBtn = new QPushButton(QString("+|"), this);
    m_pickEndBtn->setObjectName("pickEndBtn");
    m_pickEndBtn->setFixedWidth(CP_BUTTON_COMPACT_WIDTH);
    m_pickEndBtn->setCheckable(true);
    m_pickEndBtn->setAutoDefault(false);
    m_pickEndBtn->setToolTip(tr("Pick region end: click, then left-click the desired byte.\n"
                                "Click again or press Esc to cancel."));
    connect(m_pickEndBtn, &QPushButton::clicked, this, [this]() {
        if (m_regionPickTarget == PickEnd)
            cancelRegionPick();
        else
            startRegionPick(false);
    });
    regLay->addWidget(m_pickEndBtn, 1, 2);

    m_hexModeCheck = new QCheckBox(tr("hex"), this);
    m_hexModeCheck->setObjectName("hexModeCheck");
    m_hexModeCheck->setToolTip(tr("Use hexadecimal input (0x…)"));
    connect(m_hexModeCheck, &QCheckBox::toggled,
            this, &ControlsPanel::onHexModeChanged);
	regLay->addWidget(m_hexModeCheck, 2, 0, 1, 1);
    
	m_lengthModeCheck = new QCheckBox(tr("Length mode"), this);
    m_lengthModeCheck->setObjectName("lengthModeCheck");
    m_lengthModeCheck->setToolTip(
        tr("If checked, the 'End' field is interpreted as 'Length'"));
    connect(m_lengthModeCheck, &QCheckBox::toggled,
            this, &ControlsPanel::onLengthModeChanged);
    regLay->addWidget(m_lengthModeCheck, 2, 1, 1, 1);

    m_resetRegionBtn = new QPushButton(tr("X"), this);
    m_resetRegionBtn->setObjectName("resetRegionBtn");
    m_resetRegionBtn->setFixedWidth(CP_BUTTON_COMPACT_WIDTH);
    m_resetRegionBtn->setAutoDefault(false);
    m_resetRegionBtn->setToolTip(tr("Reset region: display all data"));
    connect(m_resetRegionBtn, &QPushButton::clicked,
            this, &ControlsPanel::resetRegionToDefault);
    regLay->addWidget(m_resetRegionBtn, 2, 2, 1, 1);

    layout->addWidget(regGroup);

    // --- Отображение ---
    auto *dispGroup = new QGroupBox(tr("Display"), this);
    auto *dispLay = new QGridLayout(dispGroup);
    int dr = 0;

    dispLay->addWidget(new QLabel(tr("Width mode:")), dr, 0);
    m_widthModeCombo = new QComboBox(this);
    m_widthModeCombo->addItems({
        tr("Manual"),
        tr("Auto (1:1)"),
        tr("Auto (power of 2)"),
        tr("Auto (16:9)"),
        tr("Auto (4:3)"),
        tr("Bytes per row"),
        tr("Nearest \"round\" value"),
        tr("Divisor ≤ N"),
    });
    m_widthModeCombo->setCurrentIndex(WM_Fixed);
    connect(m_widthModeCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ControlsPanel::onWidthModeChanged);
    dispLay->addWidget(m_widthModeCombo, dr++, 1);

    m_bytesPerRowLabel = new QLabel(tr("Bytes per row:"), this);
    dispLay->addWidget(m_bytesPerRowLabel, dr, 0);
    m_bytesPerRowSpin = new QSpinBox(this);
    m_bytesPerRowSpin->setRange(1, 65536);
    m_bytesPerRowSpin->setValue(16);
    m_bytesPerRowSpin->setToolTip(tr("Only for \"Bytes per row\" mode.\n"
                                     "Pixels per row = bytes / bytesPerPixel."));
    watch(m_bytesPerRowSpin);
    dispLay->addWidget(m_bytesPerRowSpin, dr++, 1);

    m_bytesPerRowLabel->setVisible(false);
    m_bytesPerRowSpin->setVisible(false);

    m_widthLabel = new QLabel(tr("Width / N:"), this);
    dispLay->addWidget(m_widthLabel, dr, 0);
    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(1, 65536);
    m_widthSpin->setValue(256);
    m_widthSpin->setToolTip(tr("For \"Manual\" — width in pixels.\n"
                               "For \"Divisor ≤ N\" — upper width limit."));
    watch(m_widthSpin);
    dispLay->addWidget(m_widthSpin, dr++, 1);

    m_widthStepLabel = new QLabel(tr("Width step:"), this);
    dispLay->addWidget(m_widthStepLabel, dr, 0);
    m_widthStepSpin = new QSpinBox(this);
    m_widthStepSpin->setObjectName("widthStepSpin");
    m_widthStepSpin->setRange(1, 65536);
    m_widthStepSpin->setValue(8);
    m_widthStepSpin->setToolTip(tr("Width change step with Ctrl+mouse wheel.\n"
                                   "Used in manual mode."));
    watch(m_widthStepSpin);
    dispLay->addWidget(m_widthStepSpin, dr++, 1);

    dispLay->addWidget(new QLabel(tr("Zoom:")), dr, 0);
    m_scaleSpin = new QSpinBox(this);
    m_scaleSpin->setObjectName("scaleSpin");
    m_scaleSpin->setRange(1, 32);
    m_scaleSpin->setValue(1);
    watch(m_scaleSpin);
    dispLay->addWidget(m_scaleSpin, dr++, 1);

    m_autoScaleCheck = new QCheckBox(tr("Auto zoom"), this);
    m_autoScaleCheck->setObjectName("autoScaleCheck");
    m_autoScaleCheck->setToolTip(tr("Pick a zoom so the image takes up\n"
                                    "maximum space in the window, but without shrinking.\n"
                                    "Mutually exclusive with \"Fit to window\"."));
    connect(m_autoScaleCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_updatingConfig)
            return;
        if (checked && m_fitCheck->isChecked()) {
            QSignalBlocker b(m_fitCheck);
            m_fitCheck->setChecked(false);
        }
        updateDisplayControlsEnabled();
        emit settingsChanged();
    });
    dispLay->addWidget(m_autoScaleCheck, dr++, 0, 1, 2);

    m_fitCheck = new QCheckBox(tr("Fit to window"), this);
    m_fitCheck->setObjectName("fitCheck");
    m_fitCheck->setToolTip(tr("Fit the entire image within the window bounds,\n"
                              "shrinking if necessary.\n"
                              "Mutually exclusive with \"Auto zoom\"."));
    connect(m_fitCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_updatingConfig)
            return;
        if (checked && m_autoScaleCheck->isChecked()) {
            QSignalBlocker b(m_autoScaleCheck);
            m_autoScaleCheck->setChecked(false);
        }
        updateDisplayControlsEnabled();
        emit settingsChanged();
    });
    dispLay->addWidget(m_fitCheck, dr++, 0, 1, 2);

    layout->addWidget(dispGroup);

    m_resetDisplayBtn = new QPushButton(tr("[X] Reset display\nsettings"), this);
    m_resetDisplayBtn->setObjectName("resetDisplayBtn");
    m_resetDisplayBtn->setAutoDefault(false);
    m_resetDisplayBtn->setToolTip(tr("Restore display parameters to default values"));
    connect(m_resetDisplayBtn, &QPushButton::clicked,
            this, &ControlsPanel::resetDisplayRequested);
    layout->addWidget(m_resetDisplayBtn);
    layout->addStretch();

    m_widthSpin->installEventFilter(new WidthWheelFilter(m_widthSpin, m_widthStepSpin, this));

    updateDisplayControlsEnabled();
    onBppChanged();
}

// =========================================================================
//  Слоты
// =========================================================================

void ControlsPanel::onBppChanged()
{
    const int bpp = bppMode();
    refillPaletteList(bpp);
    updateBitSettingsEnabled(bpp);
    notifyChanged();
}

void ControlsPanel::onPaletteChanged()
{
    if (!m_main)
        return;
    if (m_updatingConfig)
        return;

    const QString name = m_paletteCombo->currentData().toString();
    m_main->onPaletteChanged(name);
    emit settingsChanged();
}

void ControlsPanel::onHexModeChanged(bool checked)
{
    applyHexModeToSpins(checked);
    notifyChanged();
}

void ControlsPanel::onLengthModeChanged(bool checked)
{
    if (m_endLabel)
        m_endLabel->setText(checked ? tr(CP_LABEL_LENGTH) : tr(CP_LABEL_END));
    notifyChanged();
}

void ControlsPanel::resetRegionToDefault()
{
    {
        QSignalBlocker b1(m_offsetSpin), b2(m_endSpin), b3(m_lengthModeCheck);
        m_offsetSpin->setValue64(0);
        m_endSpin->setValue64(0);
        m_lengthModeCheck->setChecked(false);
        if (m_endLabel)
            m_endLabel->setText(tr(CP_LABEL_END));
    }
    notifyChanged();
}

void ControlsPanel::startRegionPick(bool startBoundary)
{
    if (!m_main || !m_main->canvasWidget()) {
        cancelRegionPick();
        return;
    }

    if (m_regionPickTarget != PickNone)
        cancelRegionPick();

    m_regionPickTarget = startBoundary ? PickStart : PickEnd;

    {
        QSignalBlocker b1(m_pickStartBtn), b2(m_pickEndBtn);
        if (startBoundary) {
            m_pickStartBtn->setChecked(true);
            m_pickEndBtn->setChecked(false);
        } else {
            m_pickStartBtn->setChecked(false);
            m_pickEndBtn->setChecked(true);
        }
    }

    m_main->canvasWidget()->setRegionPickMode(true);
}

void ControlsPanel::cancelRegionPick()
{
    if (m_regionPickTarget == PickNone) {
        if (m_main && m_main->canvasWidget())
            m_main->canvasWidget()->setRegionPickMode(false);
        if (m_pickStartBtn) {
            QSignalBlocker b(m_pickStartBtn);
            m_pickStartBtn->setChecked(false);
        }
        if (m_pickEndBtn) {
            QSignalBlocker b(m_pickEndBtn);
            m_pickEndBtn->setChecked(false);
        }
        return;
    }

    m_regionPickTarget = PickNone;

    if (m_main && m_main->canvasWidget())
        m_main->canvasWidget()->setRegionPickMode(false);

    if (m_pickStartBtn && m_pickStartBtn->isChecked()) {
        QSignalBlocker b(m_pickStartBtn);
        m_pickStartBtn->setChecked(false);
    }
    if (m_pickEndBtn && m_pickEndBtn->isChecked()) {
        QSignalBlocker b(m_pickEndBtn);
        m_pickEndBtn->setChecked(false);
    }
}

void ControlsPanel::onRegionAddressCaptured(qint64 address)
{
    const RegionPickTarget target = m_regionPickTarget;
    cancelRegionPick();

    if (m_main && m_main->canvasWidget())
        m_main->canvasWidget()->clearLock();

    if (target == PickNone)
        return;
    if (address < 0 || address > Int64SpinBox::maxAllowed())
        return;

    const qint64 maxVal = m_offsetSpin->maximum64();

    if (target == PickStart) {
        qint64 newStart = address;
        if (!m_lengthModeCheck->isChecked()) {
            const qint64 curEnd = m_endSpin->value64();
            if (curEnd != 0 && newStart >= curEnd)
                newStart = qMax<qint64>(0, curEnd - 1);
        }
        QSignalBlocker bo(m_offsetSpin);
        m_offsetSpin->setValue64(newStart);
    } else {
        if (m_lengthModeCheck->isChecked()) {
            QSignalBlocker b(m_lengthModeCheck);
            m_lengthModeCheck->setChecked(false);
            if (m_endLabel)
                m_endLabel->setText(tr(CP_LABEL_END));
        }

        qint64 newEnd = address;
        const qint64 curStart = m_offsetSpin->value64();
        if (newEnd <= curStart)
            newEnd = qMin(curStart + 1, maxVal);

        QSignalBlocker be(m_endSpin);
        m_endSpin->setValue64(newEnd);
    }

    notifyChanged();
}

// =========================================================================
//  Внутренние помощники
// =========================================================================

void ControlsPanel::fillPaletteCombo(int bpp)
{
    m_paletteCombo->clear();

    PaletteManager *pm = m_main ? m_main->paletteManager() : nullptr;
    if (!pm)
        return;

    for (const QString &id : pm->paletteIdsForBpp(bpp)) {
        const QString name = pm->displayName(id);
        const QString label = pm->isConfigurable(id)
            ? name + QStringLiteral(" *") : name;
        m_paletteCombo->addItem(label, id);
    }
}

void ControlsPanel::refillPaletteList(int bpp)
{
    QSignalBlocker b(m_paletteCombo);

    const QString current = m_paletteCombo->currentData().toString();
    fillPaletteCombo(bpp);
    selectByData(m_paletteCombo, current);
}

void ControlsPanel::notifyChanged()
{
    if (!m_updatingConfig)
        emit settingsChanged();
}

void ControlsPanel::watch(QSpinBox *spin)
{
    connect(spin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) { notifyChanged(); });
}

void ControlsPanel::watch(QComboBox *combo)
{
    connect(combo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) { notifyChanged(); });
}

void ControlsPanel::updateBitSettingsEnabled(int bpp)
{
    const bool bitpacked = (bpp >= 1 && bpp <= 4);
    const bool multibyte = (bpp >= 16);

    // Bit order виден всегда. Для 1–4 bpp — порядок извлечения битов,
    // для 8+ — bit-reversal в байте.
    if (m_bitOrderLabel)
        m_bitOrderLabel->setVisible(true);
    if (m_bitOrderCombo) {
        m_bitOrderCombo->setVisible(true);
        m_bitOrderCombo->setEnabled(true);
        m_bitOrderCombo->setToolTip(bitpacked
            ? tr("Bit extraction order for bit-packed pixels.\n"
                 "MSB-first: highest bit of byte first.\n"
                 "LSB-first: lowest bit of byte first.")
            : tr("Bit order within each byte.\n"
                 "MSB-first: normal byte.\n"
                 "LSB-first: reverse bits in byte."));
    }

    // Byte order — только для 16–64 bpp.
    if (m_byteOrderLabel)
        m_byteOrderLabel->setVisible(multibyte);
    if (m_byteOrderCombo) {
        m_byteOrderCombo->setVisible(multibyte);
        m_byteOrderCombo->setEnabled(multibyte);
    }

    // Pixel order и Padding — только для 1–4 bpp.
    if (m_pixelOrderLabel)
        m_pixelOrderLabel->setVisible(bitpacked);
    if (m_pixelOrderCombo) {
        m_pixelOrderCombo->setVisible(bitpacked);
        m_pixelOrderCombo->setEnabled(bitpacked);
    }
    if (m_rowPadLabel)
        m_rowPadLabel->setVisible(bitpacked);
    if (m_rowPadSpin) {
        m_rowPadSpin->setVisible(bitpacked);
        m_rowPadSpin->setEnabled(bitpacked);
    }
}

QString ControlsPanel::byteOrder() const
{
    if (!m_byteOrderCombo)
        return QStringLiteral(CFG_DEF_BYTE_ORDER);
    return m_byteOrderCombo->currentData().toString();
}

void ControlsPanel::updateDisplayControlsEnabled()
{
    if (!m_widthModeCombo)
        return;

    const int idx = m_widthModeCombo->currentIndex();

    const bool widthVisible = (idx == WM_Fixed || idx == WM_Divisor);
    const bool stepVisible  = (idx == WM_Fixed);
    const bool bprVisible   = (idx == WM_BytesPerRow);

    if (m_widthLabel)
        m_widthLabel->setVisible(widthVisible);
    if (m_widthSpin) {
        m_widthSpin->setVisible(widthVisible);
        m_widthSpin->setEnabled(widthVisible);
        m_widthSpin->setToolTip(widthVisible
            ? tr("For \"Manual\" — width in pixels.\n"
                 "For \"Divisor ≤ N\" — upper width limit.")
            : tr("Controlled by the current width mode."));
    }

    if (m_widthStepLabel)
        m_widthStepLabel->setVisible(stepVisible);
    if (m_widthStepSpin) {
        m_widthStepSpin->setVisible(stepVisible);
        m_widthStepSpin->setEnabled(stepVisible);
        m_widthStepSpin->setToolTip(stepVisible
            ? tr("Width change step with Ctrl+mouse wheel.\n"
                 "Used in manual mode.")
            : tr("Available only in \"Manual\" mode."));
    }

    if (m_bytesPerRowLabel)
        m_bytesPerRowLabel->setVisible(bprVisible);
    if (m_bytesPerRowSpin) {
        m_bytesPerRowSpin->setVisible(bprVisible);
        m_bytesPerRowSpin->setEnabled(bprVisible);
    }

    if (m_scaleSpin) {
        const bool locked = (m_autoScaleCheck && m_autoScaleCheck->isChecked())
                         || (m_fitCheck && m_fitCheck->isChecked());
        m_scaleSpin->setEnabled(!locked);
        m_scaleSpin->setToolTip(locked
            ? tr("Controlled by \"Auto zoom\" or \"Fit to window\".")
            : tr("Display zoom."));
    }
}

void ControlsPanel::applyHexModeToSpins(bool useHex)
{
    for (Int64SpinBox *spin : { m_offsetSpin, m_endSpin }) {
        if (!spin)
            continue;
        QSignalBlocker b(spin);
        spin->setHexMode(useHex);
    }
}

void ControlsPanel::clampRegionBounds(bool startChanged)
{
    if (m_updatingConfig)
        return;
    if (m_lengthModeCheck->isChecked())
        return;

    const qint64 maxVal   = m_offsetSpin->maximum64();
    const qint64 curStart = m_offsetSpin->value64();
    const qint64 curEnd   = m_endSpin->value64();

    QSignalBlocker bo(m_offsetSpin);
    QSignalBlocker be(m_endSpin);

    if (startChanged) {
        if (curEnd != 0 && curStart >= curEnd) {
            const qint64 newStart = qMax<qint64>(0, curEnd - 1);
            m_offsetSpin->setValue64(newStart);
        }
    } else {
        if (curEnd != 0 && curEnd <= curStart) {
            const qint64 newEnd = qMin(curStart + 1, maxVal);
            m_endSpin->setValue64(newEnd);
        }
    }
}

bool ControlsPanel::regionIsValid() const
{
    if (m_lengthModeCheck->isChecked())
        return m_endSpin->value64() > 0 || m_offsetSpin->value64() == 0;
    const qint64 end = m_endSpin->value64();
    if (end == 0)                       // «весь файл» — валидно
        return true;
    return end > m_offsetSpin->value64();
}

// =========================================================================
//  Геттеры текущих значений
// =========================================================================

int ControlsPanel::bppMode() const
{
    bool ok = false;
    const int bpp = m_bppCombo->currentData().toInt(&ok);
    return ok ? bpp : CFG_DEF_BPP_MODE;
}

QString ControlsPanel::currentPaletteId() const
{
    return m_paletteCombo->currentData().toString();
}

QString ControlsPanel::bitOrder() const
{
    return m_bitOrderCombo->currentData().toString();
}

QString ControlsPanel::pixelOrder() const
{
    return m_pixelOrderCombo->currentData().toString();
}

int ControlsPanel::rowPaddingBits() const
{
    return m_rowPadSpin->value();
}

int ControlsPanel::widthMode() const
{
    return m_widthModeCombo->currentIndex();
}

int ControlsPanel::fixedWidth() const
{
    return m_widthSpin->value();
}

int ControlsPanel::widthStep() const
{
    return m_widthStepSpin ? m_widthStepSpin->value() : CFG_DEF_WIDTH_STEP;
}

int ControlsPanel::scaleValue() const
{
    return m_scaleSpin->value();
}

bool ControlsPanel::autoScale() const
{
    return m_autoScaleCheck->isChecked();
}

bool ControlsPanel::fitToWindow() const
{
    return m_fitCheck->isChecked();
}

quint64 ControlsPanel::offset() const
{
    return quint64(m_offsetSpin->value64());
}

quint64 ControlsPanel::length() const
{
    const quint64 off = offset();
    const quint64 endOrLen = quint64(m_endSpin->value64());
    if (m_lengthModeCheck->isChecked())
        return endOrLen;
    return endOrLen > off ? (endOrLen - off) : 0;
}

bool ControlsPanel::lengthMode() const
{
    return m_lengthModeCheck->isChecked();
}

int ControlsPanel::bytesPerRow() const
{
    return m_bytesPerRowSpin ? m_bytesPerRowSpin->value() : CFG_DEF_BYTES_PER_ROW;
}

QVariantMap ControlsPanel::currentConfig() const
{
    QVariantMap cfg;
    cfg[QStringLiteral(CFG_BPP_MODE)]         = bppMode();
    cfg[QStringLiteral(CFG_PALETTE)]          = currentPaletteId();
    cfg[QStringLiteral(CFG_BIT_ORDER)]        = bitOrder();
    cfg[QStringLiteral(CFG_BYTE_ORDER)]       = byteOrder();
    cfg[QStringLiteral(CFG_PIXEL_ORDER)]      = pixelOrder();
    cfg[QStringLiteral(CFG_ROW_PADDING_BITS)] = rowPaddingBits();
    cfg[QStringLiteral(CFG_WIDTH_MODE)]       = widthModeKey(widthMode());
    cfg[QStringLiteral(CFG_FIXED_WIDTH)]      = fixedWidth();
    cfg[QStringLiteral(CFG_WIDTH_STEP)]       = widthStep();
    cfg[QStringLiteral(CFG_SCALE)]            = scaleValue();
    cfg[QStringLiteral(CFG_AUTO_SCALE)]       = autoScale();
    cfg[QStringLiteral(CFG_FIT_TO_WINDOW)]    = fitToWindow();
    cfg[QStringLiteral(CFG_OFFSET)]           = offset();
    cfg[QStringLiteral(CFG_LENGTH)]           = length();
    cfg[QStringLiteral(CFG_LENGTH_MODE)]      = lengthMode();
    cfg[QStringLiteral(CFG_BYTES_PER_ROW)]    = bytesPerRow();
    return cfg;
}

// =========================================================================
//  Silent-сеттеры (без emit)
// =========================================================================

void ControlsPanel::setCanvasWidget(CanvasWidget *canvas)
{
    if (!canvas)
        return;

    connect(canvas, &CanvasWidget::lockedChanged, this,
            [this](qint64 address) {
                if (m_regionPickTarget != PickNone && address >= 0)
                    onRegionAddressCaptured(address);
            });

    connect(canvas, &CanvasWidget::regionPickCancelled,
            this, &ControlsPanel::cancelRegionPick);
}

void ControlsPanel::setScaleSilent(int s)
{
    if (!m_scaleSpin)
        return;
    QSignalBlocker b(m_scaleSpin);
    m_scaleSpin->setValue(qBound(1, s, 32));
}

void ControlsPanel::setWidthModeSilent(int idx)
{
    if (!m_widthModeCombo)
        return;
    QSignalBlocker b(m_widthModeCombo);
    const int clamped = qBound(0, idx, int(WM_Count) - 1);
    m_widthModeCombo->setCurrentIndex(clamped);
    updateDisplayControlsEnabled();
}

void ControlsPanel::setWidthSilent(int w)
{
    if (!m_widthSpin)
        return;
    QSignalBlocker b(m_widthSpin);
    m_widthSpin->setValue(qMax(1, w));
}

// =========================================================================
//  setConfig — полная синхронизация UI с конфигом
// =========================================================================

void ControlsPanel::setConfig(const QVariantMap &cfg)
{
    m_updatingConfig = true;

    QSignalBlocker b1(m_bppCombo), b2(m_paletteCombo), b3(m_bitOrderCombo),
                   b4(m_pixelOrderCombo), b5(m_widthModeCombo), b6(m_rowPadSpin),
                   b7(m_widthSpin), b8(m_widthStepSpin), b9(m_scaleSpin),
                   b10(m_offsetSpin), b11(m_endSpin), b12(m_autoScaleCheck),
                   b13(m_fitCheck), b14(m_lengthModeCheck), b15(m_hexModeCheck),
                   b16(m_byteOrderCombo);

    const int bpp = cfg.value(QStringLiteral(CFG_BPP_MODE),
                              CFG_DEF_BPP_MODE).toInt();
    selectByData(m_bppCombo, bpp);

    // Список палитр для выбранного bpp
    fillPaletteCombo(bpp);

    QString paletteId = cfg.value(QStringLiteral(CFG_PALETTE)).toString();
    if (m_main && m_main->paletteManager() &&
        !m_main->paletteManager()->contains(paletteId))
        paletteId.clear();
    if (!selectByData(m_paletteCombo, paletteId) && m_paletteCombo->count() > 0)
        m_paletteCombo->setCurrentIndex(0);

    // Любое значение, кроме «msb»/«big», трактуется как противоположный порядок.
    selectByData(m_bitOrderCombo,
        cfg.value(QStringLiteral(CFG_BIT_ORDER),
                  QStringLiteral(CFG_DEF_BIT_ORDER)).toString()
            == QLatin1String("msb") ? QStringLiteral("msb") : QStringLiteral("lsb"));
    selectByData(m_byteOrderCombo,
        cfg.value(QStringLiteral(CFG_BYTE_ORDER),
                  QStringLiteral(CFG_DEF_BYTE_ORDER)).toString()
            == QLatin1String("little") ? QStringLiteral("little") : QStringLiteral("big"));
    selectByData(m_pixelOrderCombo,
        cfg.value(QStringLiteral(CFG_PIXEL_ORDER),
                  QStringLiteral(CFG_DEF_PIXEL_ORDER)).toString()
            == QLatin1String("msb") ? QStringLiteral("msb") : QStringLiteral("lsb"));

    m_rowPadSpin->setValue(
        cfg.value(QStringLiteral(CFG_ROW_PADDING_BITS), CFG_DEF_ROW_PADDING).toInt());
    updateBitSettingsEnabled(bpp);

    m_scaleSpin->setValue(
        qBound(1, cfg.value(QStringLiteral(CFG_SCALE), CFG_DEF_SCALE).toInt(), 32));
    m_autoScaleCheck->setChecked(
        cfg.value(QStringLiteral(CFG_AUTO_SCALE), false).toBool());
    m_fitCheck->setChecked(
        cfg.value(QStringLiteral(CFG_FIT_TO_WINDOW), false).toBool());
    if (m_autoScaleCheck->isChecked() && m_fitCheck->isChecked()) {
        QSignalBlocker b(m_autoScaleCheck);
        m_autoScaleCheck->setChecked(false);
    }

    const QString wmKey = cfg.value(QStringLiteral(CFG_WIDTH_MODE),
                                    QStringLiteral(CFG_DEF_WIDTH_MODE)).toString();
    m_widthModeCombo->setCurrentIndex(widthModeFromKey(wmKey));
    m_widthSpin->setValue(qMax(1,
        cfg.value(QStringLiteral(CFG_FIXED_WIDTH), CFG_DEF_FIXED_WIDTH).toInt()));
    if (m_widthStepSpin) {
        m_widthStepSpin->setValue(qMax(1,
            cfg.value(QStringLiteral(CFG_WIDTH_STEP), CFG_DEF_WIDTH_STEP).toInt()));
    }
    if (m_bytesPerRowSpin) {
        m_bytesPerRowSpin->setValue(qMax(1,
            cfg.value(QStringLiteral(CFG_BYTES_PER_ROW), CFG_DEF_BYTES_PER_ROW).toInt()));
    }

    updateDisplayControlsEnabled();

    const quint64 offsetVal =
        cfg.value(QStringLiteral(CFG_OFFSET)).toULongLong();
    const quint64 lengthVal =
        cfg.value(QStringLiteral(CFG_LENGTH)).toULongLong();
    const bool lenMode =
        cfg.value(QStringLiteral(CFG_LENGTH_MODE), false).toBool();

    m_lengthModeCheck->setChecked(lenMode);
    if (m_endLabel)
        m_endLabel->setText(lenMode ? tr(CP_LABEL_LENGTH) : tr(CP_LABEL_END));

    const quint64 maxSpinValue = quint64(m_endSpin->maximum64());
    const quint64 safeOffsetVal = qMin(offsetVal, maxSpinValue);

    quint64 displayVal = 0;
    if (lenMode) {
        displayVal = qMin(lengthVal, maxSpinValue);
    } else if (lengthVal > 0) {
        const quint64 safeLengthVal = qMin(lengthVal, maxSpinValue);
        displayVal = qMin(safeOffsetVal + safeLengthVal, maxSpinValue);
    }

    applyHexModeToSpins(m_hexModeCheck->isChecked());
    {
        QSignalBlocker bs1(m_offsetSpin), bs2(m_endSpin);

        m_offsetSpin->setValue64(qint64(safeOffsetVal));
        m_endSpin->setValue64(qint64(displayVal));

        if (!m_lengthModeCheck->isChecked()
                && m_endSpin->value64() != 0
                && m_endSpin->value64() <= m_offsetSpin->value64()) {
            const qint64 maxVal = m_offsetSpin->maximum64();
            m_endSpin->setValue64(qMin(m_offsetSpin->value64() + 1, maxVal));
        }
    }

    m_updatingConfig = false;
    emit settingsChanged();
}

void ControlsPanel::onWidthModeChanged()
{
    updateDisplayControlsEnabled();
    notifyChanged();
}
