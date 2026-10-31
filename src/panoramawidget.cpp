// Byte-0-scope - binary data visualizer
// File: panoramawidget.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "panoramawidget.h"
#include "mainwindow.h"
#include "thememanager.h"

#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>

// --- Настройки модуля PanoramaWidget (префикс PW_) ---

#define PW_FIXED_HEIGHT             130
#define PW_CONTENT_MARGIN           10
#define PW_MIN_VIEWPORT_PX          16
#define PW_HOVER_COLOR_DELTA        30
#define PW_HOVER_ALPHA              180
#define PW_HOVER_FALLBACK_ALPHA     210
#define PW_DRAG_PEN_COLOR           QColor(255, 255, 0)
#define PW_DRAG_PEN_WIDTH           2

PanoramaWidget::PanoramaWidget(MainWindow *parent)
    : QWidget(parent), m_main(parent)
{
    setFixedHeight(PW_FIXED_HEIGHT);
    setMouseTracking(true);
    setCursor(Qt::ArrowCursor);
    refreshColors();
}

void PanoramaWidget::refreshColors()
{
    if (m_main && m_main->themeManager()) {
        const Theme &t = m_main->themeManager()->current();
        m_bg             = t.colors.value("panorama_bg",             QColor(30, 30, 30));
        m_img            = t.colors.value("panorama_image",          QColor(60, 60, 60));
        m_imgBorder      = t.colors.value("panorama_image_border",   QColor(100, 100, 100));
        m_viewport       = t.colors.value("panorama_viewport",       QColor(255, 255, 255, 180));
        m_viewportBorder = t.colors.value("panorama_viewport_border",QColor(255, 0, 0));
        m_text           = t.colors.value("window_text",             QColor(150, 150, 150));

        QColor hv = m_viewport;
        hv.setRed  (qMin(255, hv.red()   + PW_HOVER_COLOR_DELTA));
        hv.setGreen(qMin(255, hv.green() + PW_HOVER_COLOR_DELTA));
        hv.setBlue (qMin(255, hv.blue()  + PW_HOVER_COLOR_DELTA));
        hv.setAlpha(PW_HOVER_ALPHA);
        m_hover = hv;
        return;
    }

    m_bg             = QColor(30, 30, 30);
    m_img            = QColor(60, 60, 60);
    m_imgBorder      = QColor(100, 100, 100);
    m_viewport       = QColor(255, 255, 255, 180);
    m_viewportBorder = QColor(255, 0, 0);
    m_text           = QColor(150, 150, 150);
    m_hover          = QColor(255, 255, 255, PW_HOVER_FALLBACK_ALPHA);
}

QRect PanoramaWidget::computeViewportRect() const
{
    if (!m_main || m_main->imageWidth() <= 0)
        return QRect();
    QScrollArea *sa = m_main->scrollArea();
    if (!sa)
        return QRect();

    const int imgW = m_main->imageWidth();
    const int imgH = m_main->imageHeight();
    const double canvasScale = qMax(1, m_main->scale());

    const double viewX = sa->horizontalScrollBar()->value() / canvasScale;
    const double viewY = sa->verticalScrollBar()->value()   / canvasScale;
    const double viewW = sa->viewport()->width()  / canvasScale;
    const double viewH = sa->viewport()->height() / canvasScale;

    const double w = qMax(0.0, qMin(viewW, imgW - viewX));
    const double h = qMax(0.0, qMin(viewH, imgH - viewY));

    const int sx = m_dx + int(viewX * m_scale);
    const int sy = m_dy + int(viewY * m_scale);
    const int sw = qMax(PW_MIN_VIEWPORT_PX, int(w * m_scale));
    const int sh = qMax(PW_MIN_VIEWPORT_PX, int(h * m_scale));
    return QRect(sx, sy, sw, sh);
}

void PanoramaWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), m_bg);

    if (!m_main || m_main->imageWidth() <= 0) {
        p.setPen(m_text);
        p.drawText(rect(), Qt::AlignCenter, tr("No image"));
        return;
    }

    const int ww = width()  - PW_CONTENT_MARGIN;
    const int wh = height() - PW_CONTENT_MARGIN;
    const int iw = m_main->imageWidth();
    const int ih = m_main->imageHeight();
    if (iw <= 0 || ih <= 0)
        return;

    m_scale = qMin(double(ww) / iw, double(wh) / ih);
    m_dw = int(iw * m_scale);
    m_dh = int(ih * m_scale);
    m_dx = (width()  - m_dw) / 2;
    m_dy = (height() - m_dh) / 2;

    p.fillRect(m_dx, m_dy, m_dw, m_dh, m_img);
    p.setPen(m_imgBorder);
    p.drawRect(m_dx, m_dy, m_dw, m_dh);

    // ВАЖНО: пересчитываем кубик из скроллбаров ТОЛЬКО когда пользователь
    // его не тащит. Во время drag m_currentViewport уже содержит актуальную
    // (сдвинутую) позицию — перезапись возвращала бы его обратно,
    // из-за чего drag визуально «не работал».
    if (!m_dragging)
        m_currentViewport = computeViewportRect();

    if (!m_currentViewport.isNull()) {
        const bool hovered = m_currentViewport.contains(m_lastMouse);
        p.fillRect(m_currentViewport,
                   (hovered || m_dragging) ? m_hover : m_viewport);
        p.setPen(m_viewportBorder);
        p.drawRect(m_currentViewport);
        if (m_dragging) {
            p.setPen(QPen(PW_DRAG_PEN_COLOR, PW_DRAG_PEN_WIDTH, Qt::DashLine));
            p.drawRect(m_currentViewport);
        }
    }
}

void PanoramaWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        // Используем то, что реально нарисовано, а не пересчёт «на лету».
        if (!m_currentViewport.isNull()
                && m_currentViewport.contains(e->pos())) {
            m_dragging  = true;
            m_dragStart = e->pos();
            m_vpStart   = m_currentViewport.topLeft();
            setCursor(Qt::ClosedHandCursor);
            update();
            e->accept();
            return;
        }
    }
    QWidget::mousePressEvent(e);
}

void PanoramaWidget::mouseMoveEvent(QMouseEvent *e)
{
    m_lastMouse = e->pos();

    if (m_dragging) {
        const QPoint d = e->pos() - m_dragStart;
        int nx = m_vpStart.x() + d.x();
        int ny = m_vpStart.y() + d.y();

        // Ограничиваем кубик границами миниатюры.
        // Если кубик больше картинки (весь файл влезает) — maxX/maxY < min,
        // явный qMax(min, qMin(nx, max)) оставляет кубик на min.
        const int minX = m_dx;
        const int minY = m_dy;
        const int maxX = m_dx + m_dw - m_currentViewport.width();
        const int maxY = m_dy + m_dh - m_currentViewport.height();
        nx = qMax(minX, qMin(nx, maxX));
        ny = qMax(minY, qMin(ny, maxY));

        m_currentViewport.moveTopLeft(QPoint(nx, ny));
        update();
    } else {
        setCursor((!m_currentViewport.isNull()
                    && m_currentViewport.contains(e->pos()))
                  ? Qt::OpenHandCursor : Qt::ArrowCursor);
        update();
    }
}

void PanoramaWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;

        if (m_scale > 0 && m_main) {
            const double imgX = (m_currentViewport.x() - m_dx) / m_scale;
            const double imgY = (m_currentViewport.y() - m_dy) / m_scale;
            const int sx = int(imgX * qMax(1, m_main->scale()));
            const int sy = int(imgY * qMax(1, m_main->scale()));
            emit scrollToRequested(sx, sy);
        }

        // Скроллбары уже обновлены (emit синхронный) — можно пересчитать кубик.
        m_currentViewport = computeViewportRect();

        setCursor((!m_currentViewport.isNull()
                    && m_currentViewport.contains(e->pos()))
                  ? Qt::OpenHandCursor : Qt::ArrowCursor);
        update();
        e->accept();
        return;
    }
    QWidget::mouseReleaseEvent(e);
}

void PanoramaWidget::leaveEvent(QEvent *e)
{
    m_lastMouse = QPoint(-1, -1);
    setCursor(Qt::ArrowCursor);
    update();
    QWidget::leaveEvent(e);
}