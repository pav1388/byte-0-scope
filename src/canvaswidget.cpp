// Byte-0-scope - binary data visualizer
// File: canvaswidget.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "canvaswidget.h"
#include "mainwindow.h"
#include "bitpackedreader.h"
#include "pixeldecoder.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QRegion>
#include <QWheelEvent>

// --- Настройки модуля CanvasWidget (префикс CW_) ---

#define CW_HOVER_DELAY_MS        15
#define CW_WHEEL_DEBOUNCE_MS     150
#define CW_PREFETCH_DELAY_MS     50
#define CW_MIN_TILE_SIZE         16
#define CW_MARKER_PEN_WIDTH      2

CanvasWidget::CanvasWidget(MainWindow *parent)
    : QWidget(parent), m_main(parent)
{
    setMouseTracking(true);
    setMinimumSize(1, 1);
    setFocusPolicy(Qt::StrongFocus);
    m_tiles = new TileManager(this);
    connect(m_tiles, &TileManager::tileReady, this,
            [this](const TileKey &) { update(); });

    m_hoverTimer = new QTimer(this);
    m_hoverTimer->setSingleShot(true);
    connect(m_hoverTimer, &QTimer::timeout, this, [this]() {
        if (m_locked)
            return;
        const qint64 a = byteAddressAt(m_pendingHoverPos);
        if (a == m_hoverAddr)
            return;

        m_hoverAddr = a;
        emit hoverChanged(a);
        update();
    });

    m_wheelDebounceTimer = new QTimer(this);
    m_wheelDebounceTimer->setSingleShot(true);
    connect(m_wheelDebounceTimer, &QTimer::timeout,
            this, &CanvasWidget::applyWheelZoom);

    m_prefetchTimer = new QTimer(this);
    m_prefetchTimer->setSingleShot(true);
    connect(m_prefetchTimer, &QTimer::timeout,
            this, &CanvasWidget::prefetchTiles);
}

CanvasWidget::~CanvasWidget()
{
    delete m_tiles;
}

void CanvasWidget::setRegionPickMode(bool enabled)
{
    if (m_regionPickMode == enabled)
        return;
    m_regionPickMode = enabled;
    if (enabled)
        setCursor(Qt::CrossCursor);
    else
        unsetCursor();
}

void CanvasWidget::clearLock()
{
    if (!m_locked)
        return;
    m_locked = false;
    m_lockedAddr = -1;
    emit lockedChanged(-1);
    update();
}

void CanvasWidget::rebuild()
{
    if (isVisible() && width() > 0 && height() > 0) {
        m_frozenSnapshot = grab().toImage();
        m_snapshotActive = !m_frozenSnapshot.isNull();
    } else {
        m_frozenSnapshot = QImage();
        m_snapshotActive = false;
    }

    m_tiles->clearCache();
    if (m_prefetchTimer)
        m_prefetchTimer->stop();
    m_lastPrefetchTiles.clear();

    if (!m_main) {
        update();
        return;
    }

    m_imageWidth  = m_main->imageWidth();
    m_imageHeight = m_main->imageHeight();
    m_scale       = m_main->scale();
    m_downsample  = m_main->downsample();

    setFixedSize(scaledSize());
    m_forceRedraw = true;
    update();
}

QSize CanvasWidget::scaledSize() const
{
    const int s  = qMax(1, m_scale);
    const int ds = qMax(1, m_downsample);
    return QSize(qMax(1, (m_imageWidth  * s) / ds),
                 qMax(1, (m_imageHeight * s) / ds));
}

bool CanvasWidget::allVisibleTilesReady() const
{
    if (!m_main)
        return true;
    const QSet<TileKey> vis = visibleTiles();
    for (const TileKey &k : vis) {
        if (m_tiles->getTile(k.tx, k.ty).isNull())
            return false;
    }
    return true;
}

void CanvasWidget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    const QRegion dirtyRegion = event->region();
    const QVector<QRect> dirtyRects = dirtyRegion.rects();
    for (const QRect &dirtyRect : dirtyRects)
        p.fillRect(dirtyRect, palette().window());
    // Тайлы при ds==1 хранятся в нативном размере — растягиваем при
    // рисовании (nearest-neighbor, как FastTransformation).
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    if (m_imageWidth <= 0 || m_imageHeight <= 0 || !m_main)
        return;

    if (m_forceRedraw) {
        m_tiles->clearCache();
        m_forceRedraw = false;
    }

    if (m_snapshotActive && !m_frozenSnapshot.isNull()) {
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
        p.drawImage(rect(), m_frozenSnapshot);
    }

    const QSet<TileKey> vis = visibleTiles();
    m_tiles->setVisibleTiles(vis);

    if (vis != m_lastPrefetchTiles) {
        m_lastPrefetchTiles = vis;
        if (!m_prefetchTimer->isActive())
            m_prefetchTimer->start(CW_PREFETCH_DELAY_MS);
    }

    const int ts = qMax(CW_MIN_TILE_SIZE, m_main->tileSize());
    const int s  = qMax(1, m_scale);
    const int ds = qMax(1, m_downsample);

    for (const TileKey &k : vis) {
        QImage img = m_tiles->getTile(k.tx, k.ty);
        if (img.isNull())
            continue;

        // Позиция: native (k.tx * ts) → canvas (умножить на s, делить на ds).
        const int tx = (k.tx * ts * s) / ds;
        const int ty = (k.ty * ts * s) / ds;

        // Размер: тайл уже ds-масштабирован в renderTile, поэтому
        // остаётся умножить только на s.
        const int tw = qMax(1, img.width()  * s);
        const int th = qMax(1, img.height() * s);
        const QRect tileRect(tx, ty, tw, th);

        // При системной перерисовке (в частности, во время перемещения
        // окна) не обрабатываем тайлы, не затронутые текущей областью.
        if (!dirtyRegion.intersects(tileRect))
            continue;
        p.drawImage(tileRect, img);
    }

    const qint64 addr = m_locked ? m_lockedAddr : m_hoverAddr;
    if (addr < 0)
        return;

    qint64 pixelIdx;
    if (m_main->isBitpacked())
        pixelIdx = (addr - qint64(m_main->offset())) * 8 / qMax(1, m_main->bppMode());
    else
        pixelIdx = (addr - qint64(m_main->offset())) / qMax(1, m_main->bytesPerPixel());
    if (pixelIdx < 0)
        return;

    int x = int(pixelIdx % m_imageWidth);
    int y = int(pixelIdx / m_imageWidth);
    if (m_main->mirrorHorizontal())
        x = m_imageWidth - 1 - x;
    if (m_main->mirrorVertical())
        y = m_imageHeight - 1 - y;
	
	if (m_snapshotActive && allVisibleTilesReady()) {
        m_snapshotActive = false;
        m_frozenSnapshot = QImage();
    }
	
    const int xp = (x * s) / ds;
    const int yp = (y * s) / ds;
    const int cw = qMax(1, s / ds);
    const int ch = qMax(1, s / ds);
    const QRect markerRect(xp, yp, cw, ch);
    if (!dirtyRegion.intersects(markerRect.adjusted(-2, -2, 2, 2)))
        return;

    const quint8 byteVal = m_main->readByte(addr);
    QVector<QColor> pal = m_main->currentPalette();
    int r, g, b;
    if (!pal.isEmpty()) {
        const QColor &c = pal[byteVal % pal.size()];
        r = c.red();
        g = c.green();
        b = c.blue();
    } else {
        r = g = b = byteVal;
    }
    QColor contour(qMin(255, 285 - r),
                   qMin(255, 285 - g),
                   qMin(255, 285 - b));
    p.setPen(QPen(contour, CW_MARKER_PEN_WIDTH, Qt::DashLine));
    p.drawRect(xp, yp, cw - 1, ch - 1);
}

QSet<TileKey> CanvasWidget::visibleTiles() const
{
    QSet<TileKey> out;
    if (!m_main)
        return out;
    QScrollArea *sa = m_main->scrollArea();
    if (!sa)
        return out;

    QRect visible = sa->viewport()->rect();
    visible.translate(sa->horizontalScrollBar()->value(),
                      sa->verticalScrollBar()->value());

    const int s  = qMax(1, m_scale);
    const int ds = qMax(1, m_downsample);
    const int ts = qMax(CW_MIN_TILE_SIZE, m_main->tileSize());

    const int left   = qMax(0, (visible.left()   * ds) / s);
    const int top    = qMax(0, (visible.top()    * ds) / s);
    const int right  = qMin(m_imageWidth,  (visible.right()  * ds) / s + 1);
    const int bottom = qMin(m_imageHeight, (visible.bottom() * ds) / s + 1);
    if (right <= left || bottom <= top)
        return out;

    const int startTx = qMax(0, left / ts);
    const int startTy = qMax(0, top / ts);
    const int endTx = qMin((m_imageWidth + ts - 1) / ts, right / ts + 1);
    const int endTy = qMin((m_imageHeight + ts - 1) / ts, bottom / ts + 1);

    for (int ty = startTy; ty < endTy; ++ty)
        for (int tx = startTx; tx < endTx; ++tx)
            out.insert(TileKey{tx, ty, ds});
    return out;
}

qint64 CanvasWidget::byteAddressAt(const QPoint &pos) const
{
    if (m_imageWidth <= 0 || m_imageHeight <= 0 || !m_main)
        return -1;
    const int s  = qMax(1, m_scale);
    const int ds = qMax(1, m_downsample);

    int x = (pos.x() * ds) / s;
    int y = (pos.y() * ds) / s;

    if (m_main->mirrorHorizontal())
        x = m_imageWidth - 1 - x;
    if (m_main->mirrorVertical())
        y = m_imageHeight - 1 - y;

    if (x < 0 || y < 0 || x >= m_imageWidth || y >= m_imageHeight)
        return -1;

    const qint64 pixelIdx = qint64(y) * m_imageWidth + x;
    if (m_main->isBitpacked())
        return qint64(m_main->offset()) + (pixelIdx * m_main->bppMode()) / 8;
    return qint64(m_main->offset()) + pixelIdx * qMax(1, m_main->bytesPerPixel());
}

void CanvasWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        m_dragging  = true;
        m_dragStart = e->globalPos();
        setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }

    if (e->button() == Qt::LeftButton) {
        const qint64 a = byteAddressAt(e->pos());
        if (a < 0) {
            if (m_regionPickMode) {
                setRegionPickMode(false);
                emit regionPickCancelled();
            }
            return;
        }

        // Region picking deliberately goes through the normal byte-lock signal.
        // ControlsPanel consumes lockedChanged(address), fills the requested
        // spin box and immediately calls clearLock(). This also works if a
        // different byte was already locked before entering pick mode.
        if (m_regionPickMode) {
            m_locked     = true;
            m_lockedAddr = a;
            m_hoverAddr  = a;
            emit lockedChanged(a);
            emit hoverChanged(a);
            update();
            e->accept();
            return;
        }

        if (m_locked) {
            m_locked     = false;
            m_lockedAddr = -1;
            m_hoverAddr  = a;
            emit lockedChanged(-1);
        } else {
            m_locked     = true;
            m_lockedAddr = a;
            emit lockedChanged(a);
        }
        emit hoverChanged(a);
        update();
        e->accept();
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton && m_dragging) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
        e->accept();
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging) {
        const QPoint delta = e->globalPos() - m_dragStart;
        m_dragStart = e->globalPos();
        emit scrollRequested(-delta.x(), -delta.y());
        e->accept();
        return;
    }
    m_pendingHoverPos = e->pos();
    m_hoverTimer->start(CW_HOVER_DELAY_MS);
}

void CanvasWidget::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        const int d = (e->angleDelta().y() > 0) ? +1 : -1;
        emit widthStepRequested(d);
        e->accept();
        return;
    }

    if (m_main && (m_main->fitToWindow() || m_main->autoScale())) {
        e->ignore();
        return;
    }

    const int d = (e->angleDelta().y() > 0) ? +1 : -1;
    m_wheelAccumulatedDelta += d;
    m_pendingWheelCursorPos = e->pos();
    m_wheelDebounceTimer->start(CW_WHEEL_DEBOUNCE_MS);
    e->accept();
}

void CanvasWidget::leaveEvent(QEvent *e)
{
    if (!m_locked && m_hoverAddr != -1) {
        m_hoverAddr = -1;
        emit hoverChanged(-1);
        update();
    }
    QWidget::leaveEvent(e);
}

void CanvasWidget::clearHover()
{
    m_hoverAddr  = -1;
    m_locked     = false;
    m_lockedAddr = -1;
    update();
}

// =========================================================================
//  Zoom-to-cursor
// =========================================================================
void CanvasWidget::applyWheelZoom()
{
    if (m_wheelAccumulatedDelta == 0 || !m_main)
        return;
    if (!m_main->scrollArea())
        return;

    const int totalDelta = m_wheelAccumulatedDelta;
    m_wheelAccumulatedDelta = 0;

    QScrollArea *sa = m_main->scrollArea();
    QScrollBar *h = sa->horizontalScrollBar();
    QScrollBar *v = sa->verticalScrollBar();

    const int oldScale = qMax(1, m_main->scale());
    const int oldDs    = qMax(1, m_main->downsample());
    const QPoint cursorPos = m_pendingWheelCursorPos;

    const int oldH = h->value();
    const int oldV = v->value();

    const double imgX = double(cursorPos.x()) * oldDs / double(oldScale);
    const double imgY = double(cursorPos.y()) * oldDs / double(oldScale);
    const int vpX = cursorPos.x() - oldH;
    const int vpY = cursorPos.y() - oldV;

    m_main->changeScale(totalDelta);

    const int newScale = qMax(1, m_main->scale());
    const int newDs    = qMax(1, m_main->downsample());
    if (newScale == oldScale && newDs == oldDs)
        return;

    const double newCanvasX = imgX * newScale / double(newDs);
    const double newCanvasY = imgY * newScale / double(newDs);

    int newH = qRound(newCanvasX) - vpX;
    int newV = qRound(newCanvasY) - vpY;

    h->setValue(qMax(0, qMin(newH, h->maximum())));
    v->setValue(qMax(0, qMin(newV, v->maximum())));
}

void CanvasWidget::prefetchTiles()
{
    if (m_imageWidth <= 0 || m_imageHeight <= 0 || !m_main)
        return;
    if (m_lastPrefetchTiles.isEmpty())
        return;
    m_tiles->prefetchAround(m_lastPrefetchTiles);
}

QImage CanvasWidget::renderToImage(const QSize &targetSize)
{
    if (!m_main)
        return QImage();
    if (m_imageWidth <= 0 || m_imageHeight <= 0)
        return QImage();

    QImage img(targetSize, QImage::Format_ARGB32);
    img.fill(Qt::black);

    const int s  = qMax(1, m_scale);
    const int ds = qMax(1, m_downsample);

    const qint64 regionOff = qint64(m_main->offset());
    const quint64 length   = m_main->length() > 0 ? m_main->length()
                                                  : m_main->fileSize();
    const qint64 fileSize  = qint64(m_main->fileSize());
    const qint64 regionEnd = qMin<qint64>(regionOff + qint64(length), fileSize);

    if (m_main->isBitpacked()) {
        BitPackedReader reader(m_main->bppMode(), m_main->bitOrder(),
                               m_main->rowPaddingBits(), m_main->pixelOrder());
        const int rowBytes = reader.rowBytes(m_imageWidth);
        const QVector<QRgb> lut = reader.buildLut(m_main->currentPalette());

        for (int ty = 0; ty < targetSize.height(); ++ty) {
            const int gy = (ty * ds) / s;
            if (gy < 0 || gy >= m_imageHeight)
                continue;
            const int ry = m_main->mirrorVertical() ? (m_imageHeight - 1 - gy) : gy;

            const qint64 start = regionOff + qint64(ry) * rowBytes;
            if (start >= regionEnd)
                continue;
            const qint64 end = qMin<qint64>(start + rowBytes, regionEnd);
            QByteArray raw = m_main->readRange(start, end - start);
            if (raw.size() < rowBytes)
                raw.append(QByteArray(rowBytes - raw.size(), '\0'));
            const QVector<quint8> idx = reader.extractRow(raw, m_imageWidth);

            QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(ty));
            for (int tx = 0; tx < targetSize.width(); ++tx) {
                const int gx = (tx * ds) / s;
                if (gx < 0 || gx >= m_imageWidth)
                    continue;
                const int rx = m_main->mirrorHorizontal() ? (m_imageWidth - 1 - gx) : gx;
                line[tx] = lut.value(idx.value(rx, 0), qRgb(0, 0, 0));
            }
        }
        return img;
    }

    const int bp = qMax(1, m_main->bytesPerPixel());
    const QString paletteId = m_main->currentPaletteId();
    const QVariantMap palParams = m_main->paletteParams().value(paletteId).toMap();
    const QVector<QColor> pal = m_main->currentPalette();
    const QString bitOrder  = m_main->bitOrder();
    const QString byteOrder = m_main->byteOrder();
    const bool rev = (bp == 1 && bitOrder == QLatin1String("lsb"));

    for (int ty = 0; ty < targetSize.height(); ++ty) {
        const int gy = (ty * ds) / s;
        if (gy < 0 || gy >= m_imageHeight)
            continue;
        const int ry = m_main->mirrorVertical() ? (m_imageHeight - 1 - gy) : gy;

        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(ty));
        for (int tx = 0; tx < targetSize.width(); ++tx) {
            const int gx = (tx * ds) / s;
            if (gx < 0 || gx >= m_imageWidth)
                continue;
            const int rx = m_main->mirrorHorizontal() ? (m_imageWidth - 1 - gx) : gx;

            const qint64 pix = qint64(ry) * m_imageWidth + rx;
            const qint64 off = regionOff + pix * bp;
            if (off + bp > regionEnd)
                continue;

            QByteArray px = m_main->readRange(off, bp);
            if (bp == 1) {
                quint8 v = quint8(px.at(0));
                if (rev)
                    v = PixelDecoder::reverseBits8(v);
                if (v < pal.size()) {
                    const QColor &c = pal[v];
                    line[tx] = qRgb(c.red(), c.green(), c.blue());
                } else {
                    line[tx] = qRgb(v, v, v);
                }
            } else {
                line[tx] = PixelDecoder::decodeArgb(px, paletteId,
                                                    bitOrder, byteOrder,
                                                    palParams);
            }
        }
    }
    return img;
}