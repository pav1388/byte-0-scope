// Byte-0-scope - binary data visualizer
// File: render_tilemanager.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core_bytesource.h"

#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QImage>
#include <QMetaType>
#include <QObject>
#include <QSet>
#include <QRect>
#include <QSize>
#include <QVariantMap>
#include <QVector>
#include <QtGlobal>
#include <atomic>
#include <memory>

class CanvasWidget;
class MainWindow;
class QThreadPool;

// TileKey::tx/ty are tile indices in the downsampled output grid.
struct TileKey {
    int tx;
    int ty;
    int ds;

    TileKey() : tx(0), ty(0), ds(1) {}
    TileKey(int t_x, int t_y, int d = 1) : tx(t_x), ty(t_y), ds(d) {}

    bool operator==(const TileKey &o) const {
        return tx == o.tx && ty == o.ty && ds == o.ds;
    }
	
    bool operator<(const TileKey &o) const {
        if (ds != o.ds) return ds < o.ds;
        if (ty != o.ty) return ty < o.ty;
        return tx < o.tx;
    }
};

Q_DECLARE_METATYPE(TileKey)

inline uint qHash(const TileKey &k, uint seed = 0) Q_DECL_NOTHROW
{
    uint h = qHash(k.tx, seed);
    h = qHash(k.ty, h ^ 0x9e3779b9U);
    h = qHash(k.ds, h ^ 0x85ebca6bU);
    return h;
}

inline int tileOutputExtent(int sourceExtent, int downsample)
{
    if (sourceExtent <= 0)
        return 0;
    const qint64 ds = qMax(1, downsample);
    return int((qint64(sourceExtent) + ds - 1) / ds);
}

inline int tileOutputSize(int sourceTileSize, int downsample)
{
    const qint64 ts = qMax(1, sourceTileSize);
    const qint64 ds = qMax(1, downsample);
    return int(qMax<qint64>(64, (ts + ds - 1) / ds));
}

inline int normalizedTileSourceSize(int sourceTileSize)
{
    return qBound(16, sourceTileSize, 2048);
}

struct CancelFlag {
    std::atomic<bool> flag{false};
    void cancel() noexcept             { flag.store(true, std::memory_order_release); }
    bool isCancelled() const noexcept  { return flag.load(std::memory_order_acquire); }
};

using CancelFlagPtr = std::shared_ptr<CancelFlag>;

struct RenderContext {
    QByteArray data;
    MappedFilePtr mapped;

    qint64 regionStart = 0;
    qint64 regionEnd = 0;
    int imageWidth = 0;
    int imageHeight = 0;
    int tileSize = 448;

    bool bitpacked = false;
    int bppMode = 8;
    int bytesPerPixel = 1;
    int rowPaddingBits = 0;
    QString bitOrder;
    QString pixelOrder;
    QString byteOrder;

    bool mirrorH = false;
    bool mirrorV = false;

    QString paletteId;
    QVector<QColor> palette;
    QVariantMap paletteParams;

    CancelFlagPtr cancel;

    QRgb singleByteLut[256] = {};
    QRgb singleByteLutReverse[256] = {};
    bool singleByteLutValid = false;
    bool singleByteReverse = false;

    const uchar *bytes() const
    {
        if (mapped)
            return mapped->mapped;
        return data.isEmpty()
            ? nullptr
            : reinterpret_cast<const uchar *>(data.constData());
    }

    qint64 byteSize() const
    {
        if (mapped)
            return mapped->mapped ? mapped->size : 0;
        return data.size();
    }
};

using RenderContextPtr = std::shared_ptr<const RenderContext>;

class TileManager : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(TileManager)

public:
    explicit TileManager(CanvasWidget *canvas);
    ~TileManager() override;

    static QImage renderTile(const RenderContext &ctx, const TileKey &key);

    QImage getTile(int tileX, int tileY);
    QImage renderImage(const QSize &targetSize, int scale, int downsample) const;
    bool startPngExport(const QString &path, const QRect &canvasRect,
                        int scale, int downsample, const QColor &background,
                        QString *errorMessage = nullptr);
    void prefetchAround(const QSet<TileKey> &visible);
    void setVisibleTiles(const QSet<TileKey> &visible);

    void clearCache();
    void setCacheLimitMb(int mb);

    int currentTileSize() const;
    qint64 cacheBytes() const;
    int cacheLimitMb() const;
    int activeTasks() const { return m_activeTasks; }

signals:
    void tileReady(const TileKey &key);
    void activeTasksChanged(int count);
    void pngExportFinished(const QString &path, bool success,
                           const QString &errorMessage);

private slots:
    void onTileRendered(const TileKey &key, const QImage &img, quint64 gen);
    void onPngExportTaskFinished(const QString &path, bool success,
                                 const QString &errorMessage);

private:
    RenderContextPtr buildContext() const;
    void scheduleRender(const TileKey &key, bool highPriority);
    void storeTile(const TileKey &key, const QImage &image);
    void cleanupByMemory();

    CanvasWidget *m_canvas = nullptr;
    RenderContextPtr m_context;
    RenderContextPtr m_exportContext;
    quint64 m_generation = 0;

    struct CacheEntry {
        QImage  image;
        quint64 lastUsed = 0;
    };
    QHash<TileKey, CacheEntry> m_tiles;
    quint64 m_accessCounter = 0;
    qint64  m_cachedBytes = 0;
    QSet<TileKey> m_pending;
    QSet<TileKey> m_visibleTiles;

    int m_activeTasks = 0;
    qint64 m_cacheLimitBytes = 512LL * 1024 * 1024;
    QThreadPool *m_pool = nullptr;
};