// Byte-0-scope - binary data visualizer
// File: panoramawidget.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>
#include <QColor>
#include <QRect>
#include <QPoint>

class MainWindow;

class PanoramaWidget : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(PanoramaWidget)

public:
    explicit PanoramaWidget(MainWindow *parent);

    void updatePanorama() { update(); }
    void updateTheme()    { refreshColors(); update(); }

signals:
    void scrollToRequested(int x, int y);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *e) override;

private:
    void  refreshColors();
    QRect computeViewportRect() const;

    MainWindow *m_main = nullptr;

    QColor m_bg;
    QColor m_img;
    QColor m_imgBorder;
    QColor m_viewport;
    QColor m_viewportBorder;
    QColor m_text;
    QColor m_hover;

    bool   m_dragging = false;
    QPoint m_dragStart;
    QPoint m_vpStart;
    QPoint m_lastMouse = QPoint(-1, -1);

    double m_scale = 1.0;
    int    m_dx = 0;
    int    m_dy = 0;
    int    m_dw = 0;
    int    m_dh = 0;
    QRect  m_currentViewport;
};