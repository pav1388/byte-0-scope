// Byte-0-scope - binary data visualizer
// File: controlspanel.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "controlspanel.h"
#include "mainwindow.h"
#include "palettemanager.h"
#include "canvaswidget.h"

#include <QApplication>
#include <QMouseEvent>

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
#include <QMap>

// --- Настройки модуля ControlsPanel (префикс CP_) ---

// Геометрия панели и раскладки
#define CP_PANEL_FIXED_WIDTH        300
#define CP_LAYOUT_MARGIN            4
#define CP_LAYOUT_SPACING           6
#define CP_LAYOUT_COL_MIN_WIDTH     150
#define CP_BUTTON_COMPACT_WIDTH     30

// Индексы режимов ширины (совпадают с порядком в m_widthModeCombo)
#define CP_WM_INDEX_FIXED           0
#define CP_WM_INDEX_AUTO            1
#define CP_WM_INDEX_POWER           2
#define CP_WM_INDEX_ASPECT16_9      3
#define CP_WM_INDEX_ASPECT4_3       4
#define CP_WM_INDEX_BYTES           5
#define CP_WM_INDEX_NICE            6
#define CP_WM_INDEX_DIVISOR         7
#define CP_WM_COUNT                 8

// Строковые ключи режимов ширины (для QVariantMap)
#define CP_WM_KEY_FIXED             "fixed"
#define CP_WM_KEY_AUTO              "auto"
#define CP_WM_KEY_POWER             "power"
#define CP_WM_KEY_ASPECT16_9        "aspect16_9"
#define CP_WM_KEY_ASPECT4_3         "aspect4_3"
#define CP_WM_KEY_BYTES             "bytes"
#define CP_WM_KEY_NICE              "nice"
#define CP_WM_KEY_DIVISOR           "divisor"

// Ключи QVariantMap конфигурации
#define CP_KEY_BPP_MODE             "bpp_mode"
#define CP_KEY_PALETTE              "palette"
#define CP_KEY_BIT_ORDER            "bit_order"
#define CP_KEY_PIXEL_ORDER          "pixel_order"
#define CP_KEY_ROW_PADDING_BITS     "row_padding_bits"
#define CP_KEY_WIDTH_MODE           "width_mode"
#define CP_KEY_FIXED_WIDTH          "fixed_width"
#define CP_KEY_WIDTH_STEP           "width_step"
#define CP_KEY_SCALE                "scale"
#define CP_KEY_AUTO_SCALE           "auto_scale"
#define CP_KEY_FIT_TO_WINDOW        "fit_to_window"
#define CP_KEY_OFFSET               "offset"
#define CP_KEY_LENGTH               "length"
#define CP_KEY_LENGTH_MODE          "length_mode"
#define CP_KEY_BYTES_PER_ROW        "bytes_per_row"

// Метки, изменяемые в зависимости от режима
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
            const int delta = (we->angleDelta().y() > 0) ? +1 : -1;
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

int bppFromText(const QString &t)
{
    static const QMap<QString, int> kMap = {
        { QStringLiteral("1 bpp"),   1  },
        { QStringLiteral("2 bpp"),   2  },
        { QStringLiteral("3 bpp"),   3  },
        { QStringLiteral("4 bpp"),   4  },
        { QStringLiteral("8 bpp"),   8  },
        { QStringLiteral("16 bpp"), 16  },
        { QStringLiteral("24 bpp"), 24  },
        { QStringLiteral("32 bpp"), 32  },
        { QStringLiteral("64 bpp"), 64  },
    };
    return kMap.value(t, 8);
}

QString bppToText(int bpp)
{
    return QString::number(bpp) + QStringLiteral(" bpp");
}

QString widthModeToKey(int wmInt)
{
    switch (wmInt) {
    case CP_WM_INDEX_AUTO:       return QStringLiteral(CP_WM_KEY_AUTO);
    case CP_WM_INDEX_POWER:      return QStringLiteral(CP_WM_KEY_POWER);
    case CP_WM_INDEX_ASPECT16_9: return QStringLiteral(CP_WM_KEY_ASPECT16_9);
    case CP_WM_INDEX_ASPECT4_3:  return QStringLiteral(CP_WM_KEY_ASPECT4_3);
    case CP_WM_INDEX_BYTES:      return QStringLiteral(CP_WM_KEY_BYTES);
    case CP_WM_INDEX_NICE:       return QStringLiteral(CP_WM_KEY_NICE);
    case CP_WM_INDEX_DIVISOR:    return QStringLiteral(CP_WM_KEY_DIVISOR);
    case CP_WM_INDEX_FIXED:
    default:                     return QStringLiteral(CP_WM_KEY_FIXED);
    }
}

int widthModeFromKey(const QString &wm)
{
    if (wm == QLatin1String(CP_WM_KEY_AUTO))       return CP_WM_INDEX_AUTO;
    if (wm == QLatin1String(CP_WM_KEY_POWER))      return CP_WM_INDEX_POWER;
    if (wm == QLatin1String(CP_WM_KEY_ASPECT16_9)) return CP_WM_INDEX_ASPECT16_9;
    if (wm == QLatin1String(CP_WM_KEY_ASPECT4_3))  return CP_WM_INDEX_ASPECT4_3;
    if (wm == QLatin1String(CP_WM_KEY_BYTES))      return CP_WM_INDEX_BYTES;
    if (wm == QLatin1String(CP_WM_KEY_NICE))       return CP_WM_INDEX_NICE;
    if (wm == QLatin1String(CP_WM_KEY_DIVISOR))    return CP_WM_INDEX_DIVISOR;
    return CP_WM_INDEX_FIXED;
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
    m_bppCombo->addItems({
        QStringLiteral("64 bpp"), QStringLiteral("32 bpp"),
        QStringLiteral("24 bpp"), QStringLiteral("16 bpp"),
        QStringLiteral("8 bpp"),  QStringLiteral("4 bpp"),
        QStringLiteral("3 bpp"),  QStringLiteral("2 bpp"),
        QStringLiteral("1 bpp"),
    });
    m_bppCombo->setCurrentText(QStringLiteral("8 bpp"));
    connect(m_bppCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) { onBppChanged(); });
    fmtLay->addWidget(m_bppCombo, row++, 1);

	m_bitOrderLabel = new QLabel(tr("Bit order:"), this);
    fmtLay->addWidget(m_bitOrderLabel, row, 0);
    m_bitOrderCombo = new QComboBox(this);
    m_bitOrderCombo->setObjectName("bitOrderCombo");
    m_bitOrderCombo->addItems({
        QStringLiteral("MSB-first (BE)"),
        QStringLiteral("LSB-first (LE)"),
    });
    connect(m_bitOrderCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) {
                if (!m_updatingConfig)
                    emit settingsChanged();
            });
    fmtLay->addWidget(m_bitOrderCombo, row++, 1);
	
	m_byteOrderLabel = new QLabel(tr("Byte order:"), this);
    fmtLay->addWidget(m_byteOrderLabel, row, 0);
    m_byteOrderCombo = new QComboBox(this);
    m_byteOrderCombo->setObjectName("byteOrderCombo");
    m_byteOrderCombo->addItems({
        QStringLiteral("Big-endian (BE)"),
        QStringLiteral("Little-endian (LE)"),
    });
    m_byteOrderCombo->setToolTip(tr(
        "Byte order within multi-byte pixel values (16/24/32/64 bpp).\n"
        "For RGB/RGBA palettes it also reverses the channel order\n"
        "(e.g. RGB + little = BGR)."));
    connect(m_byteOrderCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) {
                if (!m_updatingConfig)
                    emit settingsChanged();
            });
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
    connect(m_rowPadSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) {
                if (!m_updatingConfig)
                    emit settingsChanged();
            });
    fmtLay->addWidget(m_rowPadSpin, row++, 1);

	m_pixelOrderLabel = new QLabel(tr("Pixel order:"), this);
    fmtLay->addWidget(m_pixelOrderLabel, row, 0);
    m_pixelOrderCombo = new QComboBox(this);
    m_pixelOrderCombo->setObjectName("pixelOrderCombo");
    m_pixelOrderCombo->addItems({
        QStringLiteral("MSB-first"),
        QStringLiteral("LSB-first"),
    });
    connect(m_pixelOrderCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) {
                if (!m_updatingConfig)
                    emit settingsChanged();
            });
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
    m_offsetSpin = new QSpinBox(this);
    m_offsetSpin->setObjectName("offsetSpin");
    m_offsetSpin->setRange(0, 0x7FFFFFFF);
    m_offsetSpin->setValue(0);
    m_offsetSpin->setGroupSeparatorShown(false);
    m_offsetSpin->setKeyboardTracking(false);
    m_offsetSpin->setToolTip(tr("Region start (default 0)"));
    connect(m_offsetSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) {
                if (!m_updatingConfig)
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
        if (m_regionPickTarget == 1)
            cancelRegionPick();
        else
            startRegionPick(true);
    });
    regLay->addWidget(m_pickStartBtn, 0, 3);

    m_endLabel = new QLabel(tr(CP_LABEL_END), this);
    regLay->addWidget(m_endLabel, 1, 0);
    m_endSpin = new QSpinBox(this);
    m_endSpin->setObjectName("endSpin");
    m_endSpin->setRange(0, 0x7FFFFFFF);
    m_endSpin->setValue(0);
    m_endSpin->setGroupSeparatorShown(false);
    m_endSpin->setKeyboardTracking(false);
    m_endSpin->setToolTip(tr("Region end, or length if length mode is enabled"));
    connect(m_endSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) {
                if (!m_updatingConfig)
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
        if (m_regionPickTarget == 2)
            cancelRegionPick();
        else
            startRegionPick(false);
    });
    regLay->addWidget(m_pickEndBtn, 1, 3);

    m_lengthModeCheck = new QCheckBox(tr("Length mode"), this);
    m_lengthModeCheck->setObjectName("lengthModeCheck");
    m_lengthModeCheck->setToolTip(
        tr("If checked, the 'End' field is interpreted as 'Length'"));
    connect(m_lengthModeCheck, &QCheckBox::toggled,
            this, &ControlsPanel::onLengthModeChanged);
    regLay->addWidget(m_lengthModeCheck, 2, 0, 1, 2);

    m_hexModeCheck = new QCheckBox(tr("hex"), this);
    m_hexModeCheck->setObjectName("hexModeCheck");
    m_hexModeCheck->setToolTip(tr("Use hexadecimal input (0x…)"));
    connect(m_hexModeCheck, &QCheckBox::toggled,
            this, &ControlsPanel::onHexModeChanged);
    regLay->addWidget(m_hexModeCheck, 2, 2, 1, 1);

    m_resetRegionBtn = new QPushButton(tr("X"), this);
    m_resetRegionBtn->setObjectName("resetRegionBtn");
    m_resetRegionBtn->setFixedWidth(CP_BUTTON_COMPACT_WIDTH);
    m_resetRegionBtn->setAutoDefault(false);
    m_resetRegionBtn->setToolTip(tr("Reset region: display all data"));
    connect(m_resetRegionBtn, &QPushButton::clicked,
            this, &ControlsPanel::resetRegionToDefault);
    regLay->addWidget(m_resetRegionBtn, 2, 3, 1, 1);

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
    m_widthModeCombo->setCurrentIndex(CP_WM_INDEX_FIXED);
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
    connect(m_bytesPerRowSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) {
                if (!m_updatingConfig)
                    emit settingsChanged();
            });
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
    connect(m_widthSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) {
                if (!m_updatingConfig)
                    emit settingsChanged();
            });
	dispLay->addWidget(m_widthSpin, dr++, 1);

	m_widthStepLabel = new QLabel(tr("Width step:"), this);
    dispLay->addWidget(m_widthStepLabel, dr, 0);
    m_widthStepSpin = new QSpinBox(this);
    m_widthStepSpin->setObjectName("widthStepSpin");
    m_widthStepSpin->setRange(1, 65536);
    m_widthStepSpin->setValue(8);
    m_widthStepSpin->setToolTip(tr("Width change step with Ctrl+mouse wheel.\n"
                                   "Used in manual mode."));
    connect(m_widthStepSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) {
                if (!m_updatingConfig)
                    emit settingsChanged();
            });
    dispLay->addWidget(m_widthStepSpin, dr++, 1);

    dispLay->addWidget(new QLabel(tr("Zoom:")), dr, 0);
    m_scaleSpin = new QSpinBox(this);
    m_scaleSpin->setObjectName("scaleSpin");
    m_scaleSpin->setRange(1, 32);
    m_scaleSpin->setValue(1);
    connect(m_scaleSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int) {
                if (!m_updatingConfig)
                    emit settingsChanged();
            });
    dispLay->addWidget(m_scaleSpin, dr++, 1);

    auto *checkboxRow = new QHBoxLayout();
    checkboxRow->setContentsMargins(0, 0, 0, 0);
    checkboxRow->setSpacing(10);

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
    checkboxRow->addWidget(m_autoScaleCheck);

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
    checkboxRow->addWidget(m_fitCheck);
    checkboxRow->addStretch();
    dispLay->addLayout(checkboxRow, dr++, 0, 1, 2);

    layout->addWidget(dispGroup);

    m_resetDisplayBtn = new QPushButton(tr("X Reset display settings"), this);
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
    if (!m_updatingConfig)
        emit settingsChanged();
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
    if (!m_updatingConfig)
        emit settingsChanged();
}

void ControlsPanel::onLengthModeChanged(bool checked)
{
    if (m_endLabel)
        m_endLabel->setText(checked ? tr(CP_LABEL_LENGTH) : tr(CP_LABEL_END));
    if (!m_updatingConfig)
        emit settingsChanged();
}

void ControlsPanel::resetRegionToDefault()
{
    {
        QSignalBlocker b1(m_offsetSpin), b2(m_endSpin), b3(m_lengthModeCheck);
        m_offsetSpin->setValue(0);
        m_endSpin->setValue(0);
        m_lengthModeCheck->setChecked(false);
        if (m_endLabel)
            m_endLabel->setText(tr(CP_LABEL_END));
    }
    if (!m_updatingConfig)
        emit settingsChanged();
}

void ControlsPanel::startRegionPick(bool startBoundary)
{
    if (!m_main || !m_main->canvasWidget()) {
        cancelRegionPick();
        return;
    }

    if (m_regionPickTarget != 0)
        cancelRegionPick();

    m_regionPickTarget = startBoundary ? 1 : 2;

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
    if (m_regionPickTarget == 0) {
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

    m_regionPickTarget = 0;

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
    const int target = m_regionPickTarget;
    cancelRegionPick();

    if (m_main && m_main->canvasWidget())
        m_main->canvasWidget()->clearLock();

    if (target != 1 && target != 2)
        return;
    if (address < 0 || address > 0x7FFFFFFF)
        return;

    if (target == 1) {
        m_offsetSpin->setValue(int(address));
    } else {
        const bool wasLengthMode = m_lengthModeCheck->isChecked();
        if (wasLengthMode) {
            QSignalBlocker b(m_lengthModeCheck);
            m_lengthModeCheck->setChecked(false);
            if (m_endLabel)
                m_endLabel->setText(tr(CP_LABEL_END));
        }
        m_endSpin->setValue(int(address));
    }
}

// =========================================================================
//  Внутренние помощники
// =========================================================================

void ControlsPanel::refillPaletteList(int bpp)
{
    QSignalBlocker b(m_paletteCombo);

    const QString current = m_paletteCombo->currentData().toString();

    m_paletteCombo->clear();
    if (m_main && m_main->paletteManager()) {
        for (const QString &id : m_main->paletteManager()->paletteIdsForBpp(bpp)) {
            const QString name = m_main->paletteManager()->displayName(id);
            const bool cfg = m_main->paletteManager()->isConfigurable(id);
            const QString label = cfg ? name + QStringLiteral(" *") : name;
            m_paletteCombo->addItem(label, id);
        }
    }

    int idx = -1;
    for (int i = 0; i < m_paletteCombo->count(); ++i) {
        if (m_paletteCombo->itemData(i).toString() == current) {
            idx = i;
            break;
        }
    }
    if (idx >= 0)
        m_paletteCombo->setCurrentIndex(idx);
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
        return QStringLiteral("big");
    return m_byteOrderCombo->currentText().startsWith(QStringLiteral("Little"))
        ? QStringLiteral("little")
        : QStringLiteral("big");
}

void ControlsPanel::updateDisplayControlsEnabled()
{
    if (!m_widthModeCombo)
        return;

    const int idx = m_widthModeCombo->currentIndex();

    const bool widthVisible = (idx == CP_WM_INDEX_FIXED ||
                               idx == CP_WM_INDEX_DIVISOR);
    const bool stepVisible  = (idx == CP_WM_INDEX_FIXED);
    const bool bprVisible   = (idx == CP_WM_INDEX_BYTES);

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
    for (QSpinBox *spin : { m_offsetSpin, m_endSpin }) {
        if (!spin)
            continue;
        QSignalBlocker b(spin);
        spin->setDisplayIntegerBase(useHex ? 16 : 10);
        spin->setPrefix(useHex ? QStringLiteral("0x") : QString());
        spin->setGroupSeparatorShown(false);
    }
}

// =========================================================================
//  Геттеры текущих значений
// =========================================================================

int ControlsPanel::bppMode() const
{
    return bppFromText(m_bppCombo->currentText());
}

QString ControlsPanel::currentPaletteId() const
{
    return m_paletteCombo->currentData().toString();
}

QString ControlsPanel::bitOrder() const
{
    return m_bitOrderCombo->currentText().startsWith(QStringLiteral("MSB"))
        ? QStringLiteral("msb") : QStringLiteral("lsb");
}

QString ControlsPanel::pixelOrder() const
{
    return m_pixelOrderCombo->currentText().startsWith(QStringLiteral("MSB"))
        ? QStringLiteral("msb") : QStringLiteral("lsb");
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
    return m_widthStepSpin ? m_widthStepSpin->value() : 8;
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
    return quint64(m_offsetSpin->value());
}

quint64 ControlsPanel::length() const
{
    const quint64 off = offset();
    const quint64 endOrLen = quint64(m_endSpin->value());
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
    return m_bytesPerRowSpin ? m_bytesPerRowSpin->value() : 16;
}

QVariantMap ControlsPanel::currentConfig() const
{
    QVariantMap cfg;
    cfg[QStringLiteral(CP_KEY_BPP_MODE)]         = bppMode();
    cfg[QStringLiteral(CP_KEY_PALETTE)]          = currentPaletteId();
    cfg[QStringLiteral(CP_KEY_BIT_ORDER)]        = bitOrder();
	cfg[QStringLiteral(CP_KEY_BYTE_ORDER)]       = byteOrder();
    cfg[QStringLiteral(CP_KEY_PIXEL_ORDER)]      = pixelOrder();
    cfg[QStringLiteral(CP_KEY_ROW_PADDING_BITS)] = rowPaddingBits();
    cfg[QStringLiteral(CP_KEY_WIDTH_MODE)]       = widthModeToKey(widthMode());
    cfg[QStringLiteral(CP_KEY_FIXED_WIDTH)]      = fixedWidth();
    cfg[QStringLiteral(CP_KEY_WIDTH_STEP)]       = widthStep();
    cfg[QStringLiteral(CP_KEY_SCALE)]            = scaleValue();
    cfg[QStringLiteral(CP_KEY_AUTO_SCALE)]       = autoScale();
    cfg[QStringLiteral(CP_KEY_FIT_TO_WINDOW)]    = fitToWindow();
    cfg[QStringLiteral(CP_KEY_OFFSET)]           = offset();
    cfg[QStringLiteral(CP_KEY_LENGTH)]           = length();
    cfg[QStringLiteral(CP_KEY_LENGTH_MODE)]      = lengthMode();
    cfg[QStringLiteral(CP_KEY_BYTES_PER_ROW)]    = bytesPerRow();
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
                if (m_regionPickTarget != 0 && address >= 0)
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
    const int clamped = qBound(0, idx, CP_WM_COUNT - 1);
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

    const int bpp = cfg.value(QStringLiteral(CP_KEY_BPP_MODE), 8).toInt();
    m_bppCombo->setCurrentText(bppToText(bpp));

    // Список палитр для выбранного bpp
    m_paletteCombo->clear();
    if (m_main && m_main->paletteManager()) {
        for (const QString &id : m_main->paletteManager()->paletteIdsForBpp(bpp)) {
            const QString name = m_main->paletteManager()->displayName(id);
            const bool c = m_main->paletteManager()->isConfigurable(id);
            const QString label = c ? name + QStringLiteral(" *") : name;
            m_paletteCombo->addItem(label, id);
        }
    }

    QString paletteId = cfg.value(QStringLiteral(CP_KEY_PALETTE)).toString();
    if (m_main && m_main->paletteManager() &&
        !m_main->paletteManager()->contains(paletteId))
        paletteId.clear();
    if (paletteId.isEmpty()) {
        paletteId = m_paletteCombo->count() > 0
            ? m_paletteCombo->itemData(0).toString()
            : QStringLiteral("gray");
    }

    int pidx = -1;
    for (int i = 0; i < m_paletteCombo->count(); ++i) {
        if (m_paletteCombo->itemData(i).toString() == paletteId) {
            pidx = i;
            break;
        }
    }
    if (pidx < 0 && m_paletteCombo->count() > 0)
        pidx = 0;
    if (pidx >= 0)
        m_paletteCombo->setCurrentIndex(pidx);

    m_bitOrderCombo->setCurrentText(
        cfg.value(QStringLiteral(CP_KEY_BIT_ORDER), QStringLiteral("msb"))
            .toString() == QLatin1String("msb")
            ? QStringLiteral("MSB-first (BE)")
            : QStringLiteral("LSB-first (LE)"));
	
	m_byteOrderCombo->setCurrentText(
        cfg.value(QStringLiteral(CP_KEY_BYTE_ORDER), QStringLiteral("big"))
            .toString() == QLatin1String("little")
            ? QStringLiteral("Little-endian (LE)")
            : QStringLiteral("Big-endian (BE)"));
			
    m_pixelOrderCombo->setCurrentText(
        cfg.value(QStringLiteral(CP_KEY_PIXEL_ORDER), QStringLiteral("msb"))
            .toString() == QLatin1String("msb")
            ? QStringLiteral("MSB-first")
            : QStringLiteral("LSB-first"));

    m_rowPadSpin->setValue(
        cfg.value(QStringLiteral(CP_KEY_ROW_PADDING_BITS), 0).toInt());
    updateBitSettingsEnabled(bpp);

    m_scaleSpin->setValue(
        qBound(1, cfg.value(QStringLiteral(CP_KEY_SCALE), 1).toInt(), 32));
    m_autoScaleCheck->setChecked(
        cfg.value(QStringLiteral(CP_KEY_AUTO_SCALE), false).toBool());
    m_fitCheck->setChecked(
        cfg.value(QStringLiteral(CP_KEY_FIT_TO_WINDOW), false).toBool());
    if (m_autoScaleCheck->isChecked() && m_fitCheck->isChecked()) {
        QSignalBlocker b(m_autoScaleCheck);
        m_autoScaleCheck->setChecked(false);
    }

    const QString wmKey = cfg.value(QStringLiteral(CP_KEY_WIDTH_MODE),
                                    QStringLiteral(CP_WM_KEY_FIXED)).toString();
    const int wmIdx = widthModeFromKey(wmKey);

    m_widthModeCombo->setCurrentIndex(wmIdx);
    m_widthSpin->setValue(
        qMax(1, cfg.value(QStringLiteral(CP_KEY_FIXED_WIDTH), 256).toInt()));
    if (m_widthStepSpin) {
        m_widthStepSpin->setValue(
            qMax(1, cfg.value(QStringLiteral(CP_KEY_WIDTH_STEP), 8).toInt()));
    }
    if (m_bytesPerRowSpin) {
        m_bytesPerRowSpin->setValue(
            qMax(1, cfg.value(QStringLiteral(CP_KEY_BYTES_PER_ROW), 16).toInt()));
    }
	
	updateDisplayControlsEnabled();

    const quint64 offsetVal =
        cfg.value(QStringLiteral(CP_KEY_OFFSET)).toULongLong();
    const quint64 lengthVal =
        cfg.value(QStringLiteral(CP_KEY_LENGTH)).toULongLong();
    const bool lenMode =
        cfg.value(QStringLiteral(CP_KEY_LENGTH_MODE), false).toBool();

    m_lengthModeCheck->setChecked(lenMode);
    if (m_endLabel)
        m_endLabel->setText(lenMode ? tr(CP_LABEL_LENGTH) : tr(CP_LABEL_END));

    const quint64 displayVal = lenMode ? lengthVal : (offsetVal + lengthVal);

    applyHexModeToSpins(m_hexModeCheck->isChecked());
    {
        QSignalBlocker bs1(m_offsetSpin), bs2(m_endSpin);
        m_offsetSpin->setValue(int(qMin<quint64>(offsetVal,  0x7FFFFFFFULL)));
        m_endSpin->setValue   (int(qMin<quint64>(displayVal, 0x7FFFFFFFULL)));
    }

    m_updatingConfig = false;
    emit settingsChanged();
}

void ControlsPanel::onWidthModeChanged()
{
    updateDisplayControlsEnabled();
    if (!m_updatingConfig)
        emit settingsChanged();
}