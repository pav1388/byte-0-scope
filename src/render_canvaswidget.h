// Byte-0-scope - binary data visualizer
// File: render_canvaswidget.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "render_tilemanager.h"

#include <QWidget>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QTimer>
#include <QSet>

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

    void rebuild();  // сброс кэша, пересчёт размеров, стоп prefetch
    void clearHover();
    // Пересчитать, что находится под курсором (после смены геометрии/зеркал).
    void refreshHover();
    // Смещение (0..7) выбранного пикселя внутри байта для 1–4 bpp; для
    // остальных форматов 0.
    int  selectedBitOffset() const
    {
        return m_locked ? m_lockedBitOffset : m_hoverBitOffset;
    }
    void setForceRedraw() { m_forceRedraw = true; update(); }
    void setRegionPickMode(bool enabled);
    bool regionPickMode() const { return m_regionPickMode; }
    void clearLock();
    bool hasLockedPixel() const
    {
        return m_locked && m_lockedAddr >= 0 && m_lockedSourcePixelValid;
    }
    QPoint lockedSourcePixelPosition() const { return m_lockedSourcePixel; }

    QImage renderToImage(const QSize &targetSize);  // для PNG-экспорта
    QSize  scaledSize() const;

    TileManager *tileManager() const { return m_tiles; }

signals:
    void hoverChanged(qint64 addr);  // -1 = вне картинки
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
    qint64        byteAddressAt(const QPoint &pos,
                                QPoint *sourcePixel = nullptr,
                                int *bitOffset = nullptr) const;
    QSet<TileKey> visibleTiles() const;
    void          displayedRegion(quint64 *offset, quint64 *length) const;
    bool          pixelForAddress(qint64 address, int bitOffset,
                                  int *x, int *y) const;

    MainWindow *m_main = nullptr;

    int m_imageWidth  = 0;
    int m_imageHeight = 0;
    int m_scale       = 1;
    int m_downsample  = 1;

    qint64 m_hoverAddr  = -1;
    qint64 m_lockedAddr = -1;
    int    m_hoverBitOffset  = 0;
    int    m_lockedBitOffset = 0;
    bool   m_locked     = false;
    QPoint m_lockedSourcePixel;
    bool   m_lockedSourcePixelValid = false;

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
    QRect   m_snapshotRect;
    bool    m_snapshotActive = false;
    bool    allVisibleTilesReady() const;
};
