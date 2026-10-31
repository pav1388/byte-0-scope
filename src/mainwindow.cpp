// Byte-0-scope - binary data visualizer
// File: mainwindow.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"
#include "aboutdialog.h"
#include "byteinfo.h"
#include "canvaswidget.h"
#include "controlspanel.h"
#include "demobuilder.h"
#include "fileassoc.h"
#include "palettemanager.h"
#include "palettesettings.h"
#include "panoramawidget.h"
#include "settingsmanager.h"
#include "thememanager.h"
#include "tilemanager.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDateTime>
#include <QDesktopServices>
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
#include <limits>

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <psapi.h>
#endif

#ifndef APP_VERSION
#  define APP_VERSION "er-dev"
#endif

// --- Настройки модуля MainWindow (префикс MW_) ---

#define MW_LANG_RU   "ru"
#define MW_LANG_EN   "en"
#define MW_PANEL_WIDTH              300
#define MW_LAYOUT_SPACING           2
#define MW_DEFAULT_WINDOW_W         1000
#define MW_DEFAULT_WINDOW_H         720
#define MW_LAYOUT_CENTER_MARGIN     5
#define MW_RECENT_MAX               10
#define MW_MEMORY_TIMER_MS          2000
#define MW_RESIZE_DEBOUNCE_MS       50
#define MW_PERF_DEBOUNCE_MS         1000
#define MW_SHORTEN_NAME_MAX         30
#define MW_BYTES_PER_KB             1024LL
#define MW_BYTES_PER_MB             (1024.0 * 1024.0)
#define MW_MEMORY_RESERVE_MIN       (512LL * 1024 * 1024)
#define MW_MEMORY_RESERVE_FRACTION  4
#define MW_MAX_SCALE                32
#define MW_BUSY_POLL_MS             2000

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
    if (cap < 1)
        cap = 1;
    int best = 1;
    for (int i = 1; i * i <= n; ++i) {
        if (n % i != 0)
            continue;
        if (i <= cap && i > best)
            best = i;
        const int q = n / i;
        if (q <= cap && q > best)
            best = q;
    }
    return best;
}

static QString shortenName(const QString &name, int maxLen = MW_SHORTEN_NAME_MAX)
{
    if (name.size() <= maxLen)
        return name;

    const QString ext = QFileInfo(name).suffix();
    if (!ext.isEmpty() && ext.size() + 2 < maxLen) {
        const int headLen = maxLen - ext.size() - 2;
        return name.left(headLen) + QChar(0x2026) + QLatin1Char('.') + ext;
    }
    return name.left(maxLen - 1) + QChar(0x2026);
}

static QString widthModeName(int m)
{
    switch (m) {
    case 0: return QStringLiteral("fixed");
    case 1: return QStringLiteral("auto");
    case 2: return QStringLiteral("power");
    case 3: return QStringLiteral("aspect16_9");
    case 4: return QStringLiteral("aspect4_3");
    case 5: return QStringLiteral("bytes");
    case 6: return QStringLiteral("nice");
    case 7: return QStringLiteral("divisor");
    default: return QStringLiteral("unknown");
    }
}

static QString widthModeKey(int m)
{
    return widthModeName(m);
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

    if (m_loadedStartupConfig.isEmpty()) {
        applyDefaultConfig();
    } else {
        if (m_controls)
            m_controls->setConfig(m_loadedStartupConfig);
        const QString paletteId = m_controls
            ? m_controls->currentPaletteId()
            : QStringLiteral("heat");
        onPaletteChanged(paletteId.isEmpty() ? QStringLiteral("heat") : paletteId);
    }

    if (m_byteInfo && m_byteTooltipAct)
        m_byteInfo->setPopupEnabled(m_byteTooltipAct->isChecked());

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

    QAction *resetCfgAct = fileMenu->addAction(tr("Reset display settings"));
    resetCfgAct->setShortcut(QKeySequence("Ctrl+R"));
    connect(resetCfgAct, &QAction::triggered, this, &MainWindow::resetConfig);

    QAction *exportAct = fileMenu->addAction(tr("Export to PNG…"));
    exportAct->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAct, &QAction::triggered, this, &MainWindow::exportPng);

    fileMenu->addSeparator();

    QAction *closeFileAct = fileMenu->addAction(tr("Close current file"));
    closeFileAct->setShortcut(QKeySequence("Ctrl+W"));
    connect(closeFileAct, &QAction::triggered, this, &MainWindow::closeCurrentFile);

    QAction *quitAct = fileMenu->addAction(tr("Exit"));
    quitAct->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAct, &QAction::triggered, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu(tr("View"));

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

    viewMenu->addSeparator();
    viewMenu->addAction(tr("Reset flips"), this, &MainWindow::resetMirror);

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
	langEn->setData(QStringLiteral(MW_LANG_EN));
	langGroup->addAction(langEn);

	QAction *langRu = langMenu->addAction(tr("Russian"));
	langRu->setCheckable(true);
	langRu->setData(QStringLiteral(MW_LANG_RU));
	langGroup->addAction(langRu);

	if (m_language == QLatin1String(MW_LANG_RU))
		langRu->setChecked(true);
	else
		langEn->setChecked(true);

	connect(langGroup, &QActionGroup::triggered,
			this, &MainWindow::onLanguageChanged);
    m_settingsMenu->addSeparator();

    m_byteTooltipAct = m_settingsMenu->addAction(tr("Byte tooltip"));
    m_byteTooltipAct->setCheckable(true);
    m_byteTooltipAct->setChecked(
        m_loadedStartupConfig.value("byte_tooltip", false).toBool());
    connect(m_byteTooltipAct, &QAction::toggled, this, [this](bool checked) {
        if (m_byteInfo)
            m_byteInfo->setPopupEnabled(checked);
    });

    m_settingsMenu->addSeparator();

    m_loadIntoMemoryAct = m_settingsMenu->addAction(
        tr("Load the entire file into memory"));
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
    for (int i = 0; i < m_recentFiles.size() && i < 8; ++i) {
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

    // --- Левая панель ---
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

    // --- Центр ---
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

    // --- Правая панель ---
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

    auto *perfGroup = new QGroupBox(tr("RAM usage"), this);
    auto *perfLay = new QVBoxLayout(perfGroup);

    m_memoryLabel = new QLabel(
        tr("· Process memory: — MB\n· Tile cache: — MB"), this);
    perfLay->addWidget(m_memoryLabel);

    auto *sldRow = new QHBoxLayout();

    auto *lblMinus = new QLabel(tr("-"), this);
    lblMinus->setAlignment(Qt::AlignRight);
    lblMinus->setToolTip(tr("128 px per tile · cache limit 180 MB"));
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
    lblPlus->setToolTip(tr("1024 px per tile · cache limit 2 GB"));
    sldRow->addWidget(lblPlus);

    perfLay->addLayout(sldRow);
    rightLay->addWidget(perfGroup);

    m_debugGroup = new QGroupBox(tr("Debug"), this);
    auto *dbgLay = new QVBoxLayout(m_debugGroup);
    m_debugLabel = new QLabel(this);
    m_debugLabel->setStyleSheet("font-family: monospace; font-size: 8pt;");
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
	m_trainingMode = false;
	if (m_trainingBtn)
		m_trainingBtn->setVisible(false);
    // Нормализация: если пришёл .b0s — работаем с парным data-файлом,
    // но конфиг ищем по ИСХОДНОМУ пути (path), а не по «path + .b0s».
    QString actualPath = path;
    QString cfgPath;
    bool    fromB0s = false;

    if (actualPath.endsWith(".b0s", Qt::CaseInsensitive)) {
        fromB0s = true;
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
        cfgPath = actualPath + ".b0s";
    }
    Q_UNUSED(fromB0s);

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

    bool cfgLoaded = false;
    if (QFile::exists(cfgPath)) {
        QFile f(cfgPath);
        if (f.open(QIODevice::ReadOnly)) {
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
            f.close();
            if (err.error == QJsonParseError::NoError && doc.isObject())
                cfgLoaded = applyConfigFromJson(doc.object());
            else
                qWarning("b0s: could not parse %s: %s",
                         qPrintable(cfgPath), qPrintable(err.errorString()));
        }
    }
    if (!cfgLoaded)
        applyDefaultConfig();

    recomputeGeometry();
    refreshTitle();
    if (m_canvas)
        m_canvas->rebuild();
    if (m_panorama)
        m_panorama->updatePanorama();
}

void MainWindow::addRecent(const QString &path)
{
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > MW_RECENT_MAX)
        m_recentFiles.removeLast();
    m_recentFiles = SettingsManager::filterExisting(m_recentFiles, MW_RECENT_MAX);
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
	m_trainingMode = false;
	if (m_trainingBtn)
		m_trainingBtn->setVisible(false);
	
    QFile f1(p1), f2(p2);
    if (!f1.open(QIODevice::ReadOnly) || !f2.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to open one of the files for comparison."));
        return false;
    }

    const qint64 sizeA = f1.size();
    const qint64 sizeB = f2.size();
    const qint64 needed = sizeA + sizeB + qMin(sizeA, sizeB);

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    const qint64 qbaMax = qint64(QByteArray::maxSize());
#else
    const qint64 qbaMax = qint64(std::numeric_limits<int>::max());
#endif

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
    for (int i = 0; i < n; ++i)
        po[i] = pa[i] ^ pb[i];

    m_source.setData(xorData);
    m_fileSize = xorData.size();
    m_offset = 0;
    m_length = 0;
    m_xorLabel = QString("[XOR] %1 ^ %2")
        .arg(shortenName(QFileInfo(p1).fileName(), 10),
             shortenName(QFileInfo(p2).fileName(), 10));
    m_lastDir = QFileInfo(p1).absolutePath();
    invalidateByteHistogram();

    bool cfgLoaded = false;
    for (const QString &path : { p1, p2 }) {
        const QString cfgPath = path + ".b0s";
        if (!QFile::exists(cfgPath))
            continue;
        QFile f(cfgPath);
        if (!f.open(QIODevice::ReadOnly))
            continue;
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        f.close();
        if (err.error != QJsonParseError::NoError || !doc.isObject())
            continue;
        cfgLoaded = applyConfigFromJson(doc.object());
        if (cfgLoaded)
            break;
    }
    if (!cfgLoaded)
        applyDefaultConfig();

    recomputeGeometry();
    refreshTitle();
    if (m_canvas)
        m_canvas->rebuild();
    if (m_panorama)
        m_panorama->updatePanorama();
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

    if (m_canvas) {
        m_canvas->clearHover();
        if (m_canvas->tileManager())
            m_canvas->tileManager()->clearCache();
    }
    if (m_byteInfo)
        m_byteInfo->updateInfo(-1);

    applyDefaultConfig();
}

// =========================================================================
//  Палитра
// =========================================================================

void MainWindow::onPaletteChanged(const QString &value)
{
    if (!m_paletteManager)
        return;

    const QString &id = value;
    if (!m_paletteManager->contains(id))
        return;

    m_currentPaletteId = id;

    if (!m_paletteParams.contains(id))
        m_paletteParams[id] = m_paletteManager->defaultParams(id);

    const QVariantMap params = m_paletteParams.value(id).toMap();

    const bool wantsHistogram =
        (id == QLatin1String("byte_frequency") ||
         id == QLatin1String("repeats"));

    if (wantsHistogram)
        ensureByteHistogram();
    else
        invalidateByteHistogram();

    m_currentPalette = m_paletteManager->createPalette(
        id, params, m_source.data(),
        wantsHistogram ? m_byteHistogram : nullptr);

    if (m_paletteSettings)
        m_paletteSettings->buildFor(id);
    if (m_canvas)
        m_canvas->rebuild();
    emit paletteChanged();
}

void MainWindow::setPaletteParams(const QString &value, const QVariantMap &params)
{
    if (!m_paletteManager)
        return;
    const QString &id = value;
    if (!m_paletteManager->contains(id))
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

    const bool wantsHistogram =
        (m_currentPaletteId == QLatin1String("byte_frequency") ||
         m_currentPaletteId == QLatin1String("repeats"));

    if (wantsHistogram)
        ensureByteHistogram();
    else
        invalidateByteHistogram();

    m_currentPalette = m_paletteManager->createPalette(
        m_currentPaletteId, newParams, m_source.data(),
        wantsHistogram ? m_byteHistogram : nullptr);

    if (m_canvas)
        m_canvas->rebuild();
    emit paletteChanged();
}

void MainWindow::onPaletteAutoApplyToggled(bool enabled)
{
    m_autoApplyPalette = enabled;
    if (!enabled)
        return;

    const bool wantsHistogram =
        m_currentPaletteId == QLatin1String("byte_frequency") ||
        m_currentPaletteId == QLatin1String("repeats");
    if (wantsHistogram)
        ensureByteHistogram();
    else
        invalidateByteHistogram();

    const QVariantMap params = m_paletteParams.value(m_currentPaletteId).toMap();
    m_currentPalette = m_paletteManager->createPalette(
        m_currentPaletteId, params, m_source.data(),
        wantsHistogram ? m_byteHistogram : nullptr);
    if (m_canvas)
        m_canvas->rebuild();
    emit paletteChanged();
}

// =========================================================================
//  Геометрия
// =========================================================================

void MainWindow::recomputeGeometry()
{
    calculateDimensions();
    if (m_canvas)
        m_canvas->rebuild();
    if (m_panorama)
        m_panorama->updatePanorama();
}

int MainWindow::calculateWidth() const
{
    if (m_fileSize <= 0)
        return 0;

    const quint64 actualLen = qMin<quint64>(
        m_length ? m_length : (quint64(m_fileSize) - m_offset),
        quint64(m_fileSize) - m_offset);
    if (actualLen == 0)
        return 0;

    const int bp = qMax(1, bytesPerPixel());
    const int pixels = isBitpacked()
        ? int((actualLen * 8) / qMax(1, m_bppMode))
        : int(actualLen / bp);
    if (pixels <= 0)
        return 0;

    switch (m_widthMode) {
    case 0: return qMax(1, m_fixedWidth);
    case 1: return qMax(1, int(qSqrt(double(pixels))));
    case 2: return calcPowerOfTwoWidth(pixels);
    case 3: return aspectWidth(pixels, 16, 9);
    case 4: return aspectWidth(pixels, 4, 3);
    case 5: return qMax(1, m_bytesPerRow / bp);
    case 6: return nearestNiceWidth(pixels);
    case 7: return largestDivisorLE(pixels, m_fixedWidth);
    }
    return qMax(1, m_fixedWidth);
}

void MainWindow::calculateDimensions()
{
    m_imageWidth = m_imageHeight = 0;
    if (m_fileSize <= 0)
        return;

    const quint64 length = m_length ? m_length : quint64(m_fileSize);
    const quint64 end = qMin<quint64>(m_offset + length, quint64(m_fileSize));
    if (end <= m_offset)
        return;
    const quint64 actualLen = end - m_offset;

    const int pixels = isBitpacked()
        ? int((actualLen * 8) / qMax(1, m_bppMode))
        : int(actualLen / qMax(1, bytesPerPixel()));
    if (pixels <= 0)
        return;

    m_imageWidth = calculateWidth();
    if (m_imageWidth <= 0)
        return;
    m_imageHeight = (pixels + m_imageWidth - 1) / m_imageWidth;

    if (m_centerScroll) {
        const QSize vp = m_centerScroll->viewport()->size();
        m_viewportRect = QRect(
            m_centerScroll->horizontalScrollBar()->value() / qMax(1, m_scale),
            m_centerScroll->verticalScrollBar()->value()   / qMax(1, m_scale),
            vp.width()  / qMax(1, m_scale),
            vp.height() / qMax(1, m_scale));
    }
}

// =========================================================================
//  Масштаб / настройки
// =========================================================================

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
    const int oldBpp = m_bppMode;
    m_bppMode        = m_controls->bppMode();
    m_bitOrder       = m_controls->bitOrder();
    m_pixelOrder     = m_controls->pixelOrder();
    m_rowPaddingBits = m_controls->rowPaddingBits();
    m_widthMode      = m_controls->widthMode();
    m_fixedWidth     = m_controls->fixedWidth();
    m_widthStep      = m_controls->widthStep();
    m_scale          = m_controls->scaleValue();
    m_autoScale      = m_controls->autoScale();
    m_fitToWindow    = m_controls->fitToWindow();
    m_offset         = m_controls->offset();
    m_length         = m_controls->length();
    m_lengthMode     = m_controls->lengthMode();
    m_bytesPerRow    = m_controls->bytesPerRow();

    if (oldBpp != m_bppMode)
        onPaletteChanged(m_controls->currentPaletteId());

    if (m_fitToWindow) {
        applyFitToWindow();
    } else if (m_autoScale) {
        if (m_canvas && m_downsample != 1) {
            m_downsample = 1;
            if (m_canvas->tileManager())
                m_canvas->tileManager()->clearCache();
        }
        autoFitScale();
    } else {
        if (m_canvas && m_downsample != 1) {
            m_downsample = 1;
            if (m_canvas->tileManager())
                m_canvas->tileManager()->clearCache();
        }
    }

    recomputeGeometry();
    refreshTitle();
}

void MainWindow::onHoverChanged(qint64 addr)
{
    if (m_byteInfo)
        m_byteInfo->updateInfo(addr);
    m_hoverAddr = addr;
    if (m_panorama)
        m_panorama->update();
}

void MainWindow::onLockedChanged(qint64 addr)
{
    if (m_byteInfo && addr >= 0)
        m_byteInfo->updateInfo(addr);
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

    if (m_widthMode != 0) {
        const int curW = calculateWidth();
        if (curW > 0)
            m_fixedWidth = curW;
        m_widthMode = 0;
        m_controls->setWidthModeSilent(0);
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
        (lang == QLatin1String(MW_LANG_RU)) ? tr("Russian") : tr("English");

    QMessageBox::information(
        this,
        tr("Language"),
        tr("Language set to %1.\n\n"
           "Restart the application to apply the change.")
            .arg(langName));
}

// =========================================================================
//  PNG
// =========================================================================

void MainWindow::exportPng()
{
    if (m_fileSize <= 0 || !m_canvas)
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

    const QString def = dir + QLatin1Char('/') + stem + QStringLiteral(".png");

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save PNG"), def, tr("PNG (*.png)"));
    if (path.isEmpty())
        return;

    const QSize imgSize = m_canvas->scaledSize();
    if (imgSize.isEmpty())
        return;

    m_busyPngExport = true;
    updateBusyLabel();
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QImage out = m_canvas->renderToImage(imgSize);

    QApplication::restoreOverrideCursor();
    m_busyPngExport = false;
    updateBusyLabel();

    if (out.isNull())
        return;

    if (!out.save(path, "PNG")) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save."));
        return;
    }

    m_lastDir = QFileInfo(path).absolutePath();
}

// =========================================================================
//  Заголовок / drag&drop / close / hint
// =========================================================================

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
        if (isBitpacked())
            t += QString(" %1bpp").arg(m_bppMode);
        setWindowTitle(t);
    }

	if (m_hintContainer)
		m_hintContainer->setVisible(m_trainingMode || !hasFile);
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
    const QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty()) {
        e->ignore();
        return;
    }

    const QString path = urls.first().toLocalFile();
    if (path.isEmpty()) {
        e->ignore();
        return;
    }

    loadFile(path);
    e->setDropAction(Qt::CopyAction);
    e->accept();
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

// =========================================================================
//  Настройки (INI)
// =========================================================================

void MainWindow::loadSettings()
{
    const QVariantMap s = m_settings->load();
    if (s.isEmpty())
        return;

    {
        const QString themeStr = s.value("theme", "light").toString();
        const bool wantDark =
            (themeStr.compare("dark", Qt::CaseInsensitive) == 0);
        m_theme->setDark(wantDark);
    }

    m_lastDir               = s.value("last_directory").toString();
    m_perfSliderValue       = s.value("perf_slider_value", 8).toInt();
    m_loadIntoMemory        = s.value("load_into_memory", false).toBool();
    m_showDebugInfo         = s.value("show_debug_info", false).toBool();
    m_startupGeometryB64    = s.value("window_geometry").toString();
    m_startupNormalGeometry = s.value("window_normal_geometry").toString();
    m_startupMaximized      = s.value("window_maximized", false).toBool();
    m_startupFullscreen     = s.value("window_fullscreen", false).toBool();
	m_language = s.value("language", QStringLiteral(MW_LANG_EN)).toString();
	if (m_language.isEmpty())
		m_language = QStringLiteral(MW_LANG_EN);
	
    const QStringList recents = s.value("recent_files").toStringList();
    m_recentFiles = SettingsManager::filterExisting(recents, MW_RECENT_MAX);

    m_loadedStartupConfig["bpp_mode"]         = s.value("bpp_mode", 8).toInt();
    m_loadedStartupConfig["bit_order"]        = s.value("bit_order", "msb").toString();
    m_loadedStartupConfig["pixel_order"]      = s.value("pixel_order", "msb").toString();
    m_loadedStartupConfig["row_padding_bits"] = s.value("row_padding_bits", 0).toInt();
    m_loadedStartupConfig["scale"]            = s.value("scale", 1).toInt();
    m_loadedStartupConfig["width_mode"]       = s.value("width_mode", "fixed").toString();
    m_loadedStartupConfig["fixed_width"]      = s.value("fixed_width", 256).toInt();
    m_loadedStartupConfig["width_step"]       = s.value("width_step", 8).toInt();
    m_loadedStartupConfig["auto_scale"]       = s.value("auto_scale", false).toBool();
    m_loadedStartupConfig["fit_to_window"]    = s.value("fit_to_window", false).toBool();

    QString startupPaletteId = s.value("palette", "heat").toString();
    if (!m_paletteManager->contains(startupPaletteId))
        startupPaletteId = QStringLiteral("heat");
    m_loadedStartupConfig["palette"]          = startupPaletteId;
    m_loadedStartupConfig["byte_tooltip"]     = s.value("byte_tooltip", false).toBool();
    m_loadedStartupConfig["bytes_per_row"]    = s.value("bytes_per_row", 16).toInt();
}

void MainWindow::saveSettings()
{
    QVariantMap s;
	s["language"] = m_language;
    s["theme"]             = (m_theme && m_theme->isDark())
                                 ? QStringLiteral("dark")
                                 : QStringLiteral("light");
    s["bpp_mode"]          = m_bppMode;
    s["bit_order"]         = m_bitOrder;
    s["pixel_order"]       = m_pixelOrder;
    s["row_padding_bits"]  = m_rowPaddingBits;
    s["scale"]             = m_scale;
    s["width_mode"]        = widthModeKey(m_widthMode);
    s["bytes_per_row"]     = m_bytesPerRow;
    s["fixed_width"]       = m_fixedWidth;
    s["width_step"]        = m_widthStep;
    s["auto_scale"]        = m_autoScale;
    s["fit_to_window"]     = m_fitToWindow;
    s["palette"]           = m_currentPaletteId;
    s["perf_slider_value"] = m_perfSliderValue;
    s["load_into_memory"]  = m_loadIntoMemory;
    s["last_directory"]    = m_lastDir;
    s["byte_tooltip"]      = (m_byteInfo && m_byteInfo->isPopupEnabled());
    s["show_debug_info"]   = m_showDebugInfo;
    s["recent_files"]      = m_recentFiles;
    s["window_geometry"]   = QString::fromLatin1(saveGeometry().toBase64());

    const QRect ng = normalGeometry();
    s["window_normal_geometry"] = QString("%1,%2,%3,%4")
        .arg(ng.x()).arg(ng.y()).arg(ng.width()).arg(ng.height());
    s["window_maximized"]  = isMaximized();
    s["window_fullscreen"] = isFullScreen();

    m_settings->save(s);
}

// =========================================================================
//  Fit-to-window / auto-scale
// =========================================================================

void MainWindow::applyFitToWindow()
{
    if (!m_centerScroll || !m_canvas)
        return;
    if (m_fileSize <= 0)
        return;

    const int w = calculateWidth();
    if (w <= 0)
        return;

    const quint64 length = m_length ? m_length : quint64(m_fileSize);
    const quint64 end = qMin<quint64>(m_offset + length, quint64(m_fileSize));
    if (end <= m_offset)
        return;
    const quint64 actualLen = end - m_offset;

    const int pixels = isBitpacked()
        ? int((actualLen * 8) / qMax(1, m_bppMode))
        : int(actualLen / qMax(1, bytesPerPixel()));
    const int h = (pixels + w - 1) / w;
    if (h <= 0)
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

void MainWindow::autoFitScale()
{
    if (!m_centerScroll || !m_canvas)
        return;
    if (m_fileSize <= 0)
        return;

    const int w = calculateWidth();
    if (w <= 0)
        return;

    const quint64 length = m_length ? m_length : quint64(m_fileSize);
    const quint64 end = qMin<quint64>(m_offset + length, quint64(m_fileSize));
    if (end <= m_offset)
        return;
    const quint64 actualLen = end - m_offset;

    const int pixels = isBitpacked()
        ? int((actualLen * 8) / qMax(1, m_bppMode))
        : int(actualLen / qMax(1, bytesPerPixel()));
    const int h = (pixels + w - 1) / w;
    if (h <= 0)
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

// =========================================================================
//  Perf-слайдер
// =========================================================================

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
    if (m_canvas && m_canvas->tileManager()) {
        m_canvas->tileManager()->clearCache();
        m_canvas->setForceRedraw();
    }
}

void MainWindow::applyPerfFromSlider(int value)
{
    m_perfSliderValue = value;

    const double norm  = (value - 1) / 19.0;
    const double logTs = std::log(128.0) + norm * (std::log(1024.0) - std::log(128.0));
    m_tileSize = qBound(128, int(std::exp(logTs) + 0.5), 1024);

    const double logM = norm * std::log(6.0);
    m_tilePrefetchMargin = qBound(2, int(std::exp(logM) + 0.5), 7);

    const double logC = std::log(180.0) + norm * (std::log(2000.0) - std::log(180.0));
    const int cacheMb = qBound(180, int(std::exp(logC) + 0.5), 2000);
    if (m_canvas && m_canvas->tileManager()) {
        m_canvas->tileManager()->setCacheLimitMb(cacheMb);
        m_canvas->tileManager()->clearCache();
        m_canvas->setForceRedraw();
    }
}

// =========================================================================
//  Busy / Память / Зеркала
// =========================================================================

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
            rss = pages * 4096;
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
    if (m_byteInfo && m_hoverAddr >= 0)
        m_byteInfo->updateInfo(m_hoverAddr);
    if (m_panorama)
        m_panorama->updatePanorama();
}

void MainWindow::resetMirror()
{
    m_mirrorH = false;
    m_mirrorV = false;
    if (m_mirrorHAct) {
        QSignalBlocker b(m_mirrorHAct);
        m_mirrorHAct->setChecked(false);
    }
    if (m_mirrorVAct) {
        QSignalBlocker b(m_mirrorVAct);
        m_mirrorVAct->setChecked(false);
    }
    if (m_canvas)
        m_canvas->rebuild();
    if (m_byteInfo && m_hoverAddr >= 0)
        m_byteInfo->updateInfo(m_hoverAddr);
}

// =========================================================================
//  Per-file config (.b0s) + reset
// =========================================================================

QString MainWindow::configPathForFile(const QString &filePath) const
{
    if (filePath.isEmpty())
        return QString();
    if (!m_source.isOpen() || !m_xorLabel.isEmpty())
        return QString();
    return filePath + ".b0s";
}

QJsonObject MainWindow::configToJson() const
{
    QJsonObject c;
    c["bpp_mode"]           = m_bppMode;
    c["bit_order"]          = m_bitOrder;
    c["pixel_order"]        = m_pixelOrder;
    c["row_padding_bits"]   = m_rowPaddingBits;
    c["scale"]              = m_scale;
    c["width_mode"]         = widthModeKey(m_widthMode);
    c["bytes_per_row"]      = m_bytesPerRow;
    c["fixed_width"]        = m_fixedWidth;
    c["width_step"]         = m_widthStep;
    c["auto_scale"]         = m_autoScale;
    c["fit_to_window"]      = m_fitToWindow;
    c["offset"]             = qint64(m_offset);
    c["length"]             = qint64(m_length);
    c["length_mode"]        = m_lengthMode;
    c["mirror_horizontal"]  = m_mirrorH;
    c["mirror_vertical"]    = m_mirrorV;
    c["palette"]            = m_currentPaletteId;

    QJsonObject pp;
    for (auto it = m_paletteParams.constBegin();
         it != m_paletteParams.constEnd(); ++it) {
        const QVariantMap m = it.value().toMap();
        QJsonObject sub;
        for (auto mi = m.constBegin(); mi != m.constEnd(); ++mi)
            sub[mi.key()] = QJsonValue::fromVariant(mi.value());
        pp[it.key()] = sub;
    }
    c["palette_params"] = pp;
    return c;
}

bool MainWindow::applyConfigFromJson(const QJsonObject &c)
{
    if (c.isEmpty()) {
        statusBar()->showMessage(tr("The .b0s settings file is empty."), 7000);
        return false;
    }

    m_mirrorH = c.value("mirror_horizontal").toBool(m_mirrorH);
    m_mirrorV = c.value("mirror_vertical").toBool(m_mirrorV);
    if (m_mirrorHAct) {
        QSignalBlocker b(m_mirrorHAct);
        m_mirrorHAct->setChecked(m_mirrorH);
    }
    if (m_mirrorVAct) {
        QSignalBlocker b(m_mirrorVAct);
        m_mirrorVAct->setChecked(m_mirrorV);
    }

    const QJsonObject pp = c.value("palette_params").toObject();
    for (auto it = pp.constBegin(); it != pp.constEnd(); ++it) {
        QVariantMap m;
        const QJsonObject sub = it.value().toObject();
        for (auto si = sub.constBegin(); si != sub.constEnd(); ++si)
            m[si.key()] = si.value().toVariant();
        const QString &paletteId = it.key();
        if (m_paletteManager->contains(paletteId))
            m_paletteParams[paletteId] = m;
    }

    QVariantMap vmap;
    for (auto it = c.constBegin(); it != c.constEnd(); ++it)
        vmap[it.key()] = it.value().toVariant();
    if (m_controls)
        m_controls->setConfig(vmap);

    const QString paletteId = c.value("palette").toString(m_currentPaletteId);
    if (m_paletteManager->contains(paletteId))
        onPaletteChanged(paletteId);

    return true;
}

void MainWindow::saveCurrentConfig()
{
    if (!m_source.isOpen())
        return;
    const QString cfgPath = configPathForFile(m_source.sourcePath());
    if (cfgPath.isEmpty())
        return;
    QFile f(cfgPath);
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(QJsonDocument(configToJson()).toJson(QJsonDocument::Indented));
    f.close();
}

void MainWindow::loadConfigFromFile()
{
    if (!m_source.isOpen())
        return;
    const QString cfgPath = configPathForFile(m_source.sourcePath());
    if (cfgPath.isEmpty() || !QFile::exists(cfgPath))
        return;

    QFile f(cfgPath);
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;
    applyConfigFromJson(doc.object());
}

void MainWindow::resetConfig()
{
    if (!m_source.isOpen())
        return;
    if (m_source.sourcePath().isEmpty())
        return;
    applyDefaultConfig();
}

void MainWindow::resetDisplaySettings()
{
    applyDefaultConfig();
}

// =========================================================================
//  Default config
// =========================================================================

void MainWindow::applyDefaultConfig()
{
    QVariantMap cfg;
    cfg["palette"]          = "heat";
    cfg["scale"]            = 1;
    cfg["auto_scale"]       = false;
    cfg["fit_to_window"]    = false;
    cfg["width_mode"]       = "fixed";
    cfg["bytes_per_row"]    = 16;
    cfg["fixed_width"]      = 256;
    cfg["width_step"]       = 8;
    cfg["offset"]           = 0;
    cfg["length"]           = 0;
    cfg["length_mode"]      = false;
    cfg["bpp_mode"]         = 8;
    cfg["bit_order"]        = "msb";
    cfg["pixel_order"]      = "msb";
    cfg["row_padding_bits"] = 0;

    m_mirrorH = false;
    m_mirrorV = false;
    if (m_mirrorHAct) {
        QSignalBlocker b(m_mirrorHAct);
        m_mirrorHAct->setChecked(false);
    }
    if (m_mirrorVAct) {
        QSignalBlocker b(m_mirrorVAct);
        m_mirrorVAct->setChecked(false);
    }

    if (m_controls)
        m_controls->setConfig(cfg);

    const QString paletteId = m_controls
        ? m_controls->currentPaletteId()
        : QStringLiteral("heat");
    onPaletteChanged(paletteId.isEmpty() ? QStringLiteral("heat") : paletteId);
}

void MainWindow::openFileFromCommandLine(const QString &path)
{
    if (path.isEmpty())
        return;
    loadFile(path);
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

void MainWindow::onDebugInfoToggled(bool enabled)
{
    m_showDebugInfo = enabled;
    if (m_debugGroup)
        m_debugGroup->setVisible(enabled);
    if (enabled)
        updateDebugInfo();
}

void MainWindow::updateDebugInfo()
{
    if (!m_showDebugInfo || !m_debugLabel)
        return;

    auto fmtSize = [](qint64 b) -> QString {
        if (b >= 1024LL * 1024 * 1024)
            return QString::number(b / (1024.0 * 1024.0 * 1024.0), 'f', 2)
                   + " " + MainWindow::tr("GB");
        if (b >= 1024LL * 1024)
            return QString::number(b / MW_BYTES_PER_MB, 'f', 1)
                   + " " + MainWindow::tr("MB");
        if (b >= MW_BYTES_PER_KB)
            return QString::number(b / double(MW_BYTES_PER_KB), 'f', 1)
                   + " " + MainWindow::tr("KB");
        return QString::number(b) + " " + MainWindow::tr("B");
    };

    QString mode;
    if (!m_source.isOpen())
        mode = "none";
    else if (!m_xorLabel.isEmpty())
        mode = "xor";
    else if (!m_source.sourcePath().isEmpty())
        mode = m_source.mapped() ? "mmap" : "memory";
    else
        mode = "demo";

    const QString widthModeStr = widthModeName(m_widthMode);

    qint64 cacheB     = 0;
    int    activeT    = 0;
    int    cacheLimit = 0;
    if (m_canvas && m_canvas->tileManager()) {
        cacheB     = m_canvas->tileManager()->cacheBytes();
        activeT    = m_canvas->tileManager()->activeTasks();
        cacheLimit = m_canvas->tileManager()->cacheLimitMb();
    }

    QStringList L;

    // ────── Источник ──────
    L << QString("SOURCE:");
    L << "file_mode = " + mode;
    L << "file_size = " + QString::number(m_fileSize) + " (" + fmtSize(m_fileSize) + ")";
    L << "offset = " + QString::number(m_offset);
    L << "length = " + QString::number(m_length);
    L << "load_into_memory = " + QString(m_loadIntoMemory ? "true" : "false");
    L << "source_path = " + (m_source.sourcePath().isEmpty()
                                ? QStringLiteral("(none)")
                                : m_source.sourcePath());
    if (!m_xorLabel.isEmpty())
        L << "xor_label = " + m_xorLabel;

    // ────── Формат пикселей ──────
    L << QString();
    L << QString("PIXEL_FORMAT:");
    L << "bpp_mode = " + QString::number(m_bppMode);
    L << "is_bitpacked = " + QString(isBitpacked() ? "true" : "false");
    L << "bytes_per_pixel = " + QString::number(bytesPerPixel());
    L << "bit_order = " + m_bitOrder;
    L << "pixel_order = " + m_pixelOrder;
    L << "row_padding_bits = " + QString::number(m_rowPaddingBits);

    // ────── Геометрия ──────
    L << QString();
    L << QString("GEOMETRY:");
    L << "width_mode = " + widthModeStr;
    L << "fixed_width = " + QString::number(m_fixedWidth);
    L << "width_step = " + QString::number(m_widthStep);
    L << "bytes_per_row = " + QString::number(m_bytesPerRow);
    L << "image_size = " + QString::number(m_imageWidth) + " × " + QString::number(m_imageHeight);
    L << "viewport_rect = " + QString("%1,%2  %3×%4")
            .arg(m_viewportRect.x()).arg(m_viewportRect.y())
            .arg(m_viewportRect.width()).arg(m_viewportRect.height());

    // ────── Вид / зум ──────
    L << QString();
    L << QString("VIEW/ZOOM:");
    L << "scale = " + QString::number(m_scale);
    L << "downsample = " + QString::number(m_downsample);
    L << "fit_to_window = " + QString(m_fitToWindow ? "true" : "false");
    L << "auto_scale = " + QString(m_autoScale ? "true" : "false");
    L << "mirror_h = " + QString(m_mirrorH ? "true" : "false");
    L << "mirror_v = " + QString(m_mirrorV ? "true" : "false");
    L << "hover_addr = " + (m_hoverAddr < 0
                               ? QStringLiteral("-1 (none)")
                               : QString::number(m_hoverAddr));

    // ────── Тайлы / кэш ──────
    L << QString();
    L << QString("TILES/CACHE:");
    L << "tile_size = " + QString::number(m_tileSize);
    L << "tile_prefetch_margin = " + QString::number(m_tilePrefetchMargin);
    L << "cache_limit_mb = " + QString::number(cacheLimit);
    L << "cache_bytes = " + QString::number(cacheB) + " (" + fmtSize(cacheB) + ")";
    L << "active_tasks = " + QString::number(activeT);
    L << "perf_slider_value = " + QString::number(m_perfSliderValue);

    // ────── Палитра / гистограмма ──────
    L << QString();
    L << QString("PALETTE:");
    L << "palette = " + (m_currentPaletteId.isEmpty()
                            ? QStringLiteral("(none)")
                            : m_currentPaletteId);
    L << "palette_colors = " + QString::number(m_currentPalette.size());
    L << "palette_params_count = " + QString::number(m_paletteParams.size());
    L << "histogram_valid = " + QString(m_byteHistogramValid ? "true" : "false");
    L << "auto_apply_palette = " + QString(m_autoApplyPalette ? "true" : "false");

    // ────── Прочее ──────
    L << QString();
    L << QString("OTHER:");
    L << "theme = " + QString(m_theme && m_theme->isDark() ? "dark" : "light");
    L << "busy_png_export = " + QString(m_busyPngExport ? "true" : "false");
    L << "byte_tooltip = " + QString(
            (m_byteInfo && m_byteInfo->isPopupEnabled()) ? "true" : "false");
    L << "show_debug_info = " + QString(m_showDebugInfo ? "true" : "false");
    L << "recent_files = " + QString::number(m_recentFiles.size());

    m_debugLabel->setText(L.join("<br>"));
}

void MainWindow::restoreWindowGeometry()
{
    bool restored = false;

    if (!m_startupGeometryB64.isEmpty()) {
        const QByteArray geo =
            QByteArray::fromBase64(m_startupGeometryB64.toLatin1());
        if (!geo.isEmpty())
            restored = restoreGeometry(geo);
    }

    if (!restored && !m_startupNormalGeometry.isEmpty()) {
        const QStringList parts = m_startupNormalGeometry.split(',');
        if (parts.size() == 4) {
            bool okX, okY, okW, okH;
            const int x = parts[0].toInt(&okX);
            const int y = parts[1].toInt(&okY);
            const int w = parts[2].toInt(&okW);
            const int h = parts[3].toInt(&okH);
            if (okX && okY && okW && okH && w > 0 && h > 0)
                setGeometry(x, y, w, h);
        }
    }

    if (m_startupFullscreen)
        showFullScreen();
    else if (m_startupMaximized)
        showMaximized();

    const QRect frame = frameGeometry();
    bool onScreen = false;
    for (const QScreen *scr : QGuiApplication::screens()) {
        if (scr->availableGeometry().intersects(frame)) {
            onScreen = true;
            break;
        }
    }
    if (!onScreen) {
        const QRect avail =
            QGuiApplication::primaryScreen()->availableGeometry();
        const QSize sz = size();
        move(avail.center() - QPoint(sz.width() / 2, sz.height() / 2));
    }
}

void MainWindow::onTrainingButtonClicked()
{
    startTraining();
}

void MainWindow::startTraining()
{
    // 1. Загружаем обучающий файл
    const QByteArray data = DemoBuilder::buildTrainingDemo();
    m_source.setData(data);
    m_fileSize = data.size();
    m_xorLabel.clear();
    m_trainingMode = true;
    m_offset = 0;
    m_length = 0;
    m_lengthMode = false;
    invalidateByteHistogram();

    // 2. Ставим заведомо неправильный конфиг
    applyDefaultConfig();

    if (m_controls) {
        QVariantMap cfg = m_controls->currentConfig();
        cfg["bpp_mode"]         = 8;
        cfg["bit_order"]        = "msb";
        cfg["pixel_order"]      = "msb";
        cfg["row_padding_bits"] = 0;
        cfg["palette"]          = "heat";
        cfg["width_mode"]       = "fixed";
        cfg["fixed_width"]      = 256;
        cfg["scale"]            = 1;
        cfg["auto_scale"]       = false;
        cfg["fit_to_window"]    = false;
        cfg["offset"]           = 0;
        cfg["length"]           = 0;
        cfg["length_mode"]      = false;
        m_controls->setConfig(cfg);
    }

    m_mirrorH = false;
    m_mirrorV = false;
    if (m_mirrorHAct) {
        QSignalBlocker b(m_mirrorHAct);
        m_mirrorHAct->setChecked(false);
    }
    if (m_mirrorVAct) {
        QSignalBlocker b(m_mirrorVAct);
        m_mirrorVAct->setChecked(false);
    }

    // 3. Перестраиваем
    if (m_canvas) {
        m_canvas->clearHover();
        if (m_canvas->tileManager())
            m_canvas->tileManager()->clearCache();
    }
    recomputeGeometry();
    refreshTitle();
    if (m_canvas)
        m_canvas->rebuild();
    if (m_panorama)
        m_panorama->updatePanorama();

    // 4. UI
    if (m_trainingBtn)
        m_trainingBtn->setVisible(false);
    if (m_hintLabel)
        m_hintLabel->setText(DemoBuilder::trainingHintHtml());

    statusBar()->showMessage(
        tr("Tutorial: adjust the display settings to reveal the image."),
        8000);
}