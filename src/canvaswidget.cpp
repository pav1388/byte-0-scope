// Byte-0-scope - binary data visualizer
// File: canvaswidget.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "canvaswidget.h"
#include "pixelmath.h"
#include "mainwindow.h"
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QRegion>
#include <limits>
#include <QWheelEvent>

// --- Настройки модуля CanvasWidget (префикс CW_) ---

#define CW_HOVER_DELAY_MS        15
#define CW_WHEEL_DEBOUNCE_MS     150
#define CW_PREFETCH_DELAY_MS     50
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
        int bitOffset = 0;
        const qint64 a = byteAddressAt(m_pendingHoverPos, nullptr, &bitOffset);
        // В режимах 1–4 bpp несколько пикселей делят один байт, поэтому
        // одного адреса для сравнения недостаточно.
        if (a == m_hoverAddr && (a < 0 || bitOffset == m_hoverBitOffset))
            return;

        m_hoverAddr = a;
        m_hoverBitOffset = (a >= 0) ? bitOffset : 0;
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
    m_lockedBitOffset = 0;
    m_lockedSourcePixel = QPoint();
    m_lockedSourcePixelValid = false;
    emit lockedChanged(-1);
    update();
}

void CanvasWidget::rebuild()
{
    // Snapshot only the currently visible part of a potentially enormous
    // canvas. QWidget::grab() without a rect would allocate an image the size
    // of the entire virtual canvas on every slider/settings change.
    m_frozenSnapshot = QImage();
    m_snapshotRect = QRect();
    m_snapshotActive = false;
    if (isVisible() && width() > 0 && height() > 0
            && m_main && m_main->scrollArea()) {
        QScrollArea *scrollArea = m_main->scrollArea();
        const QPoint scrollOrigin(
            scrollArea->horizontalScrollBar()->value(),
            scrollArea->verticalScrollBar()->value());
        const QRect viewportRect(scrollOrigin,
                                 scrollArea->viewport()->size());
        const QRect captureRect = viewportRect.intersected(rect());
        if (!captureRect.isEmpty()) {
            m_frozenSnapshot = grab(captureRect).toImage();
            if (!m_frozenSnapshot.isNull()) {
                m_snapshotRect = captureRect;
                m_snapshotActive = true;
            }
        }
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
    // rebuild() has already invalidated the cache. Do not clear it again in
    // the next paintEvent (the old code performed two generation changes).
    m_forceRedraw = false;
    update();

    // Изображение под неподвижным курсором изменилось (ширина, зеркало,
    // bpp...) — адрес и маркер нужно пересчитать, не дожидаясь движения мыши.
    refreshHover();
}

void CanvasWidget::refreshHover()
{
    if (m_locked || !underMouse() || !m_hoverTimer)
        return;
    m_pendingHoverPos = mapFromGlobal(QCursor::pos());
    m_hoverTimer->start(CW_HOVER_DELAY_MS);
}

QSize CanvasWidget::scaledSize() const
{
    if (m_imageWidth <= 0 || m_imageHeight <= 0)
        return QSize(1, 1);

    const int s = qMax(1, m_scale);
    const int ds = qMax(1, m_downsample);
    const qint64 width = qint64(tileOutputExtent(m_imageWidth, ds)) * s;
    const qint64 height = qint64(tileOutputExtent(m_imageHeight, ds)) * s;
    if (width <= 0 || height <= 0
            || width > std::numeric_limits<int>::max()
            || height > std::numeric_limits<int>::max())
        return QSize(1, 1);
    return QSize(int(width), int(height));
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
    QPainter painter(this);
    const QRegion dirtyRegion = event->region();
    const QVector<QRect> dirtyRects = dirtyRegion.rects();
    for (const QRect &dirtyRect : dirtyRects)
        painter.fillRect(dirtyRect, palette().window());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    if (m_imageWidth <= 0 || m_imageHeight <= 0 || !m_main || !m_tiles)
        return;

    if (m_forceRedraw) {
        m_tiles->clearCache();
        m_forceRedraw = false;
    }

    if (m_snapshotActive && !m_frozenSnapshot.isNull())
        painter.drawImage(m_snapshotRect, m_frozenSnapshot);

    const QSet<TileKey> visible = visibleTiles();
    m_tiles->setVisibleTiles(visible);

    if (m_snapshotActive && allVisibleTilesReady()) {
        m_snapshotActive = false;
        m_frozenSnapshot = QImage();
        m_snapshotRect = QRect();
    }

    if (visible != m_lastPrefetchTiles) {
        m_lastPrefetchTiles = visible;
        if (m_prefetchTimer && !m_prefetchTimer->isActive())
            m_prefetchTimer->start(CW_PREFETCH_DELAY_MS);
    }

    const int sourceTileSize = normalizedTileSourceSize(m_main->tileSize());
    const int outputTileSize = tileOutputSize(sourceTileSize, m_downsample);
    const int scale = qMax(1, m_scale);
    if (outputTileSize <= 0)
        return;

    for (const TileKey &key : visible) {
        const QImage image = m_tiles->getTile(key.tx, key.ty);
        if (image.isNull())
            continue;

        const qint64 x = qint64(key.tx) * outputTileSize * scale;
        const qint64 y = qint64(key.ty) * outputTileSize * scale;
        const qint64 width = qint64(image.width()) * scale;
        const qint64 height = qint64(image.height()) * scale;
        if (x < 0 || y < 0 || width <= 0 || height <= 0
                || x > std::numeric_limits<int>::max()
                || y > std::numeric_limits<int>::max()
                || width > std::numeric_limits<int>::max()
                || height > std::numeric_limits<int>::max()
                || x + width > std::numeric_limits<int>::max()
                || y + height > std::numeric_limits<int>::max())
            continue;

        const QRect tileRect = QRect(int(x), int(y), int(width), int(height));
        if (!dirtyRegion.intersects(tileRect))
            continue;
        // When a frozen snapshot is used during rebuild, clear the old tile
        // before alpha-compositing the new one. Otherwise transparent pixels
        // are composited over the previous image and become too opaque.
        if (m_snapshotActive)
            painter.fillRect(tileRect, palette().window());
        painter.drawImage(tileRect, image);
    }

    const qint64 address = m_locked ? m_lockedAddr : m_hoverAddr;
    if (address < 0)
        return;

    const int bitOffset = m_locked ? m_lockedBitOffset : m_hoverBitOffset;

    // Пиксель в координатах исходного изображения...
    int x = -1;
    int y = -1;
    if (!pixelForAddress(address, bitOffset, &x, &y))
        return;

    // ...и его положение на экране (зеркала применяются только здесь).
    if (m_main->mirrorHorizontal())
        x = m_imageWidth - 1 - x;
    if (m_main->mirrorVertical())
        y = m_imageHeight - 1 - y;

    const int ds = qMax(1, m_downsample);
    const qint64 xp64 = qint64(x / ds) * scale;
    const qint64 yp64 = qint64(y / ds) * scale;
    if (xp64 < 0 || yp64 < 0
            || xp64 > std::numeric_limits<int>::max() - scale - 2
            || yp64 > std::numeric_limits<int>::max() - scale - 2)
        return;
    const int xp = int(xp64);
    const int yp = int(yp64);
    // One output-grid pixel is enlarged by exactly the integer zoom factor.
    const QRect markerRect(xp, yp, scale, scale);
    if (!dirtyRegion.intersects(markerRect.adjusted(-2, -2, 2, 2)))
        return;

    // Значение для цвета контура: для 1–4 bpp — значение выбранного пикселя,
    // а не всего байта.
    quint8 byteValue = m_main->readByte(address);
    if (m_main->isBitpacked()) {
        const QByteArray raw = m_main->readRange(address, 2);
        byteValue = quint8(PixelMath::bitpackedValue(
            reinterpret_cast<const uchar *>(raw.constData()), raw.size(),
            bitOffset, qBound(1, m_main->bppMode(), 8),
            m_main->bitOrder() == QLatin1String("lsb"),
            m_main->pixelOrder() == QLatin1String("lsb")));
    }
    const QVector<QColor> palette = m_main->currentPalette();
    int r = byteValue, g = byteValue, b = byteValue;
    if (!palette.isEmpty()) {
        const QColor &color = palette[byteValue % palette.size()];
        r = color.red();
        g = color.green();
        b = color.blue();
    }

    const QColor contour(qMin(255, 285 - r),
                         qMin(255, 285 - g),
                         qMin(255, 285 - b));
    painter.setPen(QPen(contour, CW_MARKER_PEN_WIDTH, Qt::DashLine));
    painter.drawRect(xp, yp, qMax(0, scale - 1), qMax(0, scale - 1));
}

QSet<TileKey> CanvasWidget::visibleTiles() const
{
    QSet<TileKey> result;
    if (!m_main || !m_main->scrollArea()
            || m_imageWidth <= 0 || m_imageHeight <= 0)
        return result;

    QScrollArea *scrollArea = m_main->scrollArea();
    const QRect viewport = scrollArea->viewport()->rect();
    const qint64 viewLeft = qint64(viewport.left())
                          + scrollArea->horizontalScrollBar()->value();
    const qint64 viewTop = qint64(viewport.top())
                         + scrollArea->verticalScrollBar()->value();
    const qint64 viewRight = viewLeft + viewport.width(); // half-open edge
    const qint64 viewBottom = viewTop + viewport.height();

    const int scale = qMax(1, m_scale);
    const int ds = qMax(1, m_downsample);
    const int outputWidth = tileOutputExtent(m_imageWidth, ds);
    const int outputHeight = tileOutputExtent(m_imageHeight, ds);
    const int outputTileSize = tileOutputSize(
        normalizedTileSourceSize(m_main->tileSize()), ds);
    if (outputWidth <= 0 || outputHeight <= 0 || outputTileSize <= 0)
        return result;

    // Clip in screen coordinates first, then convert the half-open rectangle
    // to output pixels. This is consistent with drawing each output pixel at
    // [pixel*scale, (pixel+1)*scale).
    const qint64 canvasWidth = qint64(outputWidth) * scale;
    const qint64 canvasHeight = qint64(outputHeight) * scale;
    const qint64 left = qBound<qint64>(0, viewLeft, canvasWidth);
    const qint64 top = qBound<qint64>(0, viewTop, canvasHeight);
    const qint64 right = qBound<qint64>(0, viewRight, canvasWidth);
    const qint64 bottom = qBound<qint64>(0, viewBottom, canvasHeight);
    if (right <= left || bottom <= top)
        return result;

    const qint64 outputLeft = left / scale;
    const qint64 outputTop = top / scale;
    const qint64 outputRight = (right + scale - 1) / scale;
    const qint64 outputBottom = (bottom + scale - 1) / scale;

    const qint64 firstTileX = outputLeft / outputTileSize;
    const qint64 firstTileY = outputTop / outputTileSize;
    const qint64 lastTileX = (outputRight - 1) / outputTileSize;
    const qint64 lastTileY = (outputBottom - 1) / outputTileSize;

    for (qint64 ty = firstTileY; ty <= lastTileY; ++ty) {
        for (qint64 tx = firstTileX; tx <= lastTileX; ++tx) {
            if (tx > std::numeric_limits<int>::max()
                    || ty > std::numeric_limits<int>::max())
                continue;
            result.insert(TileKey(int(tx), int(ty), ds));
        }
    }
    return result;
}

// Диапазон байтов файла, отображаемый на холсте: [offset, offset + length).
// Единственное место, где регион пересекается с размером файла.
void CanvasWidget::displayedRegion(quint64 *offset, quint64 *length) const
{
    *offset = 0;
    *length = 0;
    if (!m_main)
        return;

    const quint64 offsetValue = m_main->offset();
    if (offsetValue > quint64(std::numeric_limits<qint64>::max()))
        return;

    *offset = offsetValue;
    *length = PixelMath::effectiveRegionLength(m_main->fileSize(), offsetValue,
                                               m_main->length());
}

// Пиксель в координатах ИСХОДНОГО изображения (без учёта зеркал), которому
// принадлежит бит bitOffset (0..7) байта с адресом address. Для 1–4 bpp
// именно смещение в битах выбирает пиксель внутри байта. Обратная операция к
// byteAddressAt().
bool CanvasWidget::pixelForAddress(qint64 address, int bitOffset,
                                   int *outX, int *outY) const
{
    quint64 offsetValue = 0;
    quint64 regionLength = 0;
    displayedRegion(&offsetValue, &regionLength);
    if (!m_main || address < 0 || regionLength == 0
            || quint64(address) < offsetValue
            || quint64(address) - offsetValue >= regionLength)
        return false;

    const qint64 relative = qint64(quint64(address) - offsetValue);
    int x = -1;
    int y = -1;
    if (m_main->isBitpacked()) {
        const int bitsPerPixel = qBound(1, m_main->bppMode(), 8);
        const qint64 rowBytes = PixelMath::bitpackedRowBytes(
            m_imageWidth, bitsPerPixel, m_main->rowPaddingBits());
        if (rowBytes <= 0)
            return false;
        // Абсолютная позиция бита от начала региона.
        const qint64 bitPos = relative * 8 + qBound(0, bitOffset, 7);
        const qint64 rowBits = rowBytes * 8;
        const qint64 row = bitPos / rowBits;
        const qint64 column = (bitPos % rowBits) / bitsPerPixel;
        // column >= width — это биты паддинга в конце строки.
        if (row >= m_imageHeight || column >= m_imageWidth)
            return false;
        x = int(column);
        y = int(row);
    } else {
        const int bytesPerPixel = qBound(1, m_main->bytesPerPixel(), 8);
        const qint64 pixelIndex = relative / bytesPerPixel;
        const qint64 totalPixels = qint64(m_imageWidth) * m_imageHeight;
        if (pixelIndex >= totalPixels)
            return false;
        x = int(pixelIndex % m_imageWidth);
        y = int(pixelIndex / m_imageWidth);
    }

    *outX = x;
    *outY = y;
    return true;
}

qint64 CanvasWidget::byteAddressAt(const QPoint &pos, QPoint *sourcePixel,
                                   int *bitOffset) const
{
    if (sourcePixel)
        *sourcePixel = QPoint(-1, -1);
    if (bitOffset)
        *bitOffset = 0;

    if (m_imageWidth <= 0 || m_imageHeight <= 0 || !m_main
            || pos.x() < 0 || pos.y() < 0)
        return -1;

    quint64 offsetValue = 0;
    quint64 regionLength = 0;
    displayedRegion(&offsetValue, &regionLength);
    if (regionLength == 0)
        return -1;

    const int scale = qMax(1, m_scale);
    const int ds = qMax(1, m_downsample);
    // Use the output pixel actually painted under the cursor. This keeps
    // hit-testing aligned with the global downsample lattice.
    qint64 x64 = qint64(pos.x() / scale) * ds;
    qint64 y64 = qint64(pos.y() / scale) * ds;
    if (x64 < 0 || y64 < 0 || x64 >= m_imageWidth || y64 >= m_imageHeight)
        return -1;

    int x = int(x64);
    int y = int(y64);
    if (m_main->mirrorHorizontal())
        x = m_imageWidth - 1 - x;
    if (m_main->mirrorVertical())
        y = m_imageHeight - 1 - y;

    // Keep the exact source pixel selected by the mouse. For bit-packed
    // formats several pixels may share one byte, so the byte address alone
    // is not sufficient to restore the user's viewport position later.
    if (sourcePixel)
        *sourcePixel = QPoint(x, y);

    qint64 relative = 0;
    if (m_main->isBitpacked()) {
        const int bpp = qBound(1, m_main->bppMode(), 8);
        const qint64 rowBytes = PixelMath::bitpackedRowBytes(
            m_imageWidth, bpp, m_main->rowPaddingBits());
        if (rowBytes <= 0)
            return -1;
        const qint64 bitInRow = qint64(x) * bpp;
        relative = qint64(y) * rowBytes + bitInRow / 8;
        // Какой именно пиксель (бит/ниббл) внутри байта под курсором.
        if (bitOffset)
            *bitOffset = int(bitInRow % 8);
    } else {
        const int bytesPerPixel = qBound(1, m_main->bytesPerPixel(), 8);
        const qint64 pixelIndex = qint64(y) * m_imageWidth + x;
        if (pixelIndex > std::numeric_limits<qint64>::max() / bytesPerPixel)
            return -1;
        relative = pixelIndex * bytesPerPixel;
    }

    if (relative < 0 || quint64(relative) >= regionLength)
        return -1;
    return qint64(offsetValue + quint64(relative));
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
        QPoint sourcePixel;
        int bitOffset = 0;
        const qint64 a = byteAddressAt(e->pos(), &sourcePixel, &bitOffset);
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
            m_lockedBitOffset = bitOffset;
            m_lockedSourcePixel = sourcePixel;
            m_lockedSourcePixelValid = true;
            m_hoverAddr  = a;
            m_hoverBitOffset = bitOffset;
            emit lockedChanged(a);
            emit hoverChanged(a);
            update();
            e->accept();
            return;
        }

        if (m_locked) {
            m_locked     = false;
            m_lockedAddr = -1;
            m_lockedBitOffset = 0;
            m_lockedSourcePixel = QPoint();
            m_lockedSourcePixelValid = false;
            m_hoverAddr  = a;
            m_hoverBitOffset = bitOffset;
            emit lockedChanged(-1);
        } else {
            m_locked     = true;
            m_lockedAddr = a;
            m_lockedBitOffset = bitOffset;
            m_lockedSourcePixel = sourcePixel;
            m_lockedSourcePixelValid = true;
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
    // Горизонтальная прокрутка (angleDelta().y() == 0) раньше трактовалась как
    // «уменьшить» — отдаём такие события родительской области прокрутки.
    const int wheel = e->angleDelta().y();
    if (wheel == 0) {
        e->ignore();
        return;
    }
    const int d = (wheel > 0) ? +1 : -1;

    if (e->modifiers() & Qt::ControlModifier) {
        emit widthStepRequested(d);
        e->accept();
        return;
    }

    if (m_main && (m_main->fitToWindow() || m_main->autoScale())) {
        e->ignore();
        return;
    }

    m_wheelAccumulatedDelta += d;
    m_pendingWheelCursorPos = e->pos();
    m_wheelDebounceTimer->start(CW_WHEEL_DEBOUNCE_MS);
    e->accept();
}

void CanvasWidget::leaveEvent(QEvent *e)
{
    if (!m_locked && m_hoverAddr != -1) {
        m_hoverAddr = -1;
        m_hoverBitOffset = 0;
        emit hoverChanged(-1);
        update();
    }
    QWidget::leaveEvent(e);
}

void CanvasWidget::clearHover()
{
    m_hoverAddr  = -1;
    m_hoverBitOffset  = 0;
    m_lockedBitOffset = 0;
    m_locked     = false;
    m_lockedAddr = -1;
    m_lockedSourcePixel = QPoint();
    m_lockedSourcePixelValid = false;
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
    if (!m_tiles)
        return QImage();
    return m_tiles->renderImage(targetSize, m_scale, m_downsample);
}
