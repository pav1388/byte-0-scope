// Byte-0-scope - binary data visualizer
// File: ui_hexviewwidget.h
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>
#include <QFont>
#include <QColor>
#include <QPoint>
#include <QRect>

class MainWindow;
class QScrollBar;
class QPushButton;
class QWheelEvent;
class QMouseEvent;
class QResizeEvent;
class QPaintEvent;

class HexViewWidget : public QWidget {
    Q_OBJECT
    Q_DISABLE_COPY(HexViewWidget)

public:
    explicit HexViewWidget(MainWindow *parent);

    void refresh();                 // данные могли измениться (файл/xor/training)
    void updateTheme();             // тема переключилась
    int  bytesPerLine() const { return m_bytesPerLine; }
    void setBytesPerLine(int n);
    int  fontPt() const       { return m_fontPt; }
    void setFontPt(int pt);

public slots:
    void setSelectedAddress(qint64 addr, int bitOffset = 0);
    void clearSelection();

signals:
    void byteSelected(qint64 addr, int bitOffset);
    void bytesPerLineChanged(int n);
    void fontPtChanged(int pt);
    void heightChanged(int h);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    enum class DragMode { None, Height };

    // Метрики
    QRect dragHandleRect() const;
    QRect sidebarRect()    const;
    int   visibleRows()    const;
    int   contentPixels()  const;
    void  applyFontMetrics();

    // Раскладка символов в строке
    int addrDigits()     const;
    int addrChars()      const;
    int hexCharCount()   const;
    int asciiStartChar() const;
    int byteHexChar(int byteInRow)   const;
    int byteAsciiChar(int byteInRow) const;
    int byteForCharColumn(int col)   const;

    // Hit-test
    qint64 byteAtPoint(const QPoint &p) const;

    // Отрисовка
    void drawDragHandle(QPainter &p);
    void drawHeaderRow(QPainter &p);
    void drawHexArea(QPainter &p);
    void drawRow(QPainter &p, int row, qint64 baseAddr, int addrDigits,
                 int baseX, int baseY);

    // Дочерние виджеты: sidebar, полосы прокрутки
    void  buildSidebar();
    void  buildScrollbars();
    void  layoutChildren();
    void  updateScrollbarRanges();
    void  updateSidebarTheme();
    void  updateSidebarButtons();
    QFont sidebarButtonFont() const;

    // Вспомогательное
    void   scrollRows(int rows);
    void   ensureAddressVisible(qint64 addr);
    qint64 fileSize() const;

    MainWindow *m_main = nullptr;

    int    m_bytesPerLine      = 16;
    qint64 m_scrollTopAddr     = 0;
    qint64 m_selectedAddr      = -1;
    int    m_selectedBitOffset = 0;
    qint64 m_hoverAddr         = -1;

    QWidget     *m_sidebar      = nullptr;
    QPushButton *m_bplMinusBtn  = nullptr;
    QPushButton *m_bplPlusBtn   = nullptr;
    QPushButton *m_gotoBtn      = nullptr;
    QPushButton *m_fontMinusBtn = nullptr;
    QPushButton *m_fontPlusBtn  = nullptr;

    QScrollBar *m_vScroll = nullptr;
    QScrollBar *m_hScroll = nullptr;
    int         m_hScrollOffset = 0;
    QRect       m_viewRect;

    DragMode m_dragMode = DragMode::None;
    QPoint   m_dragStartGlobal;
    int      m_dragStartHeight = 0;

    QFont m_mono;
    QFont m_monoBold;
    int   m_fontPt     = 9;
    int   m_charWidth  = 8;
    int   m_rowHeight  = 14;
    int   m_ascent     = 10;

    QColor m_bg, m_bgAlt, m_text, m_addr;
    QColor m_selBg, m_selText, m_hoverBg;
    QColor m_border, m_handle, m_handleActive;
};
