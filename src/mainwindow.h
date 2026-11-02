// Byte-0-scope - binary data visualizer
// File: mainwindow.h
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

#include "bytesource.h"

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

    void openFileFromCommandLine(const QString &path);
    bool showDebugInfo() const { return m_showDebugInfo; }
	void startTraining();
	
    // --- Состояние ---
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
    void exportPng();
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
    void onDebugInfoToggled(bool enabled);
	void onTrainingButtonClicked();

private:
    void buildUi();
    void buildMenus();
    void initDemo();
    void updateDebugInfo();
    void loadFile(const QString &path);
    bool loadFileXor(const QString &p1, const QString &p2);
    void applyDefaultConfig();
    void recomputeGeometry();
    void refreshTitle();
    void addRecent(const QString &path);
    void loadSettings();
    void saveSettings();
    int  calculateWidth() const;
	void  applySquareWidthHint();
    void calculateDimensions();
    void applyFitToWindow();
    void autoFitScale();
    void applyPerfFromSlider(int value);
    void resetToDemo();
    void ensureByteHistogram();
    void invalidateByteHistogram() { m_byteHistogramValid = false; }

    QString     configPathForFile(const QString &filePath) const;
    QJsonObject configToJson() const;
    bool        applyConfigFromJson(const QJsonObject &cfg);
    void        restoreWindowGeometry();

    // --- Стартовые настройки ---
	QString m_language;
    QVariantMap m_loadedStartupConfig;
    QString     m_startupGeometryB64;
    QString     m_startupNormalGeometry;
    bool        m_startupMaximized  = false;
    bool        m_startupFullscreen = false;
    bool        m_loadIntoMemory    = false;
    QAction    *m_loadIntoMemoryAct = nullptr;

    // --- Источник / формат ---
    BytesSource m_source;
    qint64      m_fileSize       = 0;
    int         m_bppMode        = 8;
    QString     m_bitOrder       = "msb";
    QString     m_pixelOrder     = "msb";
	QString 	m_byteOrder  	 = "big";
    int         m_rowPaddingBits = 0;
    quint64     m_offset         = 0;
    quint64     m_length         = 0;
    bool        m_lengthMode     = false;

    // --- Отображение ---
    int  m_scale        = 1;
    int  m_downsample   = 1;
    int  m_widthMode    = 0;
    int  m_bytesPerRow  = 16;
    int  m_fixedWidth   = 256;
    int  m_widthStep    = 8;
    bool m_autoScale    = false;
    bool m_fitToWindow  = false;
    bool m_mirrorH      = false;
    bool m_mirrorV      = false;

    int   m_imageWidth  = 0;
    int   m_imageHeight = 0;
    QRect m_viewportRect;
    qint64 m_hoverAddr  = -1;

    // --- Палитра ---
    QString         m_currentPaletteId;
    QVector<QColor> m_currentPalette;
    QVariantMap     m_paletteParams;
    bool            m_autoApplyPalette = true;

    // --- Файлы ---
    QString     m_lastDir;
    QStringList m_recentFiles;
    QString     m_xorLabel;

    // --- Производительность ---
    int  m_tileSize           = 448;
    int  m_tilePrefetchMargin = 3;
    int  m_perfSliderValue    = 8;
    int  m_pendingPerfValue   = -1;
    int  m_activeTileTasks    = 0;
    bool m_busyPngExport      = false;

    // --- Гистограмма ---
    quint64 m_byteHistogram[256] = {0};
    bool    m_byteHistogramValid = false;

    // --- Менеджеры ---
    ThemeManager    *m_theme          = nullptr;
    PaletteManager  *m_paletteManager = nullptr;
    SettingsManager *m_settings       = nullptr;

    // --- Виджеты ---
    ControlsPanel        *m_controls        = nullptr;
    CanvasWidget         *m_canvas          = nullptr;
    ByteInfoWidget       *m_byteInfo        = nullptr;
    PanoramaWidget       *m_panorama        = nullptr;
    PaletteSettingsPanel *m_paletteSettings = nullptr;
    QScrollArea          *m_centerScroll    = nullptr;

    // --- Меню/действия ---
    QAction *m_themeAction     = nullptr;
    QMenu   *m_recentMenu      = nullptr;
    QMenu   *m_settingsMenu    = nullptr;
    QAction *m_mirrorHAct      = nullptr;
    QAction *m_mirrorVAct      = nullptr;
    QAction *m_byteTooltipAct  = nullptr;
    QAction *m_debugInfoAct    = nullptr;

    // --- Панели статуса/отладки ---
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