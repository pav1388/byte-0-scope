// Byte-0-scope - binary data visualizer
// File: tilemanager.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tilemanager.h"
#include "canvaswidget.h"
#include "mainwindow.h"
#include "bitpackedreader.h"
#include "pixeldecoder.h"
#include "pixelmath.h"

#include <QCoreApplication>
#include <QImageWriter>
#include <QSaveFile>
#include <QMetaObject>
#include <QPointer>
#include <QPainter>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>
#include <algorithm>
#include <climits>
#include <limits>
#include <utility>

namespace {

bool cancelled(const RenderContext &ctx) noexcept
{
    return ctx.cancel && ctx.cancel->isCancelled();
}

bool shouldAbort(const RenderContext &ctx, int row) noexcept
{
    return (row & 7) == 0 && cancelled(ctx);
}

bool validSourceRegion(const RenderContext &ctx, const uchar **raw,
                       qint64 *regionLength)
{
    const qint64 size = ctx.byteSize();
    const uchar *bytes = ctx.bytes();

    if (size < 0 || ctx.regionStart < 0 || ctx.regionEnd < ctx.regionStart
            || ctx.regionStart > size || ctx.regionEnd > size)
        return false;

    if (size > 0 && !bytes)
        return false;

    if (raw)
        *raw = bytes;
    if (regionLength)
        *regionLength = ctx.regionEnd - ctx.regionStart;
    return true;
}

bool sampledCoordinate(int tileSourceOrigin, int localOutputCoordinate,
                       int downsample, int sourceExtent, bool mirrored,
                       int *result)
{
    if (!result || sourceExtent <= 0 || localOutputCoordinate < 0)
        return false;

    const qint64 source = qint64(tileSourceOrigin)
                        + qint64(localOutputCoordinate) * qMax(1, downsample);
    if (source < 0 || source >= sourceExtent)
        return false;

    const qint64 mapped = mirrored ? qint64(sourceExtent) - 1 - source : source;
    if (mapped < 0 || mapped >= sourceExtent)
        return false;

    *result = int(mapped);
    return true;
}

bool sourcePixelOffset(qint64 pixelIndex, int bytesPerPixel,
                       qint64 regionLength, qint64 *relativeOffset)
{
    if (!relativeOffset || pixelIndex < 0 || bytesPerPixel <= 0
            || regionLength < bytesPerPixel)
        return false;

    const qint64 bpp = bytesPerPixel;
    if (pixelIndex > (regionLength - bpp) / bpp)
        return false;

    *relativeOffset = pixelIndex * bpp;
    return true;
}

// Карта «позиция в тайле -> координата исходного пикселя» (-1: пикселя нет).
// Строится один раз на тайл по каждой оси, поэтому внутренние циклы больше
// не пересчитывают downsample/зеркало для каждого пикселя.
QVector<int> buildAxisMap(int sourceOrigin, int count, int downsample,
                          int sourceExtent, bool mirrored)
{
    QVector<int> map(count, -1);
    for (int i = 0; i < count; ++i) {
        int mapped = 0;
        if (sampledCoordinate(sourceOrigin, i, downsample, sourceExtent,
                              mirrored, &mapped))
            map[i] = mapped;
    }
    return map;
}

void fillBitpacked(QImage &img, const RenderContext &ctx,
                   const QVector<int> &cols, const QVector<int> &rows,
                   const uchar *raw, qint64 regionLength)
{
    const int bpp = ctx.bppMode;
    if (bpp < 1 || bpp > 8)
        return;

    // Use 64-bit row arithmetic. Padding belongs to every source row and must
    // not be lost when a tile begins at a later row.
    const qint64 rowBytes = PixelMath::bitpackedRowBytes(
        ctx.imageWidth, bpp, ctx.rowPaddingBits);
    if (rowBytes <= 0)
        return;

    const BitPackedReader reader(bpp, ctx.bitOrder,
                                 ctx.rowPaddingBits, ctx.pixelOrder);
    const QVector<QRgb> lut = reader.buildLut(ctx.palette);
    const QRgb black = qRgb(0, 0, 0);

    for (int y = 0; y < rows.size(); ++y) {
        if (shouldAbort(ctx, y))
            return;

        const int gy = rows[y];
        if (gy < 0)
            continue;

        const qint64 rowRelative = qint64(gy) * rowBytes;
        if (rowRelative >= regionLength)
            continue;

        const qint64 available = qMin(rowBytes, regionLength - rowRelative);
        const uchar *row = (raw && available > 0)
            ? raw + ctx.regionStart + rowRelative : nullptr;

        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < cols.size(); ++x) {
            const int gx = cols[x];
            if (gx < 0)
                continue;
            line[x] = lut.value(int(reader.valueAt(row, available, gx)), black);
        }
    }
}

// 8 bpp. Палитра «gray» — частный случай: это тождественная таблица, поэтому
// отдельный быстрый путь для неё не нужен (раньше был fillGrayscale).
void fillSingleByte(QImage &img, const RenderContext &ctx,
                    const QVector<int> &cols, const QVector<int> &rows,
                    const uchar *raw, qint64 regionLength)
{
    QRgb palette[256];
    for (int i = 0; i < 256; ++i) {
        const QColor c = (i < ctx.palette.size()) ? ctx.palette[i]
                                                  : QColor(i, i, i);
        palette[i] = qRgba(c.red(), c.green(), c.blue(), 255);
    }

    const bool reverse = (ctx.bitOrder == QLatin1String("lsb"));
    QRgb reversePalette[256];
    if (reverse) {
        for (int i = 0; i < 256; ++i)
            reversePalette[i] = palette[PixelDecoder::reverseBits8(quint8(i))];
    }
    const QRgb *lut = reverse ? reversePalette : palette;

    for (int y = 0; y < rows.size(); ++y) {
        if (shouldAbort(ctx, y))
            return;

        const int gy = rows[y];
        if (gy < 0)
            continue;

        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < cols.size(); ++x) {
            const int gx = cols[x];
            if (gx < 0)
                continue;

            const qint64 pixelIndex = qint64(gy) * ctx.imageWidth + gx;
            qint64 relative = 0;
            if (!sourcePixelOffset(pixelIndex, 1, regionLength, &relative))
                continue;

            line[x] = lut[raw[ctx.regionStart + relative]];
        }
    }
}

void fillMultibyte(QImage &img, const RenderContext &ctx,
                   const QVector<int> &cols, const QVector<int> &rows,
                   const uchar *raw, qint64 regionLength)
{
    const int bpp = qBound(1, ctx.bytesPerPixel, 8);

    // Формат, порядок байтов/битов и параметры палитры разбираются один раз
    // на тайл, а не для каждого пикселя.
    const PixelDecoder::Decoder decoder(ctx.paletteId, ctx.bitOrder,
                                        ctx.byteOrder, ctx.paletteParams);

    for (int y = 0; y < rows.size(); ++y) {
        if (shouldAbort(ctx, y))
            return;

        const int gy = rows[y];
        if (gy < 0)
            continue;

        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < cols.size(); ++x) {
            const int gx = cols[x];
            if (gx < 0)
                continue;

            const qint64 pixelIndex = qint64(gy) * ctx.imageWidth + gx;
            qint64 relative = 0;
            if (!sourcePixelOffset(pixelIndex, bpp, regionLength, &relative))
                continue;

            line[x] = decoder.decode(raw + ctx.regionStart + relative, bpp);
        }
    }
}

} // namespace

// =========================================================================
// Worker task for QThreadPool
// =========================================================================
class TileRenderTask : public QRunnable {
public:
    TileRenderTask(TileKey key, RenderContextPtr ctx, quint64 generation,
                   QPointer<TileManager> manager)
        : m_key(key), m_context(std::move(ctx)),
          m_generation(generation), m_manager(manager)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        QImage image;
        if (m_context && !cancelled(*m_context))
            image = TileManager::renderTile(*m_context, m_key);

        // The manager's destructor waits for the pool, so the guarded QObject
        // remains alive while this completion is being posted.
        if (m_manager) {
            QMetaObject::invokeMethod(m_manager.data(), "onTileRendered",
                                      Qt::QueuedConnection,
                                      Q_ARG(TileKey, m_key),
                                      Q_ARG(QImage, image),
                                      Q_ARG(quint64, m_generation));
        }
    }

private:
    TileKey m_key;
    RenderContextPtr m_context;
    quint64 m_generation;
    QPointer<TileManager> m_manager;
};

// =========================================================================
// TileManager
// =========================================================================
TileManager::TileManager(CanvasWidget *canvas)
    : QObject(canvas), m_canvas(canvas)
{
    qRegisterMetaType<TileKey>("TileKey");
    m_pool = new QThreadPool(this);

    int idealThreads = QThread::idealThreadCount();
    if (idealThreads <= 0)
        idealThreads = 2;
    // A small cap prevents a high-performance setting plus 2048px tiles from
    // allocating many large temporary QImages at once.
    m_pool->setMaxThreadCount(qBound(1, idealThreads - 1, 4));
}

TileManager::~TileManager()
{
    if (m_context && m_context->cancel)
        m_context->cancel->cancel();
    if (m_exportContext && m_exportContext->cancel)
        m_exportContext->cancel->cancel();
    if (m_pool) {
        m_pool->clear();
        m_pool->waitForDone();
    }
}

void TileManager::setCacheLimitMb(int mb)
{
    const int safeMb = qBound(16, mb, 2048);
    m_cacheLimitBytes = qint64(safeMb) * 1024LL * 1024LL;
    cleanupByMemory();
}

int TileManager::cacheLimitMb() const
{
    return int(m_cacheLimitBytes / (1024LL * 1024LL));
}

void TileManager::clearCache()
{
    if (m_context && m_context->cancel)
        m_context->cancel->cancel();

    m_tiles.clear();
    m_cachedBytes = 0;
    m_pending.clear();
    m_visibleTiles.clear();
    m_context.reset();
    ++m_generation;

    // Do not reset m_activeTasks here. Old-generation jobs are still submitted
    // to QThreadPool and must decrement the real in-flight counter on completion.
}

int TileManager::currentTileSize() const
{
    return m_context ? m_context->tileSize : 0;
}

qint64 TileManager::cacheBytes() const
{
    return m_cachedBytes;
}

RenderContextPtr TileManager::buildContext() const
{
    if (!m_canvas || !m_canvas->mainWindow())
        return RenderContextPtr();

    MainWindow *mw = m_canvas->mainWindow();
    std::shared_ptr<RenderContext> context = std::make_shared<RenderContext>();

    const MappedFilePtr mapped = mw->sourceMapped();
    if (mapped)
        context->mapped = mapped;
    else
        context->data = mw->sourceBytes();

    context->imageWidth = m_canvas->imageWidth();
    context->imageHeight = m_canvas->imageHeight();
    context->tileSize = normalizedTileSourceSize(mw->tileSize());

    context->bitpacked = mw->isBitpacked();
    context->bppMode = qBound(1, mw->bppMode(), 8);
    context->bytesPerPixel = qBound(1, mw->bytesPerPixel(), 8);
    context->rowPaddingBits = qMax(0, mw->rowPaddingBits());
    context->bitOrder = mw->bitOrder();
    context->pixelOrder = mw->pixelOrder();
    context->byteOrder = mw->byteOrder();

    context->mirrorH = mw->mirrorHorizontal();
    context->mirrorV = mw->mirrorVertical();

    context->paletteId = mw->currentPaletteId();
    context->palette = mw->currentPalette();
    context->paletteParams = mw->paletteParams()
                             .value(context->paletteId).toMap();

    // Derive the region against the actual retained byte source, not a stale
    // UI file-size value. This prevents negative/out-of-range memory offsets.
    const qint64 sourceSize = context->byteSize();
    const quint64 requestedOffset = mw->offset();
    if (sourceSize >= 0 && requestedOffset <= quint64(sourceSize)) {
        const qint64 start = qint64(requestedOffset);
        const quint64 actualLength = PixelMath::effectiveRegionLength(
            quint64(sourceSize), requestedOffset, mw->length());
        context->regionStart = start;
        context->regionEnd = start + qint64(actualLength);
    } else {
        context->regionStart = 0;
        context->regionEnd = 0;
    }

    context->cancel = std::make_shared<CancelFlag>();
    return context;
}

QImage TileManager::renderTile(const RenderContext &ctx, const TileKey &key)
{
    if (ctx.imageWidth <= 0 || ctx.imageHeight <= 0 || ctx.tileSize <= 0
            || key.tx < 0 || key.ty < 0)
        return QImage();

    const int ds = qMax(1, key.ds);
    const int outputTileSize = tileOutputSize(ctx.tileSize, ds);
    const int outputWidth = tileOutputExtent(ctx.imageWidth, ds);
    const int outputHeight = tileOutputExtent(ctx.imageHeight, ds);
    if (outputTileSize <= 0 || outputWidth <= 0 || outputHeight <= 0)
        return QImage();

    const qint64 outputX64 = qint64(key.tx) * outputTileSize;
    const qint64 outputY64 = qint64(key.ty) * outputTileSize;
    if (outputX64 >= outputWidth || outputY64 >= outputHeight)
        return QImage();

    const int outputX = int(outputX64);
    const int outputY = int(outputY64);
    const int tileWidth = qMin(outputTileSize, outputWidth - outputX);
    const int tileHeight = qMin(outputTileSize, outputHeight - outputY);
    if (tileWidth <= 0 || tileHeight <= 0)
        return QImage();

    const qint64 sourceX64 = qint64(outputX) * ds;
    const qint64 sourceY64 = qint64(outputY) * ds;
    if (sourceX64 >= ctx.imageWidth || sourceY64 >= ctx.imageHeight)
        return QImage();

    if (cancelled(ctx))
        return QImage();

    QImage image(tileWidth, tileHeight, QImage::Format_ARGB32);
    if (image.isNull())
        return QImage();
    image.fill(Qt::black);

    const uchar *raw = nullptr;
    qint64 regionLength = 0;
    if (validSourceRegion(ctx, &raw, &regionLength) && regionLength > 0 && raw) {
        const QVector<int> cols = buildAxisMap(
            int(sourceX64), tileWidth, ds, ctx.imageWidth, ctx.mirrorH);
        const QVector<int> rows = buildAxisMap(
            int(sourceY64), tileHeight, ds, ctx.imageHeight, ctx.mirrorV);

        if (ctx.bitpacked)
            fillBitpacked(image, ctx, cols, rows, raw, regionLength);
        else if (ctx.bytesPerPixel == 1)
            fillSingleByte(image, ctx, cols, rows, raw, regionLength);
        else
            fillMultibyte(image, ctx, cols, rows, raw, regionLength);
    }

    if (cancelled(ctx))
        return QImage();

    return image;
}

void TileManager::scheduleRender(const TileKey &key, bool highPriority)
{
    if (!m_pool || key.tx < 0 || key.ty < 0 || key.ds < 1
            || m_pending.contains(key) || m_tiles.contains(key))
        return;

    if (!m_context)
        m_context = buildContext();
    if (!m_context)
        return;

    m_pending.insert(key);
    TileRenderTask *task = new TileRenderTask(key, m_context,
                                               m_generation, this);
    ++m_activeTasks;
    if (m_activeTasks == 1)
        emit activeTasksChanged(m_activeTasks);
    m_pool->start(task, highPriority ? 10 : 0);
}

QImage TileManager::getTile(int tileX, int tileY)
{
    if (!m_canvas || !m_canvas->mainWindow() || tileX < 0 || tileY < 0)
        return QImage();

    const int ds = qMax(1, m_canvas->downsample());
    const int outputWidth = tileOutputExtent(m_canvas->imageWidth(), ds);
    const int outputHeight = tileOutputExtent(m_canvas->imageHeight(), ds);
    const int outputTileSize = tileOutputSize(
        normalizedTileSourceSize(m_canvas->mainWindow()->tileSize()), ds);
    if (outputTileSize <= 0
            || qint64(tileX) * outputTileSize >= outputWidth
            || qint64(tileY) * outputTileSize >= outputHeight)
        return QImage();

    const TileKey key(tileX, tileY, ds);
    const auto it = m_tiles.find(key);
    if (it != m_tiles.end()) {
        it->lastUsed = ++m_accessCounter;
        return it->image;
    }

    scheduleRender(key, true);
    return QImage();
}

QImage TileManager::renderImage(const QSize &targetSize, int scale,
                                int downsample) const
{
    if (!m_canvas || !m_canvas->mainWindow()
            || targetSize.width() <= 0 || targetSize.height() <= 0)
        return QImage();

    const RenderContextPtr context = buildContext();
    if (!context || context->imageWidth <= 0 || context->imageHeight <= 0)
        return QImage();

    const int s = qMax(1, scale);
    const int ds = qMax(1, downsample);
    const int outputWidth = tileOutputExtent(context->imageWidth, ds);
    const int outputHeight = tileOutputExtent(context->imageHeight, ds);
    const qint64 expectedWidth = qint64(outputWidth) * s;
    const qint64 expectedHeight = qint64(outputHeight) * s;
    if (expectedWidth <= 0 || expectedHeight <= 0
            || expectedWidth > std::numeric_limits<int>::max()
            || expectedHeight > std::numeric_limits<int>::max())
        return QImage();

    const QSize expectedSize = QSize(int(expectedWidth), int(expectedHeight));
    if (targetSize != expectedSize)
        return QImage();

    // Qt 5.6's QImage byteCount/scanline sizes are int-based. Refuse an
    // export that cannot be represented safely instead of risking overflow
    // or a multi-gigabyte allocation attempt.
    const qint64 rowBytes = qint64(targetSize.width()) * 4;
    if (rowBytes <= 0 || rowBytes > INT_MAX
            || qint64(targetSize.height()) > INT_MAX / rowBytes)
        return QImage();

    QImage output(targetSize, QImage::Format_ARGB32);
    if (output.isNull())
        return QImage();
    const int outputTileSize = tileOutputSize(context->tileSize, ds);
    if (outputTileSize <= 0)
        return QImage();
    const int tilesX = int((qint64(outputWidth) + outputTileSize - 1)
                           / outputTileSize);
    const int tilesY = int((qint64(outputHeight) + outputTileSize - 1)
                           / outputTileSize);

    QPainter painter(&output);
    if (!painter.isActive())
        return QImage();
    // Match the widget's actual background brush so transparent source pixels
    // are composited identically on screen and in the exported PNG.
    painter.fillRect(output.rect(), m_canvas->palette().window());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (int ty = 0; ty < tilesY; ++ty) {
        for (int tx = 0; tx < tilesX; ++tx) {
            if (cancelled(*context))
                return QImage();

            const QImage tile = renderTile(*context, TileKey(tx, ty, ds));
            if (tile.isNull())
                return QImage();

            const qint64 x = qint64(tx) * outputTileSize * s;
            const qint64 y = qint64(ty) * outputTileSize * s;
            const qint64 w = qint64(tile.width()) * s;
            const qint64 h = qint64(tile.height()) * s;
            if (x < 0 || y < 0 || w <= 0 || h <= 0
                    || x + w > targetSize.width()
                    || y + h > targetSize.height())
                continue;

            painter.drawImage(QRect(int(x), int(y), int(w), int(h)), tile);
        }
    }
    painter.end();
    return output;
}


namespace {

bool validPngExportRegion(const RenderContext &context, const QRect &canvasRect,
                          int scale, int downsample, QString *errorMessage)
{
    if (context.imageWidth <= 0 || context.imageHeight <= 0
            || canvasRect.isEmpty() || scale <= 0 || downsample <= 0) {
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("TileManager", "Invalid export dimensions.");
        return false;
    }

    const int outputWidth = tileOutputExtent(context.imageWidth, downsample);
    const int outputHeight = tileOutputExtent(context.imageHeight, downsample);
    const qint64 fullWidth = qint64(outputWidth) * scale;
    const qint64 fullHeight = qint64(outputHeight) * scale;
    const qint64 right = qint64(canvasRect.x()) + canvasRect.width();
    const qint64 bottom = qint64(canvasRect.y()) + canvasRect.height();
    if (outputWidth <= 0 || outputHeight <= 0
            || fullWidth <= 0 || fullHeight <= 0
            || fullWidth > INT_MAX || fullHeight > INT_MAX
            || canvasRect.x() < 0 || canvasRect.y() < 0
            || right > fullWidth || bottom > fullHeight) {
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("TileManager", "The requested export area is outside the image.");
        return false;
    }

    // Qt 5.6 stores QImage scanline and byte-count values in int-sized fields.
    // Reject dimensions that cannot be represented before allocating anything.
    const qint64 rowBytes = qint64(canvasRect.width()) * 4;
    if (rowBytes <= 0 || rowBytes > INT_MAX
            || qint64(canvasRect.height()) > INT_MAX / rowBytes) {
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("TileManager", "The PNG is too large for this Qt build.");
        return false;
    }

    const qint64 imageBytes = rowBytes * qint64(canvasRect.height());
    const qint64 availableMemory = BytesSource::availableMemory();
    if (availableMemory > 0) {
        // Reserve up to 512 MiB of currently available physical RAM for the
        // application, source mapping, OS and the PNG encoder.
        const qint64 reserve = qMin<qint64>(512LL * 1024 * 1024,
                                            availableMemory / 4);
        if (imageBytes > availableMemory - reserve) {
            if (errorMessage)
                *errorMessage = QCoreApplication::translate(
                    "TileManager",
                    "The PNG would require more memory than is safely available.");
            return false;
        }
    }
    return true;
}

QImage renderPngExportRegion(const RenderContext &context,
                             const QRect &canvasRect,
                             int scale, int downsample,
                             const QColor &background)
{
    QString validationError;
    if (!validPngExportRegion(context, canvasRect, scale, downsample,
                              &validationError))
        return QImage();

    QImage output(canvasRect.size(), QImage::Format_ARGB32);
    if (output.isNull())
        return QImage();
    output.fill(background.rgba());

    const int outputWidth = tileOutputExtent(context.imageWidth, downsample);
    const int outputHeight = tileOutputExtent(context.imageHeight, downsample);
    const int outputTileSize = tileOutputSize(context.tileSize, downsample);
    const qint64 tileSpan = qint64(outputTileSize) * scale;
    if (tileSpan <= 0)
        return QImage();

    const qint64 regionRight = qint64(canvasRect.x()) + canvasRect.width() - 1;
    const qint64 regionBottom = qint64(canvasRect.y()) + canvasRect.height() - 1;
    const int firstTileX = int(qint64(canvasRect.x()) / tileSpan);
    const int firstTileY = int(qint64(canvasRect.y()) / tileSpan);
    const int lastTileX = int(regionRight / tileSpan);
    const int lastTileY = int(regionBottom / tileSpan);

    const int tilesX = int((qint64(outputWidth) + outputTileSize - 1)
                           / outputTileSize);
    const int tilesY = int((qint64(outputHeight) + outputTileSize - 1)
                           / outputTileSize);
    if (firstTileX < 0 || firstTileY < 0
            || lastTileX >= tilesX || lastTileY >= tilesY)
        return QImage();

    QPainter painter(&output);
    if (!painter.isActive())
        return QImage();
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    for (int ty = firstTileY; ty <= lastTileY; ++ty) {
        for (int tx = firstTileX; tx <= lastTileX; ++tx) {
            if (cancelled(context))
                return QImage();

            const QImage tile = TileManager::renderTile(
                context, TileKey(tx, ty, downsample));
            if (tile.isNull())
                return QImage();

            const qint64 tileX = qint64(tx) * tileSpan;
            const qint64 tileY = qint64(ty) * tileSpan;
            const qint64 tileWidth = qint64(tile.width()) * scale;
            const qint64 tileHeight = qint64(tile.height()) * scale;
            const qint64 destX = tileX - canvasRect.x();
            const qint64 destY = tileY - canvasRect.y();
            if (tileWidth <= 0 || tileHeight <= 0
                    || destX < INT_MIN || destX > INT_MAX
                    || destY < INT_MIN || destY > INT_MAX
                    || tileWidth > INT_MAX || tileHeight > INT_MAX)
                return QImage();

            const QRect target = QRect(
                int(destX),
                int(destY),
                int(tileWidth),
                int(tileHeight)
            );

            painter.drawImage(target, tile);
        }
    }
    painter.end();
    return output;
}

} // namespace

class PngExportTask : public QRunnable {
public:
    PngExportTask(const QString &path, RenderContextPtr context,
                  const QRect &canvasRect, int scale, int downsample,
                  const QColor &background, QPointer<TileManager> manager)
        : m_path(path), m_context(std::move(context)),
          m_canvasRect(canvasRect), m_scale(scale), m_downsample(downsample),
          m_background(background), m_manager(manager)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        bool success = false;
        QString errorMessage;

        if (m_context && !cancelled(*m_context)) {
            const QImage image = renderPngExportRegion(
                *m_context, m_canvasRect, m_scale, m_downsample, m_background);
            if (image.isNull()) {
                errorMessage = QCoreApplication::translate(
                    "TileManager",
                    "The image could not be rendered. Check its dimensions and available memory.");
            } else {
                // QSaveFile makes export atomic: a failed encoder or disk write
                // won't leave a truncated file at the requested destination.
                QSaveFile file(m_path);
                if (!file.open(QIODevice::WriteOnly)) {
                    errorMessage = file.errorString();
                } else {
                    QImageWriter writer(&file, QByteArray("png"));
                    if (!writer.write(image)) {
                        errorMessage = writer.errorString();
                        file.cancelWriting();
                    } else if (cancelled(*m_context)) {
                        errorMessage = QCoreApplication::translate("TileManager", "PNG export was cancelled.");
                        file.cancelWriting();
                    } else if (!file.commit()) {
                        errorMessage = file.errorString();
                    } else {
                        success = true;
                    }
                }
            }
        } else {
            errorMessage = QCoreApplication::translate("TileManager", "PNG export was cancelled.");
        }

        if (m_manager) {
            QMetaObject::invokeMethod(m_manager.data(),
                                      "onPngExportTaskFinished",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, m_path),
                                      Q_ARG(bool, success),
                                      Q_ARG(QString, errorMessage));
        }
    }

private:
    QString m_path;
    RenderContextPtr m_context;
    QRect m_canvasRect;
    int m_scale;
    int m_downsample;
    QColor m_background;
    QPointer<TileManager> m_manager;
};

bool TileManager::startPngExport(const QString &path, const QRect &canvasRect,
                                 int scale, int downsample,
                                 const QColor &background,
                                 QString *errorMessage)
{
    auto fail = [errorMessage](const QString &text) -> bool {
        if (errorMessage)
            *errorMessage = text;
        return false;
    };

    if (!m_pool || path.isEmpty())
        return fail(tr("The export task could not be started."));
    if (m_exportContext)
        return fail(tr("A PNG export is already running."));

    const RenderContextPtr context = buildContext();
    if (!context)
        return fail(tr("The export task could not be started."));

    QString validationError;
    if (!validPngExportRegion(*context, canvasRect, scale, downsample,
                              &validationError))
        return fail(validationError);

    // buildContext() returns a fresh context with its own cancellation token.
    // It is intentionally independent from the live tile-cache generation:
    // changing zoom, palette, or opening another file cannot invalidate an
    // export that has already started.
    m_exportContext = context;
    PngExportTask *task = new PngExportTask(
        path, context, canvasRect, scale, downsample, background, this);
    m_pool->start(task, -1); // let visible-tile renders take priority
    return true;
}

void TileManager::onPngExportTaskFinished(const QString &path, bool success,
                                          const QString &errorMessage)
{
    m_exportContext.reset();
    emit pngExportFinished(path, success, errorMessage);
}

void TileManager::setVisibleTiles(const QSet<TileKey> &visible)
{
    if (m_visibleTiles == visible)
        return;
    m_visibleTiles = visible;
    cleanupByMemory();
}

void TileManager::onTileRendered(const TileKey &key, const QImage &image,
                                 quint64 generation)
{
    if (m_activeTasks > 0) {
        --m_activeTasks;
        if (m_activeTasks == 0)
            emit activeTasksChanged(0);
    }

    if (generation != m_generation)
        return;

    m_pending.remove(key);
    if (image.isNull())
        return;

    storeTile(key, image);
    cleanupByMemory();
    emit tileReady(key);
}

void TileManager::prefetchAround(const QSet<TileKey> &visible)
{
    if (visible.isEmpty() || !m_canvas || !m_canvas->mainWindow())
        return;

    MainWindow *mw = m_canvas->mainWindow();
    const int ds = qMax(1, m_canvas->downsample());
    const int tileSourceSize = normalizedTileSourceSize(mw->tileSize());
    const int outputTileSize = tileOutputSize(tileSourceSize, ds);
    const int outputWidth = tileOutputExtent(m_canvas->imageWidth(), ds);
    const int outputHeight = tileOutputExtent(m_canvas->imageHeight(), ds);
    if (outputTileSize <= 0 || outputWidth <= 0 || outputHeight <= 0)
        return;

    const qint64 tileBytes = qint64(outputTileSize)
                           * outputTileSize * 4;
    const qint64 maxTiles = m_cacheLimitBytes / qMax<qint64>(1, tileBytes);
    if (qint64(visible.size()) >= maxTiles)
        return;

    int minX = INT_MAX, maxX = -1, minY = INT_MAX, maxY = -1;
    for (const TileKey &key : visible) {
        if (key.ds != ds || key.tx < 0 || key.ty < 0)
            continue;
        minX = qMin(minX, key.tx);
        maxX = qMax(maxX, key.tx);
        minY = qMin(minY, key.ty);
        maxY = qMax(maxY, key.ty);
    }
    if (maxX < 0 || maxY < 0)
        return;

    const qint64 tilesX64 = (qint64(outputWidth) + outputTileSize - 1)
                          / outputTileSize;
    const qint64 tilesY64 = (qint64(outputHeight) + outputTileSize - 1)
                          / outputTileSize;
    const int maxTx = int(qMin<qint64>(tilesX64, INT_MAX));
    const int maxTy = int(qMin<qint64>(tilesY64, INT_MAX));
    const int margin = qBound(0, mw->tilePrefetchMargin(), 32);

    const int beginX = int(qMax<qint64>(0, qint64(minX) - margin));
    const int beginY = int(qMax<qint64>(0, qint64(minY) - margin));
    const int endX = int(qMin<qint64>(maxTx - 1, qint64(maxX) + margin));
    const int endY = int(qMin<qint64>(maxTy - 1, qint64(maxY) + margin));

    int budget = 64;
    for (int ty = beginY; ty <= endY; ++ty) {
        for (int tx = beginX; tx <= endX; ++tx) {
            const TileKey key(tx, ty, ds);
            if (m_tiles.contains(key) || m_pending.contains(key))
                continue;
            if (budget <= 0)
                return;
            --budget;
            scheduleRender(key, false);
        }
    }
}

void TileManager::storeTile(const TileKey &key, const QImage &image)
{
    const auto old = m_tiles.constFind(key);
    if (old != m_tiles.constEnd())
        m_cachedBytes -= qMax<qint64>(0, old->image.byteCount());

    CacheEntry entry;
    entry.image = image;
    entry.lastUsed = ++m_accessCounter;
    m_tiles.insert(key, entry);
    m_cachedBytes += qMax<qint64>(0, image.byteCount());
}

void TileManager::cleanupByMemory()
{
    // Если весь кэш (включая видимые тайлы) укладывается в лимит, то и
    // невидимая его часть тоже — обход всех тайлов не нужен.
    if (m_tiles.isEmpty() || m_cachedBytes <= m_cacheLimitBytes)
        return;

    qint64 nonVisibleBytes = 0;
    for (auto it = m_tiles.constBegin(); it != m_tiles.constEnd(); ++it) {
        if (m_visibleTiles.contains(it.key()))
            continue;
        nonVisibleBytes += qMax<qint64>(0, it->image.byteCount());
    }

    if (nonVisibleBytes <= m_cacheLimitBytes)
        return;

    const qint64 target = (m_cacheLimitBytes * 3) / 4;
    QVector<QPair<TileKey, quint64> > order;
    order.reserve(m_tiles.size());
    for (auto it = m_tiles.constBegin(); it != m_tiles.constEnd(); ++it) {
        if (!m_visibleTiles.contains(it.key()))
            order.append(qMakePair(it.key(), it->lastUsed));
    }

    std::sort(order.begin(), order.end(),
              [](const QPair<TileKey, quint64> &a,
                 const QPair<TileKey, quint64> &b) {
                  return a.second < b.second;
              });

    for (const QPair<TileKey, quint64> &entry : order) {
        if (nonVisibleBytes <= target)
            break;

        const auto tileIt = m_tiles.find(entry.first);
        if (tileIt == m_tiles.end())
            continue;

        const qint64 bytes = qMax<qint64>(0, tileIt->image.byteCount());
        nonVisibleBytes = qMax<qint64>(0, nonVisibleBytes - bytes);
        m_cachedBytes   = qMax<qint64>(0, m_cachedBytes - bytes);
        m_tiles.erase(tileIt);
    }
}
