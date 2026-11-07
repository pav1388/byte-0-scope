// Byte-0-scope - binary data visualizer
// File: ui_mainwindow.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMainWindow>
#include <QVector>
#include <QColor>
#include <QStringList>
#include <QVariantMap>
#include <QRect>
#include <QPoint>

#include "core_bytesource.h"
#include "definitions.h"
#include "ui_mainwindowconfig.h"

class QLabel;
class QScrollArea;
class QAction;
class QMenu;
class QSlider;
class QTimer;
class QCheckBox;
class QPushButton;
class QGroupBox;
class QJsonObject;
class CanvasWidget;
class ControlsPanel;
class ByteInfoWidget;
class PanoramaWidget;
class PaletteSettingsPanel;
class ThemeManager;
class PaletteManager;
class SettingsManager;

class MainWindow : public QMainWindow {
    Q_OBJECT
    Q_DISABLE_COPY(MainWindow)

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void openFiles(const QStringList &paths);
    bool showDebugInfo() const { return m_showDebugInfo; }
    void startTraining();

    // --- State ---
    bool    isBitpacked() const;
    int     bppMode() const         { return m_bppMode; }
    int     bytesPerPixel() const;
    int     scale() const           { return m_scale; }
    int     downsample() const      { return m_downsample; }
    quint64 offset() const          { return m_offset; }
    quint64 length() const          { return m_length; }
    bool    lengthMode() const      { return m_lengthMode; }
    quint64 fileSize() const        { return quint64(m_fileSize); }
    QString bitOrder() const        { return m_bitOrder; }
    QString byteOrder() const       { return m_byteOrder; }
    QString pixelOrder() const      { return m_pixelOrder; }
    int     rowPaddingBits() const  { return m_rowPaddingBits; }
    bool    mirrorHorizontal() const{ return m_mirrorH; }
    bool    mirrorVertical() const  { return m_mirrorV; }
    bool    fitToWindow() const     { return m_fitToWindow; }
    bool    autoScale() const       { return m_autoScale; }

    int     imageWidth() const      { return m_imageWidth; }
    int     imageHeight() const     { return m_imageHeight; }
    QRect   viewportRect() const    { return m_viewportRect; }

    int     tileSize() const           { return m_tileSize; }
    int     tilePrefetchMargin() const { return m_tilePrefetchMargin; }
    QScrollArea *scrollArea() const    { return m_centerScroll; }
    CanvasWidget *canvasWidget() const { return m_canvas; }

    QString         currentPaletteId() const { return m_currentPaletteId; }
    QVector<QColor> currentPalette() const   { return m_currentPalette; }
    QVariantMap     paletteParams() const    { return m_paletteParams; }
    PaletteManager *paletteManager() const   { return m_paletteManager; }
    ThemeManager   *themeManager() const     { return m_theme; }
    QByteArray      sourceBytes() const      { return m_source.data(); }
    MappedFilePtr   sourceMapped() const     { return m_source.mapped(); }

    quint8     readByte(qint64 off) const;
    QByteArray readRange(qint64 off, qint64 len) const;

    bool loadIntoMemory() const { return m_loadIntoMemory; }
    int  bytesPerRow() const    { return m_bytesPerRow; }

    void onPaletteChanged(const QString &id);
    void changeScale(int delta);
    void changeDownsample(int ds);
    void setPaletteParams(const QString &id, const QVariantMap &params);
    void updateBusyLabel();
    int  averageByteRepeatCount();

signals:
    void paletteChanged();

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;

    void closeEvent(QCloseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void onLanguageChanged(QAction *action);
    void openFileDialog();
    void compareFiles();
    void closeCurrentFile();
    void saveCurrentConfig();
    void loadConfigFromFile();
    void resetConfig();
    void toggleTheme();
    void clearRecent();
    void onSettingsChanged();
    void onHoverChanged(qint64 addr);
    void onLockedChanged(qint64 addr);
    void onScrollRequested(int dx, int dy);
    void onScrollTo(int x, int y);
    void onScaleDelta(int delta);
    void onWidthStep(int delta);
    void onPaletteParamsChanged(const QVariantMap &newParams);
    void onPaletteAutoApplyToggled(bool enabled);
    void updateMemoryLabel();
    void rebuildRecentMenu();
    void onMirrorChanged();
    void resetMirror();
    void onPerfSliderMoved(int value);
    void applyPerfSettings();
    void resetDisplaySettings();
    void onActiveTasksChanged(int count);
    void onPngExportFinished(const QString &path, bool success,
                             const QString &errorMessage);
    void onDebugInfoToggled(bool enabled);
    void onTrainingButtonClicked();

private:
    void buildUi();
    void buildMenus();
    void initDemo();
    void updateDebugInfo();
    void loadFile(const QString &path);
    bool loadFileXor(const QString &p1, const QString &p2);
    static QStringList inputFiles(const QStringList &paths);
    void applyDefaultConfig();
    void recomputeGeometry();
    void refreshTitle();
    void addRecent(const QString &path);
    void loadSettings();
    void saveSettings();
    int  calculateWidth() const;
    void applySquareWidthHint();
    void calculateDimensions();
    void applyFitToWindow();
    void autoFitScale();
    void centerViewportOnSourcePixel(const QPoint &sourcePixel);
    void applyPerfFromSlider(int value);
    void resetToDemo();
    void ensureByteHistogram();
    enum PngExportMode {
        ExportVisibleArea,
        ExportFullOriginal,
        ExportFullCurrentScale
    };
    void exportPng(PngExportMode mode);
    void invalidateByteHistogram() { m_byteHistogramValid = false; }

    // Palette: single point that rebuilds m_currentPalette.
    void rebuildPalette(const QVariantMap &params);

    // Region / image geometry.
    quint64 regionByteLength() const;
    int     regionPixelCount() const;
    qint64  imageHeightFor(int width) const;
    bool    computeImageSize(int *width, int *height) const;

    // Shared post-load steps.
    void leaveTrainingMode();
    void finishLoad();
    void setMirrorState(bool horizontal, bool vertical);

    // Per-file .b0s config (implemented in mainwindowconfig.cpp).
    QString     configPathForFile(const QString &filePath) const;
    QJsonObject configToJson() const;
    bool        applyConfigFromJson(const QJsonObject &cfg);

    void        restoreWindowGeometry();

    // --- Startup state ---
    QString                m_language;
    StartupDisplayDefaults m_startupDefaults;
    QString     m_startupGeometryB64;
    QString     m_startupNormalGeometry;
    bool        m_startupMaximized  = false;
    bool        m_startupFullscreen = false;
    bool        m_loadIntoMemory    = false;
    QAction    *m_loadIntoMemoryAct = nullptr;

    // --- Source / format ---
    BytesSource m_source;
    qint64      m_fileSize       = 0;
    int         m_bppMode        = CFG_DEF_BPP_MODE;
    QString     m_bitOrder       = QStringLiteral(CFG_DEF_BIT_ORDER);
    QString     m_pixelOrder     = QStringLiteral(CFG_DEF_PIXEL_ORDER);
    QString     m_byteOrder      = QStringLiteral(CFG_DEF_BYTE_ORDER);
    int         m_rowPaddingBits = 0;
    quint64     m_offset         = 0;
    quint64     m_length         = 0;
    bool        m_lengthMode     = false;

    // --- Display ---
    int  m_scale        = CFG_DEF_SCALE;
    int  m_downsample   = 1;
    int  m_widthMode    = WM_Fixed;
    int  m_bytesPerRow  = CFG_DEF_BYTES_PER_ROW;
    int  m_fixedWidth   = CFG_DEF_FIXED_WIDTH;
    int  m_widthStep    = CFG_DEF_WIDTH_STEP;
    bool m_autoScale    = false;
    bool m_fitToWindow  = false;
    bool m_mirrorH      = false;
    bool m_mirrorV      = false;
	bool m_widthFromConfig = false;
    int m_imageWidth  = 0;
    int m_imageHeight = 0;
    QRect m_viewportRect;
    qint64 m_hoverAddr  = -1;

    // --- Palette ---
    QString m_currentPaletteId;
    QVector<QColor> m_currentPalette;
    QVariantMap m_paletteParams;
    bool m_autoApplyPalette = true;

    // --- Files ---
    QString m_lastDir;
    QStringList m_recentFiles;
    QString m_xorLabel;
    qint64 m_xorDiffBytes = 0;

    // --- Performance ---
    int  m_tileSize           = 256;
    int  m_tilePrefetchMargin = 3;
    int  m_perfSliderValue    = 10;
    int  m_pendingPerfValue   = -1;
    int  m_activeTileTasks    = 0;
    bool m_busyPngExport      = false;

    // --- Histogram ---
    quint64 m_byteHistogram[256] = {0};
    bool    m_byteHistogramValid = false;

    // --- Managers ---
    ThemeManager    *m_theme          = nullptr;
    PaletteManager  *m_paletteManager = nullptr;
    SettingsManager *m_settings       = nullptr;

    // --- Widgets ---
    ControlsPanel        *m_controls        = nullptr;
    CanvasWidget         *m_canvas          = nullptr;
    ByteInfoWidget       *m_byteInfo        = nullptr;
    PanoramaWidget       *m_panorama        = nullptr;
    PaletteSettingsPanel *m_paletteSettings = nullptr;
    QScrollArea          *m_centerScroll    = nullptr;

    // --- Menus / actions ---
    QAction *m_themeAction     = nullptr;
    QMenu   *m_recentMenu      = nullptr;
    QMenu   *m_settingsMenu    = nullptr;
    QAction *m_mirrorHAct      = nullptr;
    QAction *m_mirrorVAct      = nullptr;
    QAction *m_byteTooltipAct  = nullptr;
    QAction *m_debugInfoAct    = nullptr;
    QMenu   *m_exportMenu      = nullptr;

    // --- Status / debug panels ---
    QSlider    *m_perfSlider   = nullptr;
    QLabel     *m_memoryLabel  = nullptr;
    QLabel     *m_busyLabel    = nullptr;
    QLabel     *m_hintLabel    = nullptr;
    QWidget    *m_hintContainer = nullptr;
    QGroupBox  *m_debugGroup   = nullptr;
    QLabel     *m_debugLabel   = nullptr;

    QTimer *m_memoryTimer       = nullptr;
    QTimer *m_resizeTimer       = nullptr;
    QTimer *m_perfDebounceTimer = nullptr;

    bool m_showDebugInfo = false;
    QPushButton *m_trainingBtn = nullptr;
    bool         m_trainingMode = false;
};