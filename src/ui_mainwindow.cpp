// Byte-0-scope - binary data visualizer
// File: ui_mainwindow.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_mainwindow.h"
#include "qt_compat.h"
#include "app_fileassoc.h"
#include "app_settingsmanager.h"
#include "app_thememanager.h"
#include "core_demobuilder.h"
#include "core_pixelmath.h"
#include "definitions.h"
#include "pal_palettemanager.h"
#include "pal_palettesettings.h"
#include "render_canvaswidget.h"
#include "render_tilemanager.h"
#include "ui_aboutdialog.h"
#include "ui_byteconverter.h"
#include "ui_byteinfo.h"
#include "ui_controlspanel.h"
#include "ui_hexviewwidget.h"
#include "ui_mainwindowconfig.h"
#include "ui_panorama.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QResizeEvent>
#include <QSaveFile>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QStatusBar>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QVBoxLayout>
#include <QtMath>
#include <cmath>
#include <cstring>
#include <limits>

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <psapi.h>
#else
#  include <unistd.h>
#endif

// --- Module settings (prefix MW_) ---

#define MW_PANEL_WIDTH              262
#define MW_LAYOUT_SPACING           2
#define MW_DEFAULT_WINDOW_W         1000
#define MW_DEFAULT_WINDOW_H         720
#define MW_LAYOUT_CENTER_MARGIN     5
#define MW_MEMORY_TIMER_MS          2000
#define MW_RESIZE_DEBOUNCE_MS       50
#define MW_PERF_DEBOUNCE_MS         1000
#define MW_CACHE_MAX_MB_64          1000
#define MW_CACHE_MAX_MB_32          400
#define MW_SHORTEN_NAME_MAX         30
#define MW_BYTES_PER_KB             1024LL
#define MW_BYTES_PER_MB             (1024.0 * 1024.0)
#define MW_MEMORY_RESERVE_MIN       (512LL * 1024 * 1024)
#define MW_MEMORY_RESERVE_FRACTION  4
#define MW_MAX_SCALE                32

static int calcPowerOfTwoWidth(int pixelsCount, int maxWidth = 4096)
{
    int w = 1;
    while (w * 2 <= pixelsCount && w < maxWidth)
        w *= 2;
    return w;
}

static int aspectWidth(int pixels, int rw, int rh)
{
    return qMax(1, int(qSqrt(double(pixels) * double(rw) / double(rh))));
}

static int nearestNiceWidth(int pixels)
{
    static const int kNice[] = {
        64, 128, 192, 256, 320, 384, 448, 512,
        640, 768, 800, 896, 1024, 1280, 1536, 1920, 2048
    };
    const int ideal = qMax(1, int(qSqrt(double(pixels))));
    int best = kNice[0];
    int bestDist = qAbs(ideal - best);
    for (int v : kNice) {
        const int d = qAbs(ideal - v);
        if (d < bestDist) {
            bestDist = d;
            best = v;
        }
    }
    return best;
}

static int largestDivisorLE(int n, int cap)
{
    if (n <= 0)
        return 1;

    const int limit = qBound(1, cap, 65536);
    for (int divisor = qMin(n, limit); divisor >= 1; --divisor) {
        if (n % divisor == 0)
            return divisor;
    }
    return 1;
}

static QString shortenName(const QString &name, int maxLen = MW_SHORTEN_NAME_MAX)
{
    if (name.size() <= maxLen)
        return name;

    const QString ext = QFileInfo(name).suffix();
    if (!ext.isEmpty() && ext.size() + 2 < maxLen) {
        const int headLen = maxLen - ext.size() - 2;
        return name.left(headLen) + QChar(0x2026) + ext;
    }
    return name.left(maxLen - 1) + QChar(0x2026);
}

static QString diffPercentText(qint64 diffBytes, qint64 totalBytes)
{
    if (diffBytes <= 0 || totalBytes <= 0)
        return QStringLiteral("0");
    const double percent = 100.0 * double(diffBytes) / double(totalBytes);
    if (percent < 0.01)
        return QStringLiteral("<0.01");
    return QString::number(percent, 'f', 2);
}

static bool paletteNeedsHistogram(const QString &id)
{
    return id == QLatin1String(PaletteIds::ByteFrequency)
        || id == QLatin1String(PaletteIds::Repeats);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);

    m_theme          = new ThemeManager();
    m_paletteManager = new PaletteManager();
    m_settings       = new SettingsManager();

    initDemo();
    loadSettings();
    buildMenus();
    buildUi();
    m_theme->apply(qApp);

    if (m_startupDefaults.hasAny) {
        if (m_controls)
            m_controls->setConfig(startupDefaultsToConfig(m_startupDefaults));
        const QString paletteId = m_controls
            ? m_controls->currentPaletteId()
            : QStringLiteral(CFG_DEF_PALETTE);
        onPaletteChanged(paletteId.isEmpty()
                             ? QStringLiteral(CFG_DEF_PALETTE)
                             : paletteId);
    } else {
        applyDefaultConfig();
    }

    if (m_byteInfo && m_byteTooltipAct)
        m_byteInfo->setPopupEnabled(m_startupDefaults.byteTooltip);

    recomputeGeometry();
    refreshTitle();
    resize(MW_DEFAULT_WINDOW_W, MW_DEFAULT_WINDOW_H);
    restoreWindowGeometry();

    m_memoryTimer = new QTimer(this);
    connect(m_memoryTimer, &QTimer::timeout, this, &MainWindow::updateMemoryLabel);
    m_memoryTimer->start(MW_MEMORY_TIMER_MS);
    updateMemoryLabel();

    m_resizeTimer = new QTimer(this);
    m_resizeTimer->setSingleShot(true);
    connect(m_resizeTimer, &QTimer::timeout, this, [this]() {
        if (m_fitToWindow)
            applyFitToWindow();
        else if (m_autoScale)
            autoFitScale();
    });

    m_perfDebounceTimer = new QTimer(this);
    m_perfDebounceTimer->setSingleShot(true);
    connect(m_perfDebounceTimer, &QTimer::timeout,
            this, &MainWindow::applyPerfSettings);

    QTimer::singleShot(0, this, [this]() {
        if (m_canvas)
            m_canvas->rebuild();
    });
}

MainWindow::~MainWindow()
{
    delete m_theme;
    delete m_paletteManager;
    delete m_settings;
    m_theme = nullptr;
    m_paletteManager = nullptr;
    m_settings = nullptr;
}

bool MainWindow::isBitpacked() const
{
    return m_bppMode >= 1 && m_bppMode <= 4;
}

int MainWindow::bytesPerPixel() const
{
    if (isBitpacked())   return 1;
    if (m_bppMode >= 64) return 8;
    if (m_bppMode >= 32) return 4;
    if (m_bppMode >= 24) return 3;
    if (m_bppMode >= 16) return 2;
    return 1;
}

quint8 MainWindow::readByte(qint64 off) const
{
    return m_source.at(off);
}

QByteArray MainWindow::readRange(qint64 off, qint64 len) const
{
    return m_source.mid(off, len);
}

void MainWindow::initDemo()
{
    const QByteArray data = DemoBuilder::buildDefaultDemo();
    m_source.setData(data);
    m_fileSize = data.size();
}

void MainWindow::buildMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));

    QAction *openAct = fileMenu->addAction(tr("Open…"));
    openAct->setShortcut(QKeySequence("Ctrl+O"));
    connect(openAct, &QAction::triggered, this, &MainWindow::openFileDialog);

    m_recentMenu = fileMenu->addMenu(tr("Recent files"));
    connect(m_recentMenu, &QMenu::aboutToShow, this, &MainWindow::rebuildRecentMenu);

    fileMenu->addSeparator();

    QAction *cmpAct = fileMenu->addAction(
        tr("Compare two files (XOR) and show differences…"));
    cmpAct->setShortcut(QKeySequence("Ctrl+X"));
    connect(cmpAct, &QAction::triggered, this, &MainWindow::compareFiles);

    QAction *saveCfgAct = fileMenu->addAction(
        tr("Save .b0s settings for file"));
    saveCfgAct->setShortcut(QKeySequence("Ctrl+S"));
    connect(saveCfgAct, &QAction::triggered, this, &MainWindow::saveCurrentConfig);

    QAction *loadCfgAct = fileMenu->addAction(
        tr("Load .b0s settings for file"));
    loadCfgAct->setShortcut(QKeySequence("Ctrl+L"));
    connect(loadCfgAct, &QAction::triggered, this, &MainWindow::loadConfigFromFile);

    m_exportMenu = fileMenu->addMenu(tr("Export to PNG"));
    QAction *exportVisibleAct = m_exportMenu->addAction(
        tr("Current visible area…"));
    connect(exportVisibleAct, &QAction::triggered, this, [this]() {
        exportPng(ExportVisibleArea);
    });

    QAction *exportOriginalAct = m_exportMenu->addAction(
        tr("Entire image (1:1)…"));
    exportOriginalAct->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportOriginalAct, &QAction::triggered, this, [this]() {
        exportPng(ExportFullOriginal);
    });

    QAction *exportCurrentAct = m_exportMenu->addAction(
        tr("Entire image (current scale)…"));
    connect(exportCurrentAct, &QAction::triggered, this, [this]() {
        exportPng(ExportFullCurrentScale);
    });

    fileMenu->addSeparator();

    QAction *closeFileAct = fileMenu->addAction(tr("Close current file"));
    closeFileAct->setShortcut(QKeySequence("Ctrl+W"));
    connect(closeFileAct, &QAction::triggered, this, &MainWindow::closeCurrentFile);

    QAction *quitAct = fileMenu->addAction(tr("Exit"));
    quitAct->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAct, &QAction::triggered, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu(tr("View"));
	
    QAction *resetCfgAct = viewMenu->addAction(tr("Reset display settings"));
    resetCfgAct->setShortcut(QKeySequence("Ctrl+R"));
    connect(resetCfgAct, &QAction::triggered, this, &MainWindow::applyDefaultConfig);
	viewMenu->addSeparator();
	
    m_mirrorHAct = viewMenu->addAction(tr("Flip horizontally"));
    m_mirrorHAct->setCheckable(true);
    m_mirrorHAct->setChecked(m_mirrorH);
    connect(m_mirrorHAct, &QAction::toggled, this, [this](bool checked) {
        m_mirrorH = checked;
        onMirrorChanged();
    });
    m_mirrorVAct = viewMenu->addAction(tr("Flip vertically"));
    m_mirrorVAct->setCheckable(true);
    m_mirrorVAct->setChecked(m_mirrorV);
    connect(m_mirrorVAct, &QAction::toggled, this, [this](bool checked) {
        m_mirrorV = checked;
        onMirrorChanged();
    });
    viewMenu->addAction(tr("Reset flips"), this, &MainWindow::resetMirror);
	viewMenu->addSeparator();
	m_hexViewAct = viewMenu->addAction(tr("Hex view"));
	m_hexViewAct->setCheckable(true);
	m_hexViewAct->setChecked(false);
	connect(m_hexViewAct, &QAction::toggled, this, &MainWindow::onHexViewToggled);

    m_settingsMenu = menuBar()->addMenu(tr("Settings"));

    m_themeAction = m_settingsMenu->addAction(
        m_theme->isDark() ? tr("Light theme") : tr("Dark theme"));
    connect(m_themeAction, &QAction::triggered, this, &MainWindow::toggleTheme);
    m_settingsMenu->addSeparator();

    QMenu *langMenu = m_settingsMenu->addMenu(tr("Language"));

    auto *langGroup = new QActionGroup(this);
    langGroup->setExclusive(true);

    QAction *langEn = langMenu->addAction(tr("English"));
    langEn->setCheckable(true);
    langEn->setData(QString::fromLatin1(AppLimits::LangEn));
    langGroup->addAction(langEn);

    QAction *langRu = langMenu->addAction(tr("Russian"));
    langRu->setCheckable(true);
    langRu->setData(QString::fromLatin1(AppLimits::LangRu));
    langGroup->addAction(langRu);

    if (m_language == QLatin1String(AppLimits::LangRu))
        langRu->setChecked(true);
    else
        langEn->setChecked(true);

    connect(langGroup, &QActionGroup::triggered,
            this, &MainWindow::onLanguageChanged);
    m_settingsMenu->addSeparator();

    m_byteTooltipAct = m_settingsMenu->addAction(tr("Byte tooltip"));
    m_byteTooltipAct->setCheckable(true);
    m_byteTooltipAct->setChecked(m_startupDefaults.byteTooltip);
    connect(m_byteTooltipAct, &QAction::toggled, this, [this](bool checked) {
        if (m_byteInfo)
            m_byteInfo->setPopupEnabled(checked);
    });

    m_settingsMenu->addSeparator();

    m_loadIntoMemoryAct = m_settingsMenu->addAction(
        tr("Load the entire file into memory at once"));
    m_loadIntoMemoryAct->setCheckable(true);
    m_loadIntoMemoryAct->setChecked(m_loadIntoMemory);
    m_loadIntoMemoryAct->setToolTip(tr(
        "Off (default) — the file is mapped into memory via mmap.\n"
        "Suitable for large files: the OS loads pages on demand,\n"
        "RAM is not allocated in full.\n\n"
        "On — the file is read into a single QByteArray.\n"
        "Faster, but requires a contiguous RAM block the size of the file.\n"
        "On 32-bit builds files >1.5 GB may not fit.\n\n"
        "Applied the next time a file is opened."));
    connect(m_loadIntoMemoryAct, &QAction::toggled, this, [this](bool checked) {
        m_loadIntoMemory = checked;
        if (m_source.isOpen() && !m_source.sourcePath().isEmpty()) {
            statusBar()->showMessage(
                tr("Load mode changed. It will take effect the next time "
                   "a file is opened."), 5000);
        }
    });

    m_settingsMenu->addSeparator();
    m_settingsMenu->addAction(tr("Associate .b0s with this program"),
        this, [this]() {
            FileAssoc::registerFileAssociation(".b0s", this);
        });

    m_settingsMenu->addAction(tr("Remove .b0s association"),
        this, [this]() {
            FileAssoc::unregisterFileAssociation(".b0s", this);
        });
    m_settingsMenu->addSeparator();

    m_debugInfoAct = m_settingsMenu->addAction(
        tr("Debug information (right panel)"));
    m_debugInfoAct->setCheckable(true);
    m_debugInfoAct->setChecked(m_showDebugInfo);
    connect(m_debugInfoAct, &QAction::toggled,
            this, &MainWindow::onDebugInfoToggled);

    m_settingsMenu->addAction(tr("Open settings folder"),
        this, [this]() {
            if (!m_settings)
                return;
            const QString dir = QFileInfo(m_settings->configPath()).absolutePath();
            if (!QDir(dir).exists())
                QDir().mkpath(dir);
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
        });

    QMenu *helpMenu = menuBar()->addMenu(tr("Help"));
    QAction *aboutAct = helpMenu->addAction(tr("About…"));
    connect(aboutAct, &QAction::triggered, this, [this]() { showAboutDialog(this); });
}

void MainWindow::rebuildRecentMenu()
{
    m_recentMenu->clear();
    if (m_recentFiles.isEmpty()) {
        QAction *empty = m_recentMenu->addAction(tr("(no recent files)"));
        empty->setEnabled(false);
        return;
    }
    const int shown = qMin(m_recentFiles.size(), AppLimits::RecentMax);
    for (int i = 0; i < shown; ++i) {
        const QString path = m_recentFiles[i];
        QAction *a = m_recentMenu->addAction(
            QString("%1. %2").arg(i + 1).arg(QFileInfo(path).fileName()));
        a->setToolTip(path);
        connect(a, &QAction::triggered, this, [this, path]() { loadFile(path); });
    }
    m_recentMenu->addSeparator();
    connect(m_recentMenu->addAction(tr("Clear list")),
            &QAction::triggered, this, &MainWindow::clearRecent);
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *lay = new QHBoxLayout(central);
    lay->setContentsMargins(0, MW_LAYOUT_SPACING, MW_LAYOUT_SPACING, 0);
    lay->setSpacing(MW_LAYOUT_SPACING);

    auto *leftScroll = new QScrollArea(this);
    leftScroll->setWidgetResizable(true);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setFixedWidth(MW_PANEL_WIDTH);
    m_controls = new ControlsPanel(this);
    connect(m_controls, &ControlsPanel::settingsChanged,
            this, &MainWindow::onSettingsChanged);
    connect(m_controls, &ControlsPanel::resetDisplayRequested,
            this, &MainWindow::resetDisplaySettings);
    leftScroll->setWidget(m_controls);
    lay->addWidget(leftScroll);

    auto *centerWrapper = new QWidget(this);
    auto *centerLay = new QVBoxLayout(centerWrapper);
    centerLay->setContentsMargins(0, 0, 0, 0);
    centerLay->setSpacing(0);

    m_centerScroll = new QScrollArea(this);
    m_centerScroll->setWidgetResizable(false);
    m_canvas = new CanvasWidget(this);
    m_centerScroll->setWidget(m_canvas);
    connect(m_canvas, &CanvasWidget::hoverChanged,
            this, &MainWindow::onHoverChanged);
    connect(m_canvas, &CanvasWidget::lockedChanged,
            this, &MainWindow::onLockedChanged);
    connect(m_canvas, &CanvasWidget::scrollRequested,
            this, &MainWindow::onScrollRequested);
    connect(m_canvas, &CanvasWidget::scaleDeltaRequested,
            this, &MainWindow::onScaleDelta);
    connect(m_canvas, &CanvasWidget::widthStepRequested,
            this, &MainWindow::onWidthStep);
    if (m_controls)
        m_controls->setCanvasWidget(m_canvas);
    centerLay->addWidget(m_centerScroll, 1);
	
	m_hexView = new HexViewWidget(this);
	m_hexView->setVisible(false);
	if (m_hexBytesPerLine != 16)
		m_hexView->setBytesPerLine(m_hexBytesPerLine);
	if (m_hexFontPt != 9)
		m_hexView->setFontPt(m_hexFontPt);
	if (m_hexViewHeight > 0)
		m_hexView->setFixedHeight(m_hexViewHeight);

	connect(m_hexView, &HexViewWidget::heightChanged, this,
			[this](int h) { m_hexViewHeight = h; });

	connect(m_hexView, &HexViewWidget::byteSelected, this,
			[this](qint64 addr, int bitOffset) {
				if (!m_canvas)
					return;
				m_canvas->setLockedAddress(addr, bitOffset);
				if (m_canvas->hasLockedPixel())
					centerViewportOnSourcePixel(
						m_canvas->lockedSourcePixelPosition());
			});
	centerLay->addWidget(m_hexView, 0);

    m_hintContainer = new QWidget(this);
    auto *hintLay = new QVBoxLayout(m_hintContainer);
    hintLay->setContentsMargins(MW_LAYOUT_CENTER_MARGIN, MW_LAYOUT_CENTER_MARGIN,
                                MW_LAYOUT_CENTER_MARGIN, MW_LAYOUT_CENTER_MARGIN);
    m_hintLabel = new QLabel(
        tr("<b>Byte-0-scope - Interactive binary data visualizer</b>"),
        this);
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_hintLabel->setTextFormat(Qt::RichText);
    m_hintLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    hintLay->addWidget(m_hintLabel);

    m_trainingBtn = new QPushButton(tr("# Tutorial on a demo file"), this);
    m_trainingBtn->setAutoDefault(false);
    connect(m_trainingBtn, &QPushButton::clicked,
            this, &MainWindow::onTrainingButtonClicked);
    hintLay->addWidget(m_trainingBtn, 0, Qt::AlignLeft);

    hintLay->addStretch();
    centerLay->addWidget(m_hintContainer, 0);

    lay->addWidget(centerWrapper, 1);

    auto *rightScroll = new QScrollArea(this);
    rightScroll->setWidgetResizable(true);
    rightScroll->setFrameShape(QFrame::NoFrame);
    rightScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rightScroll->setFixedWidth(MW_PANEL_WIDTH);

    auto *rightInner = new QWidget(this);
    auto *rightLay = new QVBoxLayout(rightInner);
    rightLay->setContentsMargins(4, 4, 4, 4);
    rightLay->setSpacing(6);

    m_byteInfo = new ByteInfoWidget(this);
    rightLay->addWidget(m_byteInfo, 0, Qt::AlignTop);

    m_paletteSettings = new PaletteSettingsPanel(this);
    connect(m_paletteSettings, &PaletteSettingsPanel::paramsChanged,
            this, &MainWindow::onPaletteParamsChanged);
    connect(m_paletteSettings, &PaletteSettingsPanel::autoApplyToggled,
            this, &MainWindow::onPaletteAutoApplyToggled);
    rightLay->addWidget(m_paletteSettings, 0);

    auto *panGroup = new QGroupBox(tr("Panorama"), this);
    auto *panLay = new QVBoxLayout(panGroup);
    m_panorama = new PanoramaWidget(this);
    connect(m_panorama, &PanoramaWidget::scrollToRequested,
            this, &MainWindow::onScrollTo);
    panLay->addWidget(m_panorama);
    rightLay->addWidget(panGroup);
	
    auto *convGroup = new QGroupBox(tr("Byte converter"), this);
    auto *convLay = new QVBoxLayout(convGroup);
    convLay->setContentsMargins(6, 6, 6, 6);
    m_byteConverter = new ByteConverterWidget(this);
    convLay->addWidget(m_byteConverter);
    rightLay->addWidget(convGroup);

    auto *perfGroup = new QGroupBox(tr("RAM usage"), this);
    auto *perfLay = new QVBoxLayout(perfGroup);
    m_memoryLabel = new QLabel(
        tr("· Process memory: — MB\n· Tile cache: — MB"), this);
    perfLay->addWidget(m_memoryLabel);

    auto *sldRow = new QHBoxLayout();

    auto *lblMinus = new QLabel(tr("-"), this);
    lblMinus->setAlignment(Qt::AlignRight);
    lblMinus->setToolTip(tr("256 px per tile · cache limit 180 MB"));
    sldRow->addWidget(lblMinus);

    m_perfSlider = new QSlider(Qt::Horizontal, this);
    m_perfSlider->setRange(1, 20);
    m_perfSlider->setValue(m_perfSliderValue);
    m_perfSlider->setToolTip(tr(
        "Small tiles — more frequent updates, smaller pause per tile,\n"
        "but more overhead and more objects in the cache.\n\n"
        "Large tiles — less overhead,\n"
        "but each tile renders longer (in the background).\n\n"
        "In \"Fit to window\" mode tiles are stored downscaled\n"
        "(according to the fit scale), so the cache uses\n"
        "little memory even for gigabyte-sized files.\n\n"
        "Cache limit — target for invisible tiles. Visible tiles are protected\n"
        "from eviction, so momentarily the cache may exceed the limit."));
    connect(m_perfSlider, &QSlider::valueChanged,
            this, &MainWindow::onPerfSliderMoved);
    sldRow->addWidget(m_perfSlider);

    auto *lblPlus = new QLabel(tr("+"), this);
    lblPlus->setToolTip(tr("2048 px per tile · cache limit 1 GB"));
    sldRow->addWidget(lblPlus);
    perfLay->addLayout(sldRow);

    m_busyLabel = new QLabel(this);
    m_busyLabel->setStyleSheet("font-weight: bold;");
    m_busyLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    perfLay->addWidget(m_busyLabel);
    rightLay->addWidget(perfGroup);

    m_debugGroup = new QGroupBox(tr("Debug"), this);
    auto *dbgLay = new QVBoxLayout(m_debugGroup);
    m_debugLabel = new QLabel(this);
    QFont debugFont(QStringLiteral("Monospace"));
	debugFont.setStyleHint(QFont::TypeWriter);
	debugFont.setPointSize(8);
	m_debugLabel->setFont(debugFont);
    m_debugLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_debugLabel->setWordWrap(true);
    m_debugLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_debugLabel->setText(tr("(enable in Settings -> Debug information)"));
    dbgLay->addWidget(m_debugLabel);
    m_debugGroup->setVisible(m_showDebugInfo);
    rightLay->addWidget(m_debugGroup);

    rightLay->addStretch();
    rightScroll->setWidget(rightInner);
    lay->addWidget(rightScroll);

    if (m_canvas && m_canvas->tileManager()) {
        connect(m_canvas->tileManager(), &TileManager::activeTasksChanged,
                this, &MainWindow::onActiveTasksChanged);
        connect(m_canvas->tileManager(), &TileManager::pngExportFinished,
                this, &MainWindow::onPngExportFinished);
    }
    applyPerfFromSlider(m_perfSliderValue);

    if (m_centerScroll && m_panorama) {
        QScrollBar *h = m_centerScroll->horizontalScrollBar();
        QScrollBar *v = m_centerScroll->verticalScrollBar();

        auto refreshPanorama = [this]() {
            if (m_panorama)
                m_panorama->updatePanorama();
        };

        connect(h, &QScrollBar::valueChanged, this, refreshPanorama);
        connect(v, &QScrollBar::valueChanged, this, refreshPanorama);
        connect(h, &QScrollBar::rangeChanged, this,
                [refreshPanorama](int, int) { refreshPanorama(); });
        connect(v, &QScrollBar::rangeChanged, this,
                [refreshPanorama](int, int) { refreshPanorama(); });
    }
	
	if (m_hexViewAct)
		m_hexViewAct->setChecked(m_hexViewVisible);
	
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::openFileDialog()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open file"), m_lastDir, tr("All files (*.*)"));
    if (!path.isEmpty())
        loadFile(path);
}

void MainWindow::loadFile(const QString &path)
{
    leaveTrainingMode();

    QString actualPath = path;
    QString cfgPath;

    if (actualPath.endsWith(QLatin1String(".b0s"), Qt::CaseInsensitive)) {
        cfgPath = actualPath;
        const QString dataPath = actualPath.left(actualPath.size() - 4);
        if (dataPath.isEmpty() || !QFile::exists(dataPath)) {
            QMessageBox::warning(this, tr(".b0s config"),
                tr("Data file for the config was not found:\n%1\n\nExpected: %2")
                    .arg(actualPath, dataPath));
            return;
        }
        actualPath = dataPath;
    } else {
        cfgPath = actualPath + QLatin1String(".b0s");
    }

    if (!m_source.open(actualPath, m_loadIntoMemory)) {
        const QString err = m_source.lastError();
        QMessageBox::critical(this, tr("Failed to open file"),
            err.isEmpty() ? tr("Reason unknown.") : err);
        resetToDemo();
        return;
    }

    m_fileSize = m_source.size();
    m_offset   = 0;
    m_length   = 0;
    m_xorLabel.clear();
    m_lastDir  = QFileInfo(actualPath).absolutePath();
    addRecent(actualPath);
    invalidateByteHistogram();
    m_paletteParams.remove(QString::fromLatin1(PaletteIds::Repeats));

    bool cfgLoaded = false;
    if (QFile::exists(cfgPath)) {
        QJsonObject cfg;
        QString error;
        if (readConfigFile(cfgPath, &cfg, &error))
            cfgLoaded = applyConfigFromJson(cfg);
        else
            qWarning("b0s: could not read %s: %s",
                     qPrintable(cfgPath), qPrintable(error));
    }
    if (!cfgLoaded)
        applyDefaultConfig();

    finishLoad();
}

void MainWindow::finishLoad()
{
    applySquareWidthHint();
    recomputeGeometry();
    refreshTitle();
    if (m_panorama)
        m_panorama->updatePanorama();
    if (m_hexView)
        m_hexView->refresh();
}

void MainWindow::leaveTrainingMode()
{
    m_trainingMode = false;
    if (m_trainingBtn)
        m_trainingBtn->setVisible(false);
}

void MainWindow::addRecent(const QString &path)
{
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > AppLimits::RecentMax)
        m_recentFiles.removeLast();
    m_recentFiles = SettingsManager::filterExisting(m_recentFiles,
                                                    AppLimits::RecentMax);
}

void MainWindow::clearRecent()
{
    m_recentFiles.clear();
}

void MainWindow::compareFiles()
{
    QFileDialog dlg(this, tr("File 1"));
    dlg.setDirectory(m_lastDir);
    if (!dlg.exec())
        return;
    const QString p1 = dlg.selectedFiles().value(0);
    if (p1.isEmpty())
        return;

    const QString p2 = QFileDialog::getOpenFileName(
        this, tr("File 2"), m_lastDir);
    if (p2.isEmpty())
        return;

    loadFileXor(p1, p2);
}

bool MainWindow::loadFileXor(const QString &p1, const QString &p2)
{
    leaveTrainingMode();

    QFile f1(p1), f2(p2);
    if (!f1.open(QIODevice::ReadOnly) || !f2.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to open one of the files for comparison."));
        return false;
    }

    const qint64 sizeA = f1.size();
    const qint64 sizeB = f2.size();
    const qint64 needed = sizeA + sizeB + qMin(sizeA, sizeB);

    const qint64 qbaMax = QtCompat::byteArrayMaxSize();

    if (needed > qbaMax) {
        QMessageBox::warning(this, tr("Files are too large"),
            tr("XOR requires ~%1 MB, but the maximum QByteArray is %2 MB.\n"
               "Build a 64-bit version or reduce the file sizes.")
                .arg(needed / MW_BYTES_PER_MB, 0, 'f', 1)
                .arg(qbaMax / MW_BYTES_PER_MB, 0, 'f', 1));
        return false;
    }

    const qint64 avail = BytesSource::availableMemory();
    if (avail > 0) {
        const qint64 reserve = qMax<qint64>(MW_MEMORY_RESERVE_MIN,
                                            avail / MW_MEMORY_RESERVE_FRACTION);
        if (needed + reserve > avail) {
            QMessageBox::warning(this, tr("Not enough memory"),
                tr("XOR requires ~%1 MB, but only %2 MB is available.")
                    .arg(needed / MW_BYTES_PER_MB, 0, 'f', 1)
                    .arg(avail  / MW_BYTES_PER_MB, 0, 'f', 1));
            return false;
        }
    }

    const QByteArray a = f1.readAll();
    const QByteArray b = f2.readAll();
    f1.close();
    f2.close();

    if (a.size() != sizeA || b.size() != sizeB) {
        QMessageBox::critical(this, tr("Read error"),
            tr("Failed to read the files completely."));
        return false;
    }

    const int n = qMin(a.size(), b.size());
    QByteArray xorData(n, 0);
    const uchar *pa = reinterpret_cast<const uchar *>(a.constData());
    const uchar *pb = reinterpret_cast<const uchar *>(b.constData());
    uchar *po = reinterpret_cast<uchar *>(xorData.data());
    qint64 diffBytes = 0;
    for (int i = 0; i < n; ++i) {
        const uchar x = uchar(pa[i] ^ pb[i]);
        po[i] = x;
        diffBytes += (x != 0) ? 1 : 0;
    }

    m_source.setData(xorData);
    m_fileSize = xorData.size();
    m_offset = 0;
    m_length = 0;
    m_xorLabel = QString("[XOR] %1 ^ %2")
        .arg(shortenName(QFileInfo(p1).fileName(), 22),
             shortenName(QFileInfo(p2).fileName(), 22));
    m_xorDiffBytes = diffBytes;
    m_lastDir = QFileInfo(p1).absolutePath();
    invalidateByteHistogram();
    m_paletteParams.remove(QString::fromLatin1(PaletteIds::Repeats));

    bool cfgLoaded = false;
    for (const QString &path : { p1, p2 }) {
        const QString cfgPath = path + QLatin1String(".b0s");
        QJsonObject cfg;
        if (!QFile::exists(cfgPath) || !readConfigFile(cfgPath, &cfg, nullptr))
            continue;
        cfgLoaded = applyConfigFromJson(cfg);
        if (cfgLoaded)
            break;
    }
    if (!cfgLoaded)
        applyDefaultConfig();

    finishLoad();

    if (sizeA != sizeB) {
        statusBar()->showMessage(
            tr("File sizes differ (%1 and %2 bytes): only the first %3 bytes are compared.")
                .arg(sizeA).arg(sizeB).arg(qint64(n)),
            7000);
    }

    if (diffBytes == 0) {
        if (sizeA == sizeB) {
            QMessageBox::information(this, tr("XOR comparison"),
                tr("The files are identical: there are no differences "
                   "(%1 bytes compared).").arg(qint64(n)));
        } else {
            QMessageBox::information(this, tr("XOR comparison"),
                tr("The first %1 bytes of the files are identical, but the file "
                   "sizes differ (%2 and %3 bytes).")
                    .arg(qint64(n)).arg(sizeA).arg(sizeB));
        }
    }
    return true;
}

void MainWindow::closeCurrentFile()
{
    resetToDemo();
}

void MainWindow::resetToDemo()
{
    m_source.close();
    initDemo();
    invalidateByteHistogram();
    m_paletteParams.remove(QString::fromLatin1(PaletteIds::Repeats));
    m_fileSize = m_source.size();
    m_offset = 0;
    m_length = 0;
    m_lengthMode = false;
    m_xorLabel.clear();
    m_currentPalette.clear();
    m_currentPaletteId.clear();
    m_paletteParams.clear();
    m_scale = 1;
    m_downsample = 1;

    m_trainingMode = false;
    if (m_trainingBtn)
        m_trainingBtn->setVisible(true);
    if (m_hintLabel) {
        m_hintLabel->setText(
            tr("<b>Byte-0-scope - Interactive binary data visualizer</b>"));
    }

    if (m_canvas)
        m_canvas->clearHover();
    if (m_byteInfo)
        m_byteInfo->updateInfo(-1);
	if (m_hexView)
        m_hexView->refresh();

    applyDefaultConfig();
}

void MainWindow::rebuildPalette(const QVariantMap &params)
{
    const bool wantsHistogram = paletteNeedsHistogram(m_currentPaletteId);
    if (wantsHistogram)
        ensureByteHistogram();
    else
        invalidateByteHistogram();

    m_currentPalette = m_paletteManager->createPalette(
        m_currentPaletteId, params, m_source.data(),
        wantsHistogram ? m_byteHistogram : nullptr);
}

void MainWindow::onPaletteChanged(const QString &id)
{
    if (!m_paletteManager || !m_paletteManager->contains(id))
        return;

    m_currentPaletteId = id;

    if (!m_paletteParams.contains(id))
        m_paletteParams[id] = m_paletteManager->defaultParams(id);

    if (id == QLatin1String(PaletteIds::FromFile)) {
        QVariantMap p = m_paletteParams.value(id).toMap();
        p[QStringLiteral("bpp")] = m_bppMode;
        m_paletteParams[id] = p;
    }

    rebuildPalette(m_paletteParams.value(id).toMap());

    if (m_paletteSettings)
        m_paletteSettings->buildFor(id);
    if (m_canvas)
        m_canvas->rebuild();
    emit paletteChanged();
}

void MainWindow::setPaletteParams(const QString &id, const QVariantMap &params)
{
    if (!m_paletteManager || !m_paletteManager->contains(id))
        return;
    m_paletteParams[id] = params;
}

void MainWindow::onPaletteParamsChanged(const QVariantMap &newParams)
{
    if (m_currentPaletteId.isEmpty())
        return;
    m_paletteParams[m_currentPaletteId] = newParams;

    if (!m_autoApplyPalette)
        return;

    rebuildPalette(newParams);
    if (m_canvas)
        m_canvas->rebuild();
    emit paletteChanged();
}

void MainWindow::onPaletteAutoApplyToggled(bool enabled)
{
    m_autoApplyPalette = enabled;
    if (!enabled)
        return;

    rebuildPalette(m_paletteParams.value(m_currentPaletteId).toMap());
    if (m_canvas)
        m_canvas->rebuild();
    emit paletteChanged();
}

void MainWindow::recomputeGeometry()
{
    calculateDimensions();
    if (m_canvas)
        m_canvas->rebuild();
    if (m_panorama)
        m_panorama->updatePanorama();
}

quint64 MainWindow::regionByteLength() const
{
    return PixelMath::effectiveRegionLength(
        quint64(qMax<qint64>(0, m_fileSize)), m_offset, m_length);
}

int MainWindow::regionPixelCount() const
{
    return PixelMath::pixelCountForBytes(regionByteLength(), isBitpacked(),
                                         m_bppMode, bytesPerPixel());
}

qint64 MainWindow::imageHeightFor(int width) const
{
    return PixelMath::imageHeightForBytes(
        regionByteLength(), width, isBitpacked(), m_bppMode,
        m_rowPaddingBits, bytesPerPixel());
}

bool MainWindow::computeImageSize(int *width, int *height) const
{
    const int w = calculateWidth();
    if (w <= 0)
        return false;

    const qint64 h = imageHeightFor(w);
    if (h <= 0 || h > std::numeric_limits<int>::max())
        return false;

    *width = w;
    *height = int(h);
    return true;
}

int MainWindow::calculateWidth() const
{
    if (regionByteLength() == 0)
        return 0;

    const int pixels = regionPixelCount();
    if (pixels <= 0)
        return 0;

    const int bp = qMax(1, bytesPerPixel());
    switch (m_widthMode) {
    case WM_Fixed:        return qMax(1, m_fixedWidth);
    case WM_Auto:         return qMax(1, int(qSqrt(double(pixels))));
    case WM_Power:        return calcPowerOfTwoWidth(pixels);
    case WM_Aspect16x9:   return aspectWidth(pixels, 16, 9);
    case WM_Aspect4x3:    return aspectWidth(pixels, 4, 3);
    case WM_BytesPerRow:  return qMax(1, m_bytesPerRow / bp);
    case WM_Nice:         return nearestNiceWidth(pixels);
    case WM_Divisor:      return largestDivisorLE(pixels, m_fixedWidth);
    }
    return qMax(1, m_fixedWidth);
}

void MainWindow::applySquareWidthHint()
{
    if (m_widthFromConfig)
        return;

    if (regionByteLength() == 0)
        return;

    const int pixels = regionPixelCount();
    if (pixels <= 0)
        return;

    const int squareW = qMax(1, int(qSqrt(double(pixels))));
    m_fixedWidth = squareW;
    if (m_controls)
        m_controls->setWidthSilent(squareW);
}

void MainWindow::calculateDimensions()
{
    m_imageWidth = m_imageHeight = 0;

    int w = 0;
    int h = 0;
    if (!computeImageSize(&w, &h))
        return;
    m_imageWidth = w;
    m_imageHeight = h;

    if (m_centerScroll) {
        const QSize vp = m_centerScroll->viewport()->size();
        m_viewportRect = QRect(
            m_centerScroll->horizontalScrollBar()->value() / qMax(1, m_scale),
            m_centerScroll->verticalScrollBar()->value()   / qMax(1, m_scale),
            vp.width()  / qMax(1, m_scale),
            vp.height() / qMax(1, m_scale));
    }
}

void MainWindow::changeScale(int delta)
{
    const int newScale = qBound(1, m_scale + delta, MW_MAX_SCALE);
    if (newScale == m_scale)
        return;
    m_scale = newScale;
    if (m_controls)
        m_controls->setScaleSilent(m_scale);
    if (m_canvas)
        m_canvas->rebuild();
    if (m_panorama)
        m_panorama->updatePanorama();
}

void MainWindow::changeDownsample(int ds)
{
    m_downsample = qMax(1, ds);
    if (m_canvas)
        m_canvas->rebuild();
}

void MainWindow::onSettingsChanged()
{
    if (!m_controls)
        return;
    const bool wasFitToWindow = m_fitToWindow;
    const int oldBpp = m_bppMode;
    m_bppMode = m_controls->bppMode();

    const QString fromFileKey = QString::fromLatin1(PaletteIds::FromFile);
    if (m_paletteParams.contains(fromFileKey)) {
        QVariantMap p = m_paletteParams.value(fromFileKey).toMap();
        p["bpp"] = m_bppMode;
        m_paletteParams[fromFileKey] = p;
    }

    m_bitOrder = m_controls->bitOrder();
    m_byteOrder = m_controls->byteOrder();
    m_pixelOrder = m_controls->pixelOrder();
    m_rowPaddingBits = m_controls->rowPaddingBits();
    m_widthMode = m_controls->widthMode();
    m_fixedWidth = m_controls->fixedWidth();
    m_widthStep = m_controls->widthStep();
    m_scale = m_controls->scaleValue();
    m_autoScale = m_controls->autoScale();
    m_fitToWindow = m_controls->fitToWindow();
    m_offset = m_controls->offset();
    m_length = m_controls->length();
    m_lengthMode = m_controls->lengthMode();
    m_bytesPerRow = m_controls->bytesPerRow();

    if (oldBpp != m_bppMode)
        onPaletteChanged(m_controls->currentPaletteId());

    if (m_fitToWindow) {
        applyFitToWindow();
    } else if (m_autoScale) {
        if (m_canvas && m_downsample != 1)
            m_downsample = 1;
        autoFitScale();
    } else {
        if (m_canvas && m_downsample != 1)
            m_downsample = 1;
    }

    recomputeGeometry();
    refreshTitle();

    if (wasFitToWindow && !m_fitToWindow && m_canvas
            && m_canvas->hasLockedPixel()) {
        const QPoint selectedPixel = m_canvas->lockedSourcePixelPosition();
        QTimer::singleShot(0, this, [this, selectedPixel]() {
            centerViewportOnSourcePixel(selectedPixel);
        });
    }
}

void MainWindow::onHoverChanged(qint64 addr)
{
    if (m_byteInfo)
        m_byteInfo->updateInfo(
            addr, m_canvas ? m_canvas->selectedBitOffset() : 0);
    m_hoverAddr = addr;
    if (m_panorama)
        m_panorama->update();
}

void MainWindow::onLockedChanged(qint64 addr)
{
    if (m_byteInfo && addr >= 0)
        m_byteInfo->updateInfo(
            addr, m_canvas ? m_canvas->selectedBitOffset() : 0);
    if (m_hexView) {
        if (addr >= 0)
            m_hexView->setSelectedAddress(
                addr, m_canvas ? m_canvas->selectedBitOffset() : 0);
        else
            m_hexView->clearSelection();
    }
}

void MainWindow::onScrollRequested(int dx, int dy)
{
    if (!m_centerScroll)
        return;
    auto *h = m_centerScroll->horizontalScrollBar();
    auto *v = m_centerScroll->verticalScrollBar();
    h->setValue(h->value() + dx);
    v->setValue(v->value() + dy);
    if (m_panorama)
        m_panorama->updatePanorama();
}

void MainWindow::onScrollTo(int x, int y)
{
    if (!m_centerScroll || !m_canvas)
        return;
    auto *h = m_centerScroll->horizontalScrollBar();
    auto *v = m_centerScroll->verticalScrollBar();
    const int maxH = qMax(0, m_canvas->width()  - m_centerScroll->viewport()->width());
    const int maxV = qMax(0, m_canvas->height() - m_centerScroll->viewport()->height());
    h->setValue(qMin(x, maxH));
    v->setValue(qMin(y, maxV));
    if (m_panorama)
        m_panorama->updatePanorama();
}

void MainWindow::onScaleDelta(int delta)
{
    if (m_fitToWindow)
        return;
    changeScale(delta);
}

void MainWindow::onWidthStep(int delta)
{
    if (!m_controls)
        return;
    const int step = qMax(1, m_widthStep);

    if (m_widthMode != WM_Fixed) {
        const int curW = calculateWidth();
        if (curW > 0)
            m_fixedWidth = curW;
        m_widthMode = WM_Fixed;
        m_controls->setWidthModeSilent(WM_Fixed);
        m_controls->setWidthSilent(m_fixedWidth);
    }

    const int steps = m_fixedWidth / step;
    m_fixedWidth = qMax(1, (steps + delta) * step);
    m_controls->setWidthSilent(m_fixedWidth);
    onSettingsChanged();
}

void MainWindow::toggleTheme()
{
    m_theme->toggle();
    m_theme->apply(qApp);
    m_themeAction->setText(
        m_theme->isDark() ? tr("Light theme") : tr("Dark theme"));
    if (m_canvas)
        m_canvas->update();
    if (m_panorama)
        m_panorama->updateTheme();
    if (m_byteInfo)
        m_byteInfo->updateTheme();
    if (m_paletteSettings && !m_currentPaletteId.isEmpty())
        m_paletteSettings->buildFor(m_currentPaletteId);
    if (m_hexView)
        m_hexView->updateTheme();
}

void MainWindow::onLanguageChanged(QAction *action)
{
    if (!action)
        return;
    const QString lang = action->data().toString();
    if (lang.isEmpty() || lang == m_language)
        return;

    m_language = lang;

    if (m_settings) {
        QVariantMap s = m_settings->load();
        s["language"] = m_language;
        m_settings->save(s);
    }

    const QString langName =
        (lang == QLatin1String(AppLimits::LangRu)) ? tr("Russian") : tr("English");

    QMessageBox::information(
        this,
        tr("Language"),
        tr("Language set to %1.\n\n"
           "Restart the application to apply the change.")
            .arg(langName));
}

void MainWindow::onHexViewToggled(bool enabled)
{
    if (!m_hexView)
        return;

    if (enabled) {
        if (m_hexViewHeight > 0) {
            m_hexView->setFixedHeight(m_hexViewHeight);
        } else {
            const int vh = m_centerScroll
                ? m_centerScroll->viewport()->height() : 600;
            m_hexView->setFixedHeight(qMax(160, vh / 3));
            m_hexViewHeight = m_hexView->height();
        }
        m_hexView->refresh();
    }
    m_hexView->setVisible(enabled);
    refreshTitle();
}

void MainWindow::exportPng(PngExportMode mode)
{
    if (m_busyPngExport) {
        statusBar()->showMessage(tr("A PNG export is already running."), 4000);
        return;
    }
    if (m_fileSize <= 0 || !m_canvas || !m_canvas->tileManager())
        return;

    QString dir;
    if (!m_source.sourcePath().isEmpty())
        dir = QFileInfo(m_source.sourcePath()).absolutePath();
    if (dir.isEmpty())
        dir = m_lastDir;
    if (dir.isEmpty())
        dir = QDir::homePath();

    QString stem;
    if (!m_source.sourcePath().isEmpty()) {
        stem = QFileInfo(m_source.sourcePath()).completeBaseName();
    } else if (!m_xorLabel.isEmpty()) {
        stem = QStringLiteral("xor");
    }
    if (stem.isEmpty())
        stem = QStringLiteral("untitled");

    int exportScale = 1;
    int exportDownsample = 1;
    QRect canvasRect;
    QString suffix;
    QString dialogTitle;

    switch (mode) {
    case ExportVisibleArea: {
        exportScale = qMax(1, m_canvas->scale());
        exportDownsample = qMax(1, m_canvas->downsample());
        if (!m_centerScroll || !m_centerScroll->viewport())
            return;
        const QPoint canvasOrigin = m_canvas->mapTo(
            m_centerScroll->viewport(), QPoint(0, 0));
        const QRect canvasInViewport(canvasOrigin, m_canvas->size());
        const QRect visibleInViewport = canvasInViewport.intersected(
            m_centerScroll->viewport()->rect());
        if (!visibleInViewport.isEmpty()) {
            canvasRect = QRect(visibleInViewport.topLeft() - canvasOrigin,
                               visibleInViewport.size());
        }
        suffix = QStringLiteral("_visible");
        dialogTitle = tr("Save visible area as PNG");
        break;
    }
    case ExportFullOriginal:
        exportScale = 1;
        exportDownsample = 1;
        canvasRect = QRect(QPoint(0, 0),
                           QSize(m_canvas->imageWidth(), m_canvas->imageHeight()));
        suffix = QStringLiteral("_1to1");
        dialogTitle = tr("Save entire image (1:1) as PNG");
        break;
    case ExportFullCurrentScale:
    default:
        exportScale = qMax(1, m_canvas->scale());
        exportDownsample = qMax(1, m_canvas->downsample());
        canvasRect = QRect(QPoint(0, 0), m_canvas->scaledSize());
        suffix.clear();
        dialogTitle = tr("Save entire image at current scale as PNG");
        break;
    }

    if (canvasRect.isEmpty()) {
        QMessageBox::information(this, tr("Export to PNG"),
                                 tr("There is no visible image area to export."));
        return;
    }

    const QString def = dir + QLatin1Char('/') + stem + suffix
                      + QStringLiteral(".png");
    QString path = QFileDialog::getSaveFileName(
        this, dialogTitle, def, tr("PNG (*.png)"));
    if (path.isEmpty())
        return;
    const QString selectedSuffix = QFileInfo(path).suffix();
    if (selectedSuffix.isEmpty()) {
        path += QStringLiteral(".png");
    } else if (selectedSuffix.compare(QLatin1String("png"),
                                      Qt::CaseInsensitive) != 0) {
        QMessageBox::warning(
            this, tr("Export to PNG"),
            tr("Please use the .png file extension for PNG exports."));
        return;
    }

    if (!m_source.sourcePath().isEmpty()) {
        const QFileInfo sourceInfo(m_source.sourcePath());
        const QFileInfo destinationInfo(path);
        QString sourcePath = sourceInfo.canonicalFilePath();
        QString destinationPath = destinationInfo.canonicalFilePath();
        if (sourcePath.isEmpty()) {
            const QString sourceDir = QFileInfo(sourceInfo.absolutePath()).canonicalFilePath();
            sourcePath = QDir::cleanPath(
                QDir(sourceDir.isEmpty() ? sourceInfo.absolutePath() : sourceDir)
                    .filePath(sourceInfo.fileName()));
        }
        if (destinationPath.isEmpty()) {
            const QString destinationDir =
                QFileInfo(destinationInfo.absolutePath()).canonicalFilePath();
            destinationPath = QDir::cleanPath(
                QDir(destinationDir.isEmpty() ? destinationInfo.absolutePath()
                                              : destinationDir)
                    .filePath(destinationInfo.fileName()));
        }
#ifdef Q_OS_WIN
        const Qt::CaseSensitivity pathCase = Qt::CaseInsensitive;
#else
        const Qt::CaseSensitivity pathCase = Qt::CaseSensitive;
#endif
        if (sourcePath.compare(destinationPath, pathCase) == 0) {
            QMessageBox::warning(
                this, tr("Export to PNG"),
                tr("The PNG destination must not be the currently opened source file."));
            return;
        }
    }

    const QColor background = m_canvas->palette().window().color();
    QString exportError;
    if (!m_canvas->tileManager()->startPngExport(
            path, canvasRect, exportScale, exportDownsample, background,
            &exportError)) {
        QMessageBox::warning(
            this, tr("Export to PNG"),
			exportError.isEmpty()
				? tr("The requested PNG dimensions are invalid, "
					 "or the export task could not be started.")
				: exportError);
        return;
    }

    m_busyPngExport = true;
    if (m_exportMenu)
        m_exportMenu->setEnabled(false);
    updateBusyLabel();
    statusBar()->showMessage(tr("PNG export started…"));
}

void MainWindow::onPngExportFinished(const QString &path, bool success,
                                     const QString &errorMessage)
{
    m_busyPngExport = false;
    if (m_exportMenu)
        m_exportMenu->setEnabled(true);
    updateBusyLabel();

    if (success) {
        m_lastDir = QFileInfo(path).absolutePath();
        statusBar()->showMessage(tr("PNG saved: %1").arg(QFileInfo(path).fileName()),
                                 6000);
    } else {
        statusBar()->clearMessage();
        QMessageBox::warning(
            this, tr("Export to PNG"),
            tr("Failed to save PNG:\n%1").arg(errorMessage));
    }
}

void MainWindow::refreshTitle()
{
    const bool isXor = !m_xorLabel.isEmpty();
    const bool hasFile = isXor || !m_source.sourcePath().isEmpty();

    const QString baseTitle = tr("Byte-0-scope v%1").arg(APP_VERSION);

    if (!hasFile) {
        setWindowTitle(baseTitle);
    } else {
        QString namePart;
        if (isXor)
            namePart = m_xorLabel;
        else
            namePart = shortenName(QFileInfo(m_source.sourcePath()).fileName());

        const QString sizeComma =
            QLocale(QLocale::English, QLocale::UnitedStates).toString(qint64(m_fileSize));
        QString t = tr("%1: [%2] [0x%3 / %4 bytes] [%5×%6]")
            .arg(baseTitle, namePart)
            .arg(qint64(m_fileSize), 0, 16)
            .arg(sizeComma)
            .arg(m_imageWidth).arg(m_imageHeight);
        if (isXor) {
            t += tr(" [differs: %1%]")
                     .arg(diffPercentText(m_xorDiffBytes, m_fileSize));
        }
        setWindowTitle(t);
    }

    if (m_hintContainer) {
        const bool hexViewVisible = m_hexViewAct && m_hexViewAct->isChecked();
        m_hintContainer->setVisible(!hexViewVisible
                                    && (m_trainingMode || !hasFile));
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (!e->mimeData() || !e->mimeData()->hasUrls()) {
        e->ignore();
        return;
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *e)
{
    if (!e->mimeData() || !e->mimeData()->hasUrls()) {
        e->ignore();
        return;
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    if (!e->mimeData() || !e->mimeData()->hasUrls()) {
        e->ignore();
        return;
    }

    QStringList paths;
    const QList<QUrl> urls = e->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile())
            paths << url.toLocalFile();
    }

    if (inputFiles(paths).isEmpty()) {
        e->ignore();
        return;
    }

    e->setDropAction(Qt::CopyAction);
    e->accept();

    QTimer::singleShot(0, this, [this, paths]() { openFiles(paths); });
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    saveSettings();
    e->accept();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    if (m_resizeTimer)
        m_resizeTimer->start(MW_RESIZE_DEBOUNCE_MS);
}

void MainWindow::applyFitToWindow()
{
    if (!m_centerScroll || !m_canvas)
        return;
    if (m_fileSize <= 0)
        return;

    int w = 0;
    int h = 0;
    if (!computeImageSize(&w, &h))
        return;

    const QSize vp = m_centerScroll->viewport()->size();
    const int availW = qMax(1, vp.width()  - 2);
    const int availH = qMax(1, vp.height() - 2);

    const double ideal = qMin(qMin(double(availW) / w,
                                   double(availH) / h), 32.0);
    int newScale = 1, newDs = 1;
    if (ideal >= 1.0) {
        newScale = qMax(1, int(ideal));
    } else {
        newScale = 1;
        newDs = qMax(1, int(std::ceil(1.0 / ideal)));
    }

    if (newScale != m_scale || newDs != m_downsample) {
        m_scale = newScale;
        m_downsample = newDs;
        if (m_controls)
            m_controls->setScaleSilent(m_scale);
        m_canvas->rebuild();
    }
}

void MainWindow::centerViewportOnSourcePixel(const QPoint &sourcePixel)
{
    if (!m_centerScroll || !m_canvas
            || sourcePixel.x() < 0 || sourcePixel.y() < 0
            || sourcePixel.x() >= m_imageWidth
            || sourcePixel.y() >= m_imageHeight)
        return;

    QScrollBar *h = m_centerScroll->horizontalScrollBar();
    QScrollBar *v = m_centerScroll->verticalScrollBar();
    QWidget *viewport = m_centerScroll->viewport();
    if (!h || !v || !viewport)
        return;

    int displayX = m_mirrorH ? (m_imageWidth - 1 - sourcePixel.x())
                             : sourcePixel.x();
    int displayY = m_mirrorV ? (m_imageHeight - 1 - sourcePixel.y())
                             : sourcePixel.y();

    const int ds = qMax(1, m_downsample);
    const int scale = qMax(1, m_scale);
    const qint64 canvasX = qint64(displayX / ds) * scale + scale / 2;
    const qint64 canvasY = qint64(displayY / ds) * scale + scale / 2;
    const qint64 targetH = canvasX - viewport->width() / 2;
    const qint64 targetV = canvasY - viewport->height() / 2;

    const int newH = int(qBound<qint64>(h->minimum(), targetH, h->maximum()));
    const int newV = int(qBound<qint64>(v->minimum(), targetV, v->maximum()));
    h->setValue(newH);
    v->setValue(newV);

    if (m_panorama)
        m_panorama->updatePanorama();
}

void MainWindow::autoFitScale()
{
    if (!m_centerScroll || !m_canvas)
        return;
    if (m_fileSize <= 0)
        return;

    int w = 0;
    int h = 0;
    if (!computeImageSize(&w, &h))
        return;

    const QSize vp = m_centerScroll->viewport()->size();
    const int availW = qMax(1, vp.width()  - 20);
    const int availH = qMax(1, vp.height() - 20);
    const int scaleW = availW / w;
    const int scaleH = availH / h;
    const int newScale = qBound(1, qMin(scaleW, scaleH), 32);

    if (newScale != m_scale) {
        m_scale = newScale;
        m_downsample = 1;
        if (m_controls)
            m_controls->setScaleSilent(m_scale);
        m_canvas->rebuild();
        if (m_panorama)
            m_panorama->updatePanorama();
    }
}

void MainWindow::onPerfSliderMoved(int value)
{
    m_pendingPerfValue = value;
    if (m_perfDebounceTimer)
        m_perfDebounceTimer->start(MW_PERF_DEBOUNCE_MS);
}

void MainWindow::applyPerfSettings()
{
    if (m_pendingPerfValue < 0)
        return;
    const int v = m_pendingPerfValue;
    m_pendingPerfValue = -1;
    applyPerfFromSlider(v);
}

void MainWindow::applyPerfFromSlider(int value)
{
    value = qBound(1, value, 20);
    m_perfSliderValue = value;

    const double norm = (value - 1) / 19.0;
    const double logTs = std::log(256.0) + norm * (std::log(2048.0) - std::log(256.0));
    m_tileSize = qBound(256, int(std::exp(logTs) + 0.5), 2048);

    const double logM = norm * std::log(6.0);
    m_tilePrefetchMargin = qBound(2, int(std::exp(logM) + 0.5), 7);

    const int cacheMaxMb = QtCompat::is32BitProcess() ? MW_CACHE_MAX_MB_32
                                                      : MW_CACHE_MAX_MB_64;
    const double logC = std::log(180.0) + norm * (std::log(double(cacheMaxMb)) - std::log(180.0));
    const int cacheMb = qBound(180, int(std::exp(logC) + 0.5), cacheMaxMb);

    if (m_canvas && m_canvas->tileManager()) {
        m_canvas->tileManager()->setCacheLimitMb(cacheMb);
        m_canvas->rebuild();
    }
}

void MainWindow::onActiveTasksChanged(int count)
{
    m_activeTileTasks = count;
    updateBusyLabel();
}

void MainWindow::updateBusyLabel()
{
    if (!m_busyLabel)
        return;
    if (m_busyPngExport) {
        m_busyLabel->setText(tr("[ Saving to PNG...]"));
    } else if (m_activeTileTasks > 0) {
        m_busyLabel->setText(tr("[ Rendering...]"));
    } else {
        m_busyLabel->clear();
    }
}

void MainWindow::updateMemoryLabel()
{
    if (!m_memoryLabel)
        return;

    qint64 rss = 0;
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             (PROCESS_MEMORY_COUNTERS *)&pmc,
                             sizeof(pmc))) {
        rss = qint64(pmc.WorkingSetSize);
    }
#else
    QFile f("/proc/self/statm");
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray line = f.readLine();
        f.close();
        const QList<QByteArray> parts = line.simplified().split(' ');
        if (parts.size() >= 2) {
            const qint64 pages = parts[1].toLongLong();
            const long pageSize = sysconf(_SC_PAGESIZE);
            rss = pages * (pageSize > 0 ? qint64(pageSize) : 4096);
        }
    }
#endif

    qint64 cacheBytes = 0;
    int    cacheLimit = 0;
    if (m_canvas && m_canvas->tileManager()) {
        cacheBytes = m_canvas->tileManager()->cacheBytes();
        cacheLimit = m_canvas->tileManager()->cacheLimitMb();
    }

    const double mbRss   = rss / MW_BYTES_PER_MB;
    const double mbCache = cacheBytes / MW_BYTES_PER_MB;

    const QString rssLine = (rss > 0)
        ? tr("· Process memory: %1 MB").arg(mbRss, 0, 'f', 0)
        : tr("· Process memory: — MB");

    const QString cacheLine = tr("· Tile cache: %1 MB / %2 MB")
        .arg(mbCache, 0, 'f', 1)
        .arg(cacheLimit);

    m_memoryLabel->setText(rssLine + "\n" + cacheLine);
    if (m_showDebugInfo)
        updateDebugInfo();
}

void MainWindow::onMirrorChanged()
{
    if (m_canvas)
        m_canvas->rebuild();
    if (m_panorama)
        m_panorama->updatePanorama();
}

void MainWindow::setMirrorState(bool horizontal, bool vertical)
{
    m_mirrorH = horizontal;
    m_mirrorV = vertical;
    if (m_mirrorHAct) {
        QSignalBlocker b(m_mirrorHAct);
        m_mirrorHAct->setChecked(m_mirrorH);
    }
    if (m_mirrorVAct) {
        QSignalBlocker b(m_mirrorVAct);
        m_mirrorVAct->setChecked(m_mirrorV);
    }
}

void MainWindow::resetMirror()
{
    setMirrorState(false, false);
    if (m_canvas)
        m_canvas->rebuild();
}

void MainWindow::resetDisplaySettings()
{
    applyDefaultConfig();
}

void MainWindow::applyDefaultConfig()
{
    m_paletteParams.remove(QString::fromLatin1(PaletteIds::Repeats));
    setMirrorState(false, false);

    if (m_controls)
        m_controls->setConfig(defaultDisplayConfig());

    const QString paletteId = m_controls
        ? m_controls->currentPaletteId()
        : QStringLiteral(CFG_DEF_PALETTE);
    onPaletteChanged(paletteId.isEmpty() ? QStringLiteral(CFG_DEF_PALETTE)
                                         : paletteId);
}

QStringList MainWindow::inputFiles(const QStringList &paths)
{
    QStringList files;
    QStringList canonical;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (path.isEmpty() || !info.isFile())
            continue;
        const QString key = info.canonicalFilePath();
        if (canonical.contains(key))
            continue;
        canonical << key;
        files << path;
    }

    QStringList result;
    for (const QString &path : files) {
        if (path.endsWith(QLatin1String(".b0s"), Qt::CaseInsensitive)) {
            const QString dataPath = path.left(path.size() - 4);
            const QString dataKey = QFileInfo(dataPath).canonicalFilePath();
            if (!dataKey.isEmpty() && canonical.contains(dataKey))
                continue;
        }
        result << path;
    }
    return result;
}

void MainWindow::openFiles(const QStringList &paths)
{
    const QStringList files = inputFiles(paths);
    if (files.isEmpty())
        return;

    if (files.size() == 1) {
        loadFile(files.first());
        return;
    }

    if (loadFileXor(files.at(0), files.at(1)) && files.size() > 2) {
        statusBar()->showMessage(
            tr("%1 files were given: only the first two are compared (XOR).")
                .arg(files.size()),
            7000);
    }
}

int MainWindow::averageByteRepeatCount()
{
    ensureByteHistogram();
    return PaletteManager::averageRepeatCount(m_byteHistogram);
}

void MainWindow::ensureByteHistogram()
{
    if (m_byteHistogramValid)
        return;

    ::memset(m_byteHistogram, 0, sizeof(m_byteHistogram));

    const uchar *p = m_source.rawData();
    const qint64 n = m_source.size();

    for (qint64 i = 0; i < n; ++i)
        ++m_byteHistogram[p[i]];

    m_byteHistogramValid = true;
}

void MainWindow::onTrainingButtonClicked()
{
    startTraining();
}

void MainWindow::startTraining()
{
    const QByteArray data = DemoBuilder::buildTrainingDemo();
    m_source.setData(data);
    m_fileSize = data.size();
    m_xorLabel.clear();
    m_trainingMode = true;
    m_offset = 0;
    m_length = 0;
    m_lengthMode = false;
    invalidateByteHistogram();

    applyDefaultConfig();

    if (m_canvas)
        m_canvas->clearHover();
    recomputeGeometry();
    refreshTitle();
    if (m_panorama)
        m_panorama->updatePanorama();

    if (m_trainingBtn)
        m_trainingBtn->setVisible(false);
    if (m_hintLabel)
        m_hintLabel->setText(DemoBuilder::trainingHintHtml());
	if (m_hexView)
        m_hexView->refresh();
    
	statusBar()->showMessage(
        tr("Tutorial: adjust the display settings to reveal the image."),
        8000);
}