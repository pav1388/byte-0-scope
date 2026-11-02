// Byte-0-scope - binary data visualizer
// File: canvaswidget.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>
#include <QImage>
#include <QPoint>
#include <QSize>
#include <QTimer>
#include <QSet>

#include "tilemanager.h"

class MainWindow;

class CanvasWidget : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(CanvasWidget)

public:
    explicit CanvasWidget(MainWindow *parent);
    ~CanvasWidget() override;

    int         scale()       const { return m_scale; }
    int         downsample()  const { return m_downsample; }
    int         imageWidth()  const { return m_imageWidth; }
    int         imageHeight() const { return m_imageHeight; }
    MainWindow *mainWindow()  const { return m_main; }

    void rebuild();                  // сброс кэша, пересчёт размеров, стоп prefetch
    void clearHover();
    void setForceRedraw() { m_forceRedraw = true; update(); }
    void setRegionPickMode(bool enabled);
    bool regionPickMode() const { return m_regionPickMode; }
    void clearLock();

    QImage renderToImage(const QSize &targetSize);   // для PNG-экспорта
    QSize  scaledSize() const;

    TileManager *tileManager() const { return m_tiles; }

signals:
    void hoverChanged(qint64 addr);           // -1 = вне картинки
    void lockedChanged(qint64 addr);
    void scrollRequested(int dx, int dy);
    void scaleDeltaRequested(int delta);
    void widthStepRequested(int delta);
    void regionPickCancelled();

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void leaveEvent(QEvent *e) override;

private slots:
    void applyWheelZoom();
    void prefetchTiles();

private:
    qint64        byteAddressAt(const QPoint &pos) const;
    QSet<TileKey> visibleTiles() const;

    MainWindow *m_main = nullptr;

    int m_imageWidth  = 0;
    int m_imageHeight = 0;
    int m_scale       = 1;
    int m_downsample  = 1;

    qint64 m_hoverAddr  = -1;
    qint64 m_lockedAddr = -1;
    bool   m_locked     = false;

    bool   m_regionPickMode = false;
    bool   m_dragging       = false;
    QPoint m_dragStart;

    bool m_forceRedraw = true;

    QTimer *m_hoverTimer = nullptr;
    QPoint  m_pendingHoverPos;

    QTimer *m_wheelDebounceTimer    = nullptr;
    int     m_wheelAccumulatedDelta = 0;
    QPoint  m_pendingWheelCursorPos;

    QTimer       *m_prefetchTimer = nullptr;
    QSet<TileKey> m_lastPrefetchTiles;

    TileManager *m_tiles = nullptr;
	
	QImage  m_frozenSnapshot;
    bool    m_snapshotActive = false;
    bool    allVisibleTilesReady() const;
};