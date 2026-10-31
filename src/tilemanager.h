// Byte-0-scope - binary data visualizer
// File: tilemanager.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "bytesource.h"

#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QImage>
#include <QMetaType>
#include <QObject>
#include <QSet>
#include <QVariantMap>
#include <QVector>

#include <atomic>
#include <memory>

class CanvasWidget;
class MainWindow;
class QThreadPool;

struct TileKey {
    int tx;
    int ty;
    int ds;

    TileKey() : tx(0), ty(0), ds(1) {}
    TileKey(int t_x, int t_y, int d = 1) : tx(t_x), ty(t_y), ds(d) {}

    bool operator==(const TileKey &o) const {
        return tx == o.tx && ty == o.ty && ds == o.ds;
    }
};

Q_DECLARE_METATYPE(TileKey)

inline uint qHash(const TileKey &k, uint seed = 0) Q_DECL_NOTHROW {
    return qHash(k.tx, seed) ^ qHash(k.ty, seed << 1) ^ qHash(k.ds, seed << 2);
}

struct CancelFlag {
    std::atomic<bool> flag{false};
    void cancel() noexcept              { flag.store(true,  std::memory_order_release); }
    bool isCancelled() const noexcept   { return flag.load(std::memory_order_acquire); }
};

using CancelFlagPtr = std::shared_ptr<CancelFlag>;

struct RenderContext {
    QByteArray    data;
    MappedFilePtr mapped;

    qint64     regionStart = 0;
    qint64     regionEnd   = 0;
    int        imageWidth  = 0;
    int        imageHeight = 0;
    int        tileSize    = 448;

    bool       bitpacked      = false;
    int        bppMode        = 8;
    int        bytesPerPixel  = 1;
    int        rowPaddingBits = 0;
    QString    bitOrder;
    QString    pixelOrder;

    bool       mirrorH = false;
    bool       mirrorV = false;

    QString         paletteId;
    QVector<QColor> palette;
    QVariantMap     paletteParams;

    CancelFlagPtr cancel;

    const uchar *bytes() const {
        if (mapped && mapped->mapped)
            return mapped->mapped;
        return reinterpret_cast<const uchar *>(data.constData());
    }

    qint64 byteSize() const {
        if (mapped)
            return mapped->size;
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
    void   prefetchAround(const QSet<TileKey> &visible);
    void   setVisibleTiles(const QSet<TileKey> &visible);

    void   clearCache();
    void   setCacheLimitMb(int mb);

    int    currentTileSize() const;
    qint64 cacheBytes() const;
    int    cacheLimitMb() const;
    int    activeTasks() const { return m_activeTasks; }

signals:
    void tileReady(const TileKey &key);
    void activeTasksChanged(int count);

private slots:
    void onTileRendered(const TileKey &key, const QImage &img, quint64 gen);

private:
    RenderContextPtr buildContext() const;
    void             scheduleRender(const TileKey &key, bool highPriority);
    void             cleanupByMemory();
    qint64           now() const;

    CanvasWidget *m_canvas = nullptr;

    RenderContextPtr m_context;
    quint64          m_generation = 0;

    QHash<TileKey, QImage> m_tiles;
    QHash<TileKey, qint64> m_accessTimes;
    QSet<TileKey>          m_pending;
    QSet<TileKey>          m_visibleTiles;

    int           m_activeTasks     = 0;
    qint64        m_cacheLimitBytes = 512LL * 1024 * 1024;
    QThreadPool  *m_pool = nullptr;
};