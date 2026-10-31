// Byte-0-scope - binary data visualizer
// File: tilemanager.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tilemanager.h"
#include "canvaswidget.h"
#include "mainwindow.h"
#include "bitpackedreader.h"
#include "pixeldecoder.h"

#include <QDateTime>
#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>
#include <QtMath>
#include <algorithm>
#include <climits>

namespace {

bool cancelled(const RenderContext &ctx) noexcept
{
    return ctx.cancel && ctx.cancel->isCancelled();
}

bool shouldAbort(const RenderContext &ctx, int row) noexcept
{
    return (row & 7) == 0 && cancelled(ctx);
}

void fillBitpacked(QImage &img, const RenderContext &ctx,
                   int tilePx, int tilePy, int tw, int th)
{
    BitPackedReader reader(ctx.bppMode, ctx.bitOrder,
                           ctx.rowPaddingBits, ctx.pixelOrder);
    const QVector<QRgb> lut = reader.buildLut(ctx.palette);
    const int rowBytes = reader.rowBytes(ctx.imageWidth);
    const qint64 dataEnd = ctx.byteSize();

    for (int y = 0; y < th; ++y) {
        if (shouldAbort(ctx, y))
            return;
        int gy = tilePy + y;
        if (ctx.mirrorV)
            gy = ctx.imageHeight - 1 - gy;
        if (gy < 0 || gy >= ctx.imageHeight)
            continue;

        const qint64 start = ctx.regionStart + qint64(gy) * rowBytes;
        if (start >= ctx.regionEnd || start >= dataEnd)
            continue;
        const qint64 end = qMin<qint64>(qMin(start + rowBytes, ctx.regionEnd), dataEnd);

        QByteArray raw(reinterpret_cast<const char *>(ctx.bytes() + start),
                       int(end - start));
        if (raw.size() < rowBytes)
            raw.append(QByteArray(rowBytes - raw.size(), '\0'));

        const QVector<quint8> idx = reader.extractRow(raw, ctx.imageWidth);

        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < tw; ++x) {
            int gx = tilePx + x;
            if (ctx.mirrorH)
                gx = ctx.imageWidth - 1 - gx;
            if (gx < 0 || gx >= ctx.imageWidth || gx >= idx.size())
                continue;
            line[x] = lut.value(idx[gx], qRgb(0, 0, 0));
        }
    }
}

void fillSingleByte(QImage &img, const RenderContext &ctx,
                    int tilePx, int tilePy, int tw, int th)
{
    QRgb paletteArg[256];
    for (int i = 0; i < 256; ++i) {
        const QColor c = (i < ctx.palette.size()) ? ctx.palette[i]
                                                  : QColor(i, i, i);
        paletteArg[i] = qRgba(c.red(), c.green(), c.blue(), 255);
    }
    const uchar *raw = ctx.bytes();
    const qint64 dataSize = ctx.byteSize();

    for (int y = 0; y < th; ++y) {
        if (shouldAbort(ctx, y))
            return;
        int gy = tilePy + y;
        if (ctx.mirrorV)
            gy = ctx.imageHeight - 1 - gy;
        if (gy < 0 || gy >= ctx.imageHeight)
            continue;

        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < tw; ++x) {
            int gx = tilePx + x;
            if (ctx.mirrorH)
                gx = ctx.imageWidth - 1 - gx;
            if (gx < 0 || gx >= ctx.imageWidth)
                continue;

            const qint64 byteOff = ctx.regionStart
                                 + qint64(gy) * ctx.imageWidth + gx;
            if (byteOff >= ctx.regionEnd)
                continue;
            if (byteOff < 0 || byteOff >= dataSize)
                continue;
            line[x] = paletteArg[raw[byteOff]];
        }
    }
}

void fillGrayscale(QImage &img, const RenderContext &ctx,
                   int tilePx, int tilePy, int tw, int th)
{
    const uchar *raw = ctx.bytes();
    const qint64 dataSize = ctx.byteSize();

    for (int y = 0; y < th; ++y) {
        if (shouldAbort(ctx, y))
            return;
        int gy = tilePy + y;
        if (ctx.mirrorV)
            gy = ctx.imageHeight - 1 - gy;
        if (gy < 0 || gy >= ctx.imageHeight)
            continue;

        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < tw; ++x) {
            int gx = tilePx + x;
            if (ctx.mirrorH)
                gx = ctx.imageWidth - 1 - gx;
            if (gx < 0 || gx >= ctx.imageWidth)
                continue;

            const qint64 byteOff = ctx.regionStart
                                 + qint64(gy) * ctx.imageWidth + gx;
            if (byteOff >= ctx.regionEnd)
                continue;
            if (byteOff < 0 || byteOff >= dataSize)
                continue;
            const int v = raw[byteOff];
            line[x] = qRgba(v, v, v, 255);
        }
    }
}

void fillMultibyte(QImage &img, const RenderContext &ctx,
                   int tilePx, int tilePy, int tw, int th)
{
    const int bpp = qMax(1, ctx.bytesPerPixel);
    const QString &pal = ctx.paletteId;
    const QVariantMap &palParams = ctx.paletteParams;
    const uchar *raw = ctx.bytes();
    const qint64 dataSize = ctx.byteSize();

    for (int y = 0; y < th; ++y) {
        if (shouldAbort(ctx, y))
            return;
        int gy = tilePy + y;
        if (ctx.mirrorV)
            gy = ctx.imageHeight - 1 - gy;
        if (gy < 0 || gy >= ctx.imageHeight)
            continue;

        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < tw; ++x) {
            int gx = tilePx + x;
            if (ctx.mirrorH)
                gx = ctx.imageWidth - 1 - gx;
            if (gx < 0 || gx >= ctx.imageWidth)
                continue;

            const qint64 off = ctx.regionStart
                             + qint64(gy) * ctx.imageWidth * bpp
                             + qint64(gx) * bpp;
            if (off + bpp > ctx.regionEnd)
                continue;
            if (off < 0 || off + bpp > dataSize)
                continue;

            line[x] = PixelDecoder::decodeArgbRaw(raw + off, bpp, pal, palParams);
        }
    }
}

} // namespace

// =========================================================================
//  Задача для QThreadPool
// =========================================================================
class TileRenderTask : public QRunnable {
public:
    TileRenderTask(TileKey key, RenderContextPtr ctx, quint64 gen,
                   QPointer<TileManager> mgr)
        : m_key(key), m_ctx(std::move(ctx)), m_gen(gen), m_mgr(mgr)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        QImage img;
        if (!(m_ctx->cancel && m_ctx->cancel->isCancelled()))
            img = TileManager::renderTile(*m_ctx, m_key);

        if (m_mgr) {
            QMetaObject::invokeMethod(m_mgr, "onTileRendered",
                                      Qt::QueuedConnection,
                                      Q_ARG(TileKey, m_key),
                                      Q_ARG(QImage, img),
                                      Q_ARG(quint64, m_gen));
        }
    }

private:
    TileKey               m_key;
    RenderContextPtr      m_ctx;
    quint64               m_gen;
    QPointer<TileManager> m_mgr;
};

// =========================================================================
//  TileManager
// =========================================================================

TileManager::TileManager(CanvasWidget *canvas)
    : QObject(canvas), m_canvas(canvas)
{
    qRegisterMetaType<TileKey>("TileKey");
    m_pool = new QThreadPool(this);
    m_pool->setMaxThreadCount(qMax(2, QThread::idealThreadCount() - 1));
}

TileManager::~TileManager() = default;

qint64 TileManager::now() const
{
    return QDateTime::currentMSecsSinceEpoch();
}

void TileManager::setCacheLimitMb(int mb)
{
    m_cacheLimitBytes = qint64(qMax(16, mb)) * 1024LL * 1024LL;
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
    m_accessTimes.clear();
    m_pending.clear();
    m_visibleTiles.clear();
    m_context.reset();
    ++m_generation;
}

int TileManager::currentTileSize() const
{
    return m_context ? m_context->tileSize : 0;
}

qint64 TileManager::cacheBytes() const
{
    qint64 total = 0;
    for (auto it = m_tiles.constBegin(); it != m_tiles.constEnd(); ++it)
        total += qint64(it.value().byteCount());
    return total;
}

RenderContextPtr TileManager::buildContext() const
{
    if (!m_canvas || !m_canvas->mainWindow())
        return {};

    MainWindow *mw = m_canvas->mainWindow();

    auto ctx = std::make_shared<RenderContext>();

    if (auto mf = mw->sourceMapped())
        ctx->mapped = mf;
    else
        ctx->data = mw->sourceBytes();

    ctx->imageWidth   = m_canvas->imageWidth();
    ctx->imageHeight  = m_canvas->imageHeight();
    ctx->tileSize     = qMax(16, mw->tileSize());

    ctx->bitpacked      = mw->isBitpacked();
    ctx->bppMode        = mw->bppMode();
    ctx->bytesPerPixel  = qMax(1, mw->bytesPerPixel());
    ctx->rowPaddingBits = mw->rowPaddingBits();
    ctx->bitOrder       = mw->bitOrder();
    ctx->pixelOrder     = mw->pixelOrder();

    ctx->mirrorH = mw->mirrorHorizontal();
    ctx->mirrorV = mw->mirrorVertical();

    ctx->paletteId     = mw->currentPaletteId();
    ctx->palette       = mw->currentPalette();
    ctx->paletteParams = mw->paletteParams().value(ctx->paletteId).toMap();

    const quint64 offset  = mw->offset();
    const quint64 length  = mw->length() > 0 ? mw->length() : mw->fileSize();
    const quint64 fs      = mw->fileSize();
    const quint64 realLen = qMin(length, fs > offset ? (fs - offset) : quint64(0));
    ctx->regionStart = qint64(offset);
    ctx->regionEnd   = qint64(offset + realLen);

    ctx->cancel = std::make_shared<CancelFlag>();
    return ctx;
}

QImage TileManager::renderTile(const RenderContext &ctx, const TileKey &key)
{
    if (ctx.imageWidth <= 0 || ctx.imageHeight <= 0 || ctx.tileSize <= 0)
        return {};

    const int ts = ctx.tileSize;
    const int px = key.tx * ts;
    const int py = key.ty * ts;
    const int tw = qMin(ts, ctx.imageWidth  - px);
    const int th = qMin(ts, ctx.imageHeight - py);
    if (tw <= 0 || th <= 0)
        return {};

    if (cancelled(ctx))
        return {};

    QImage img(tw, th, QImage::Format_ARGB32);
    img.fill(Qt::black);

    if (ctx.bitpacked) {
        fillBitpacked(img, ctx, px, py, tw, th);
    } else if (ctx.bytesPerPixel == 1
               && ctx.paletteId == QLatin1String("gray")) {
        fillGrayscale(img, ctx, px, py, tw, th);
    } else if (ctx.bytesPerPixel == 1) {
        fillSingleByte(img, ctx, px, py, tw, th);
    } else {
        fillMultibyte(img, ctx, px, py, tw, th);
    }

    if (cancelled(ctx))
        return {};

    if (key.ds > 1) {
        const int w = qMax(1, tw / key.ds);
        const int h = qMax(1, th / key.ds);
        if (w != tw || h != th)
            img = img.scaled(w, h, Qt::IgnoreAspectRatio,
                             Qt::FastTransformation);
    }

    return img;
}

void TileManager::scheduleRender(const TileKey &key, bool highPriority)
{
    if (m_pending.contains(key))
        return;
    if (!m_context)
        m_context = buildContext();
    if (!m_context || !m_pool)
        return;

    m_pending.insert(key);

    auto *task = new TileRenderTask(key, m_context, m_generation, this);
    m_pool->start(task, highPriority ? 10 : 0);

    if (m_activeTasks++ == 0)
        emit activeTasksChanged(m_activeTasks);
}

QImage TileManager::getTile(int tileX, int tileY)
{
    if (!m_canvas || !m_canvas->mainWindow())
        return {};

    const int ds = qMax(1, m_canvas->downsample());
    TileKey key{tileX, tileY, ds};

    auto it = m_tiles.constFind(key);
    if (it != m_tiles.constEnd()) {
        m_accessTimes[key] = now();
        return it.value();
    }

    scheduleRender(key, /*highPriority=*/true);
    return QImage();
}

void TileManager::setVisibleTiles(const QSet<TileKey> &visible)
{
    if (m_visibleTiles == visible)
        return;
    m_visibleTiles = visible;
    cleanupByMemory();
}

void TileManager::onTileRendered(const TileKey &key, const QImage &img,
                                 quint64 gen)
{
    if (m_activeTasks > 0 && --m_activeTasks == 0)
        emit activeTasksChanged(0);

    if (gen != m_generation)
        return;

    m_pending.remove(key);
    if (img.isNull())
        return;

    m_tiles.insert(key, img);
    m_accessTimes.insert(key, now());
    cleanupByMemory();

    emit tileReady(key);
}

void TileManager::prefetchAround(const QSet<TileKey> &visible)
{
    if (visible.isEmpty() || !m_canvas)
        return;
    MainWindow *mw = m_canvas->mainWindow();
    if (!mw)
        return;

    const int ds = qMax(1, m_canvas->downsample());
    const int ts = qMax(16, mw->tileSize());

    const int renderedTileW = qMax(1, ts / ds);
    const qint64 tileBytes  = qint64(renderedTileW) * renderedTileW * 4;
    const qint64 maxTiles   = m_cacheLimitBytes / qMax<qint64>(1, tileBytes);
    if (qint64(visible.size()) >= maxTiles)
        return;

    const int margin = qMax(0, mw->tilePrefetchMargin());
    const int iw = m_canvas->imageWidth();
    const int ih = m_canvas->imageHeight();

    const int maxTx = (iw + ts - 1) / ts;
    const int maxTy = (ih + ts - 1) / ts;

    int minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
    for (const TileKey &k : visible) {
        minX = qMin(minX, k.tx);
        maxX = qMax(maxX, k.tx);
        minY = qMin(minY, k.ty);
        maxY = qMax(maxY, k.ty);
    }

    int budget = 64;
    for (int ty = qMax(0, minY - margin);
         ty <= qMin(maxTy - 1, maxY + margin); ++ty) {
        for (int tx = qMax(0, minX - margin);
             tx <= qMin(maxTx - 1, maxX + margin); ++tx) {
            TileKey k{tx, ty, ds};
            if (m_tiles.contains(k) || m_pending.contains(k))
                continue;
            if (budget-- <= 0)
                return;
            scheduleRender(k, /*highPriority=*/false);
        }
    }
}

void TileManager::cleanupByMemory()
{
    if (m_tiles.isEmpty())
        return;

    qint64 nonVisibleBytes = 0;
    for (auto it = m_tiles.constBegin(); it != m_tiles.constEnd(); ++it) {
        if (!m_visibleTiles.contains(it.key()))
            nonVisibleBytes += qint64(it.value().byteCount());
    }

    if (nonVisibleBytes <= m_cacheLimitBytes)
        return;

    const qint64 target = qint64(m_cacheLimitBytes * 0.75);

    QVector<QPair<TileKey, qint64>> order;
    order.reserve(m_accessTimes.size());
    for (auto it = m_accessTimes.constBegin(); it != m_accessTimes.constEnd(); ++it) {
        if (m_visibleTiles.contains(it.key()))
            continue;
        order.append({it.key(), it.value()});
    }

    std::sort(order.begin(), order.end(),
              [](const QPair<TileKey, qint64> &a,
                 const QPair<TileKey, qint64> &b) {
                  return a.second < b.second;
              });

    for (const auto &pair : order) {
        if (nonVisibleBytes <= target)
            break;
        auto it = m_tiles.find(pair.first);
        if (it == m_tiles.end()) {
            m_accessTimes.remove(pair.first);
            continue;
        }
        nonVisibleBytes -= qint64(it.value().byteCount());
        m_tiles.erase(it);
        m_accessTimes.remove(pair.first);
    }
}