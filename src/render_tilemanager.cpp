// Byte-0-scope - binary data visualizer
// File: render_tilemanager.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "render_tilemanager.h"
#include "core_bitpackedreader.h"
#include "core_pixeldecoder.h"
#include "core_pixelmath.h"
#include "qt_compat.h"
#include "render_canvaswidget.h"
#include "render_pngstream.h"
#include "ui_mainwindow.h"

#include <QCoreApplication>
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>
#include <algorithm>
#include <climits>
#include <cstring>
#include <list>
#include <map>
#include <utility>

namespace {

bool cancelled(const RenderContext &ctx) noexcept
{
    return ctx.cancel && ctx.cancel->isCancelled();
}

void cancelContext(const RenderContextPtr &ctx)
{
    if (ctx && ctx->cancel)
        ctx->cancel->cancel();
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

    const qint64 rowBytes = PixelMath::bitpackedRowBytes(
        ctx.imageWidth, bpp, ctx.rowPaddingBits);
    if (rowBytes <= 0)
        return;

    const BitPackedReader reader(bpp, ctx.bitOrder, ctx.pixelOrder);
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

// 8 bpp
void fillSingleByte(QImage &img, const RenderContext &ctx,
                    const QVector<int> &cols, const QVector<int> &rows,
                    const uchar *raw, qint64 regionLength)
{
    const QRgb *lut = ctx.singleByteReverse ? ctx.singleByteLutReverse
                                            : ctx.singleByteLut;

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
    m_pool->setMaxThreadCount(qBound(1, idealThreads - 1, 4));
}

TileManager::~TileManager()
{
    cancelContext(m_context);
    cancelContext(m_exportContext);
    m_pool->clear();
    m_pool->waitForDone();
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
    cancelContext(m_context);

    m_tiles.clear();
    m_cachedBytes = 0;
    m_pending.clear();
    m_visibleTiles.clear();
    m_context.reset();
    ++m_generation;
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

    for (int i = 0; i < 256; ++i) {
        const QColor c = (i < context->palette.size())
                             ? context->palette[i]
                             : QColor(i, i, i);
        context->singleByteLut[i] =
            qRgba(c.red(), c.green(), c.blue(), 255);
    }
    context->singleByteReverse =
        (context->bitOrder == QLatin1String("lsb"));
    if (context->singleByteReverse) {
        for (int i = 0; i < 256; ++i) {
            context->singleByteLutReverse[i] =
                context->singleByteLut[PixelMath::reverseBits8(quint8(i))];
        }
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
    if (key.tx < 0 || key.ty < 0 || key.ds < 1
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

    const qint64 rowBytes = qint64(canvasRect.width()) * 4;
    if (rowBytes <= 0 || rowBytes > INT_MAX) {
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("TileManager",
                "The PNG row is too wide for this build.");
        return false;
    }

    // Оценка по худшему формату (RGBA): строка + буфер IDAT.
    const qint64 streamOverhead = rowBytes + 1024 * 1024;
    const qint64 availableMemory = BytesSource::availableMemory();
    if (availableMemory > 0) {
        const qint64 reserve = qMin<qint64>(512LL * 1024 * 1024,
                                            availableMemory / 4);
        if (streamOverhead > availableMemory - reserve) {
            if (errorMessage)
                *errorMessage = QCoreApplication::translate("TileManager",
                    "Not enough memory to buffer a single PNG row.");
            return false;
        }
    }
    return true;
}

// Подбирает минимальный формат PNG, в который без потерь укладывается
// текущий вывод: индексный 1/2/4/8 бит для палитровых режимов, RGB для
// непрозрачных многобайтовых, иначе RGBA. Для индексного формата заполняет
// *indexOf (цвет -> номер в PLTE).
PngStream::Format choosePngFormat(const RenderContext &ctx,
                                  const QColor &background,
                                  QHash<QRgb, int> *indexOf)
{
    PngStream::Format format;   // RGBA 8 бит по умолчанию
    indexOf->clear();
    const bool opaqueBackground = background.alpha() == 255;

    QVector<QRgb> source;
    if (ctx.bitpacked) {
        source = BitPackedReader(ctx.bppMode, ctx.bitOrder, ctx.pixelOrder)
                     .buildLut(ctx.palette);
    } else if (ctx.bytesPerPixel == 1) {
        const QRgb *lut = ctx.singleByteReverse ? ctx.singleByteLutReverse
                                                : ctx.singleByteLut;
        source.reserve(258);
        for (int i = 0; i < 256; ++i)
            source.append(lut[i]);
    } else {
        const PixelDecoder::Decoder decoder(ctx.paletteId, ctx.bitOrder,
                                            ctx.byteOrder, ctx.paletteParams);
        if (opaqueBackground && !decoder.mayHaveAlpha())
            format.type = PngStream::ColorType::Rgb;
        return format;
    }

    if (!opaqueBackground)
        return format;

    // Цвета тайла: LUT, чёрный (заливка тайла) и фон (вне изображения).
    QVector<QRgb> colors;
    source.append(qRgb(0, 0, 0));
    source.append(background.rgb());
    for (const QRgb c : source) {
        if (!indexOf->contains(c)) {
            indexOf->insert(c, colors.size());
            colors.append(c);
        }
    }

    if (colors.size() > 256) {
        indexOf->clear();
        format.type = PngStream::ColorType::Rgb;
        return format;
    }

    format.type = PngStream::ColorType::Indexed;
    format.palette = colors;
    const int n = colors.size();
    format.bitDepth = n <= 2 ? 1 : n <= 4 ? 2 : n <= 16 ? 4 : 8;
    return format;
}

// Локальный LRU-кэш тайлов для стримингового экспорта.
// Не трогаем интерактивный кэш TileManager — экспорт идёт в рабочем
// потоке и не должен пересекаться с UI.
class TileCache {
public:
    explicit TileCache(qint64 maxBytes)
        : m_maxBytes(qMax<qint64>(16LL * 1024 * 1024, maxBytes)) {}

    QImage get(const TileKey &key) {
        auto it = m_map.find(key);
        if (it == m_map.end())
            return QImage();
        m_order.splice(m_order.begin(), m_order, it->second);
        return it->second->second;
    }

    void put(const TileKey &key, const QImage &img) {
        if (img.isNull())
            return;
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            m_bytes -= QtCompat::imageBytes(it->second->second);
            it->second->second = img;
            m_order.splice(m_order.begin(), m_order, it->second);
        } else {
            m_order.emplace_front(key, img);
            m_map[key] = m_order.begin();
        }
        m_bytes += QtCompat::imageBytes(img);

        while (m_bytes > m_maxBytes && !m_order.empty()) {
            const std::pair<TileKey, QImage> &back = m_order.back();
            m_bytes -= QtCompat::imageBytes(back.second);
            m_map.erase(back.first);
            m_order.pop_back();
        }
    }

private:
    typedef std::list<std::pair<TileKey, QImage> > Order;

    qint64 m_maxBytes;
    qint64 m_bytes = 0;
    Order m_order;
    std::map<TileKey, Order::iterator> m_map;
};

} // namespace

// =========================================================================
// Worker task for streaming PNG export
// =========================================================================
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

        if (!m_context || cancelled(*m_context)) {
            errorMessage = QCoreApplication::translate(
                "TileManager", "PNG export was cancelled.");
        } else {
            success = exportImage(*m_context, &errorMessage);
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
    bool exportImage(const RenderContext &ctx, QString *errorMessage) const
    {
        const int scale = m_scale;
        const int ds = qMax(1, m_downsample);
        const int outputTileSize = tileOutputSize(ctx.tileSize, ds);
        const qint64 canvasW = m_canvasRect.width();
        const qint64 canvasH = m_canvasRect.height();

        if (outputTileSize <= 0 || canvasW <= 0 || canvasH <= 0) {
            *errorMessage = QCoreApplication::translate(
                "TileManager", "Invalid export dimensions.");
            return false;
        }

        qint64 cacheBudget = 256LL * 1024 * 1024;
        const qint64 avail = BytesSource::availableMemory();
        if (avail > 0)
            cacheBudget = qBound<qint64>(32LL * 1024 * 1024,
                                         avail / 4,
                                         512LL * 1024 * 1024);
        TileCache tileCache(cacheBudget);

        QHash<QRgb, int> indexOf;
        const PngStream::Format format =
            choosePngFormat(ctx, m_background, &indexOf);
        const QRgb bgPx = m_background.rgba();
        const int depth = format.bitDepth;

        auto provideRow = [&](int rowIdx, quint8 *rowData) -> bool {
            if (cancelled(ctx))
                return false;

            const qint64 outY = (qint64(m_canvasRect.y()) + rowIdx) / scale;
            const int tileY  = int(outY / outputTileSize);
            const int localY = int(outY % outputTileSize);

            if (format.type == PngStream::ColorType::Indexed) {
                // Дописываемые ИЛИ-ом байты нужно обнулить заранее.
                std::memset(rowData, 0,
                            size_t(PngStream::rowBytes(format, int(canvasW))));
            }

            quint8 *p = rowData;
            int lastTileX = -1;
            QImage lastTile;
            QRgb lastColor = 0;
            int lastIndex = 0;

            for (qint64 x = 0; x < canvasW; ++x) {
                if ((x & 0x3FF) == 0 && cancelled(ctx))
                    return false;

                const qint64 outX = (qint64(m_canvasRect.x()) + x) / scale;
                const int tileX  = int(outX / outputTileSize);
                const int localX = int(outX % outputTileSize);

                if (tileX != lastTileX) {
                    lastTileX = tileX;
                    const TileKey key(tileX, tileY, ds);
                    lastTile = tileCache.get(key);
                    if (lastTile.isNull()) {
                        lastTile = TileManager::renderTile(ctx, key);
                        tileCache.put(key, lastTile);
                    }
                }

                QRgb px = bgPx;
                if (!lastTile.isNull()
                        && localX < lastTile.width()
                        && localY < lastTile.height()) {
                    px = reinterpret_cast<const QRgb *>(
                        lastTile.constScanLine(localY))[localX];
                }

                switch (format.type) {
                case PngStream::ColorType::Indexed: {
                    if (x == 0 || px != lastColor) {
                        lastColor = px;
                        lastIndex = indexOf.value(px, 0);
                    }
                    const qint64 bit = x * depth;
                    rowData[bit >> 3] |= quint8(
                        lastIndex << (8 - depth - int(bit & 7)));
                    break;
                }
                case PngStream::ColorType::Rgb:
                    *p++ = quint8(qRed(px));
                    *p++ = quint8(qGreen(px));
                    *p++ = quint8(qBlue(px));
                    break;
                case PngStream::ColorType::Rgba:
                    *p++ = quint8(qRed(px));
                    *p++ = quint8(qGreen(px));
                    *p++ = quint8(qBlue(px));
                    *p++ = quint8(qAlpha(px));
                    break;
                }
            }
            return true;
        };

        return PngStream::write(m_path, format, int(canvasW), int(canvasH),
                                provideRow, errorMessage);
    }

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

    if (path.isEmpty())
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

    m_exportContext = context;
    m_pool->start(new PngExportTask(path, context, canvasRect, scale,
                                    downsample, background, this), -1);
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
    if (m_activeTasks > 0 && --m_activeTasks == 0)
        emit activeTasksChanged(0);

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
    const int outputTileSize = tileOutputSize(
        normalizedTileSourceSize(mw->tileSize()), ds);
    const int outputWidth = tileOutputExtent(m_canvas->imageWidth(), ds);
    const int outputHeight = tileOutputExtent(m_canvas->imageHeight(), ds);
    if (outputTileSize <= 0 || outputWidth <= 0 || outputHeight <= 0)
        return;

    const qint64 tileBytes = qint64(outputTileSize) * outputTileSize * 4;
    if (qint64(visible.size()) >= m_cacheLimitBytes / tileBytes)
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

    const int tilesX = (outputWidth  - 1) / outputTileSize + 1;
    const int tilesY = (outputHeight - 1) / outputTileSize + 1;
    const int margin = qBound(0, mw->tilePrefetchMargin(), 32);

    const int beginX = qMax(0, minX - margin);
    const int beginY = qMax(0, minY - margin);
    const int endX = qMin(tilesX - 1, maxX + margin);
    const int endY = qMin(tilesY - 1, maxY + margin);

    int budget = 64;
    for (int ty = beginY; ty <= endY; ++ty) {
        for (int tx = beginX; tx <= endX; ++tx) {
            const TileKey key(tx, ty, ds);
            if (m_tiles.contains(key) || m_pending.contains(key))
                continue;
            if (budget-- <= 0)
                return;
            scheduleRender(key, false);
        }
    }
}

void TileManager::storeTile(const TileKey &key, const QImage &image)
{
    const auto old = m_tiles.constFind(key);
    if (old != m_tiles.constEnd())
        m_cachedBytes -= QtCompat::imageBytes(old->image);

    CacheEntry entry;
    entry.image = image;
    entry.lastUsed = ++m_accessCounter;
    m_tiles.insert(key, entry);
    m_cachedBytes += QtCompat::imageBytes(image);
}

void TileManager::cleanupByMemory()
{
    if (m_cachedBytes <= m_cacheLimitBytes)
        return;

    qint64 nonVisibleBytes = 0;
    QVector<QPair<TileKey, quint64> > order;
    order.reserve(m_tiles.size());
    for (auto it = m_tiles.constBegin(); it != m_tiles.constEnd(); ++it) {
        if (m_visibleTiles.contains(it.key()))
            continue;
        nonVisibleBytes += QtCompat::imageBytes(it->image);
        order.append(qMakePair(it.key(), it->lastUsed));
    }
    if (nonVisibleBytes <= m_cacheLimitBytes)
        return;

    std::sort(order.begin(), order.end(),
              [](const QPair<TileKey, quint64> &a,
                 const QPair<TileKey, quint64> &b) {
                  return a.second < b.second;
              });

    const qint64 target = (m_cacheLimitBytes * 3) / 4;
    for (const QPair<TileKey, quint64> &entry : order) {
        if (nonVisibleBytes <= target)
            break;

        const auto tileIt = m_tiles.find(entry.first);
        if (tileIt == m_tiles.end())
            continue;

        const qint64 bytes = QtCompat::imageBytes(tileIt->image);
        nonVisibleBytes -= bytes;
        m_cachedBytes   -= bytes;
        m_tiles.erase(tileIt);
    }
}
