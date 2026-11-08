// Byte-0-scope - binary data visualizer
// File: ui_hexviewwidget.cpp
// Copyright (C) 2026 pav13
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_hexviewwidget.h"
#include "ui_mainwindow.h"
#include "app_thememanager.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QWheelEvent>

// --- HexViewWidget module settings (prefix HV_) ---

#define HV_DRAG_HANDLE_HEIGHT      6
#define HV_TEXT_TOP_MARGIN         4
#define HV_TEXT_LEFT_MARGIN        4
#define HV_ROW_HEIGHT_EXTRA        2
#define HV_SIDEBAR_CHARS           3
#define HV_SIDEBAR_MARGIN          2
#define HV_MIN_HEIGHT              160
#define HV_MAX_HEIGHT_RESERVE      100
#define HV_ADDR_CHARS              10
#define HV_GROUP_SIZE              8
#define HV_SCROLL_STEP_ROWS        3
#define HV_ASCII_GAP               2
#define HV_BPL_MIN                 8
#define HV_BPL_MAX                 256
#define HV_BPL_STEP                8
#define HV_FONT_PT_MIN             6
#define HV_FONT_PT_MAX             20
#define HV_FONT_PT			       9

HexViewWidget::HexViewWidget(MainWindow *parent)
    : QWidget(parent), m_main(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    setMinimumHeight(HV_MIN_HEIGHT);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_mono.setFamily(QStringLiteral("Monospace"));
    m_mono.setStyleHint(QFont::TypeWriter, QFont::PreferQuality);
    m_mono.setFixedPitch(true);
    applyFontMetrics();

    updateTheme();
    buildSidebar();
    buildScrollbars();
    layoutChildren();
    updateScrollbarRanges();
    updateSidebarButtons();
}

HexViewWidget::~HexViewWidget() = default;

qint64 HexViewWidget::fileSize() const
{
    return m_main ? qint64(m_main->fileSize()) : 0;
}

void HexViewWidget::buildScrollbars()
{
    m_vScroll = new QScrollBar(Qt::Vertical, this);
    m_hScroll = new QScrollBar(Qt::Horizontal, this);

    connect(m_vScroll, &QScrollBar::valueChanged, this, [this](int v) {
        const qint64 newTop = qint64(v) * m_bytesPerLine;
        if (newTop == m_scrollTopAddr)
            return;
        m_scrollTopAddr = newTop;
        update();
    });
    connect(m_hScroll, &QScrollBar::valueChanged, this, [this](int v) {
        if (v == m_hScrollOffset)
            return;
        m_hScrollOffset = v;
        update();
    });
}

void HexViewWidget::updateTheme()
{
    if (m_main && m_main->themeManager()) {
        const Theme &cur = m_main->themeManager()->current();
        m_bg           = cur.colors.value(QStringLiteral("hex_bg"),
                                          cur.colors.value(QStringLiteral("base"), QColor(30, 30, 30)));
        m_bgAlt        = cur.colors.value(QStringLiteral("hex_bg_alt"),
                                          cur.colors.value(QStringLiteral("alternate_base"), QColor(48, 48, 48)));
        m_text         = cur.colors.value(QStringLiteral("hex_text"),
                                          cur.colors.value(QStringLiteral("text"), QColor(220, 220, 220)));
        m_addr         = cur.colors.value(QStringLiteral("hex_addr"),
                                          cur.colors.value(QStringLiteral("window_text"), QColor(180, 180, 180)));
        m_selBg        = cur.colors.value(QStringLiteral("hex_sel_bg"),
                                          cur.colors.value(QStringLiteral("highlight"), QColor(42, 130, 218)));
        m_selText      = cur.colors.value(QStringLiteral("hex_sel_text"),
                                          cur.colors.value(QStringLiteral("highlighted_text"), QColor(255, 255, 255)));
        m_hoverBg      = cur.colors.value(QStringLiteral("hex_hover_bg"), QColor(80, 80, 80, 120));
        m_border       = cur.colors.value(QStringLiteral("hex_border"),
                                          cur.colors.value(QStringLiteral("button"), QColor(74, 74, 74)));
        m_handle       = cur.colors.value(QStringLiteral("hex_handle"),
                                          cur.colors.value(QStringLiteral("button"), QColor(74, 74, 74)));
        m_handleActive = cur.colors.value(QStringLiteral("hex_handle_active"),
                                          cur.colors.value(QStringLiteral("highlight"), QColor(42, 130, 218)));
    } else {
        m_bg = QColor(30, 30, 30);
        m_bgAlt = QColor(48, 48, 48);
        m_text = QColor(220, 220, 220);
        m_addr = QColor(150, 150, 150);
        m_selBg = QColor(42, 130, 218);
        m_selText = QColor(255, 255, 255);
        m_hoverBg = QColor(80, 80, 80, 120);
        m_border = QColor(74, 74, 74);
        m_handle = QColor(74, 74, 74);
        m_handleActive = QColor(42, 130, 218);
    }
    updateSidebarTheme();
    update();
}

// =========================================================================
//  Метрики
// =========================================================================

QRect HexViewWidget::dragHandleRect() const
{
    return QRect(0, 0, width(), HV_DRAG_HANDLE_HEIGHT);
}

int HexViewWidget::textTop()  const { return HV_DRAG_HANDLE_HEIGHT + HV_TEXT_TOP_MARGIN; }
int HexViewWidget::textLeft() const { return HV_TEXT_LEFT_MARGIN; }

QRect HexViewWidget::textAreaRect() const
{
    const int sw = sidebarRect().width();
    return QRect(0, HV_DRAG_HANDLE_HEIGHT,
                 width() - sw, height() - HV_DRAG_HANDLE_HEIGHT);
}

QRect HexViewWidget::sidebarRect() const
{
    const int sw = HV_SIDEBAR_CHARS * m_charWidth + 2 * HV_SIDEBAR_MARGIN + 4;
    return QRect(width() - sw, HV_DRAG_HANDLE_HEIGHT,
                 sw, height() - HV_DRAG_HANDLE_HEIGHT);
}

int HexViewWidget::visibleRows() const
{
    const int avail = height() - textTop() - HV_TEXT_TOP_MARGIN;
    return qMax(1, avail / m_rowHeight);
}

int HexViewWidget::hexCharCount() const
{
    return m_bytesPerLine * 3 + (m_bytesPerLine - 1) / HV_GROUP_SIZE;
}

int HexViewWidget::asciiStartChar() const
{
    return HV_ADDR_CHARS + hexCharCount() + HV_ASCII_GAP;
}

int HexViewWidget::byteHexChar(int n) const
{
    return HV_ADDR_CHARS + n * 3 + n / HV_GROUP_SIZE;
}

int HexViewWidget::byteAsciiChar(int n) const
{
    return asciiStartChar() + n;
}

int HexViewWidget::byteForCharColumn(int col) const
{
    if (col < HV_ADDR_CHARS)
        return -1;
    for (int n = 0; n < m_bytesPerLine; ++n) {
        const int start = byteHexChar(n);
        if (col >= start && col < start + 3)
            return n;
    }
    const int ascStart = asciiStartChar();
    if (col >= ascStart && col < ascStart + m_bytesPerLine)
        return col - ascStart;
    return -1;
}

qint64 HexViewWidget::byteAtPoint(const QPoint &p) const
{
    if (!m_viewRect.contains(p))
        return -1;

    const int row = (p.y() - m_viewRect.top()) / m_rowHeight;
    if (row < 0 || row >= visibleRows())
        return -1;

    const int col = (p.x() - m_viewRect.left() + m_hScrollOffset) / m_charWidth;
    if (col < 0)
        return -1;

    const int byteInRow = byteForCharColumn(col);
    if (byteInRow < 0)
        return -1;

    const qint64 addr = m_scrollTopAddr
                        + qint64(row) * m_bytesPerLine + byteInRow;
    return (addr >= fileSize()) ? -1 : addr;
}

// =========================================================================
//  Public API
// =========================================================================

void HexViewWidget::refresh()
{
    const qint64 fs = fileSize();
    const qint64 maxAddr = qMax<qint64>(0, fs - 1);
    if (m_scrollTopAddr > maxAddr)
        m_scrollTopAddr = (maxAddr / m_bytesPerLine) * m_bytesPerLine;
    if (m_selectedAddr >= fs)
        m_selectedAddr = -1;

    updateScrollbarRanges();
    update();
}

void HexViewWidget::setSelectedAddress(qint64 addr, int bitOffset)
{
    if (addr < 0 || addr >= fileSize()) {
        clearSelection();
        return;
    }
    m_selectedAddr = addr;
    m_selectedBitOffset = qBound(0, bitOffset, 7);
    ensureAddressVisible(addr);
    update();
}

void HexViewWidget::clearSelection()
{
    if (m_selectedAddr < 0 && m_selectedBitOffset == 0)
        return;
    m_selectedAddr = -1;
    m_selectedBitOffset = 0;
    update();
}

// =========================================================================
//  Вспомогательное
// =========================================================================

void HexViewWidget::applyFontMetrics()
{
    m_mono.setPointSize(m_fontPt);
    const QFontMetrics fm(m_mono);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    m_charWidth = qMax(4, fm.horizontalAdvance(QLatin1Char('0')));
#else
    m_charWidth = qMax(4, fm.width(QLatin1Char('0')));
#endif
    m_rowHeight = fm.height() + HV_ROW_HEIGHT_EXTRA;
    m_ascent    = fm.ascent();
}

void HexViewWidget::setBytesPerLine(int n)
{
    int rounded = (n / HV_BPL_STEP) * HV_BPL_STEP;
    rounded = qBound(HV_BPL_MIN, rounded, HV_BPL_MAX);
    if (rounded == m_bytesPerLine)
        return;
    m_bytesPerLine = rounded;
    m_scrollTopAddr = (m_scrollTopAddr / m_bytesPerLine) * m_bytesPerLine;
    m_hScrollOffset = 0;
    updateSidebarButtons();
    updateScrollbarRanges();
    emit bytesPerLineChanged(m_bytesPerLine);
    update();
}

void HexViewWidget::setFontPt(int pt)
{
    const int clamped = qBound(HV_FONT_PT_MIN, pt, HV_FONT_PT_MAX);
    if (clamped == m_fontPt)
        return;
    m_fontPt = clamped;
    applyFontMetrics();

    QFont btnFont = m_mono;
    btnFont.setPointSize(qMax(7, m_fontPt - 1));
    for (QPushButton *b : { m_bplMinusBtn, m_bplPlusBtn,
                             m_fontMinusBtn, m_fontPlusBtn }) {
        if (b) {
            b->setFont(btnFont);
            b->setFixedHeight(m_rowHeight + 4);
        }
    }

    layoutChildren();
    updateScrollbarRanges();
    updateSidebarButtons();
    emit fontPtChanged(m_fontPt);
    update();
}

void HexViewWidget::updateSidebarButtons()
{
    if (m_bplMinusBtn)
        m_bplMinusBtn->setEnabled(m_bytesPerLine > HV_BPL_MIN);
    if (m_bplPlusBtn)
        m_bplPlusBtn->setEnabled(m_bytesPerLine < HV_BPL_MAX);
    if (m_fontMinusBtn)
        m_fontMinusBtn->setEnabled(m_fontPt > HV_FONT_PT_MIN);
    if (m_fontPlusBtn)
        m_fontPlusBtn->setEnabled(m_fontPt < HV_FONT_PT_MAX);
}

void HexViewWidget::scrollRows(int rows)
{
    const qint64 fs = fileSize();
    const qint64 maxTop = qMax<qint64>(0, fs - 1);
    m_scrollTopAddr += qint64(rows) * m_bytesPerLine;
    m_scrollTopAddr = qBound<qint64>(0, m_scrollTopAddr, maxTop);
    m_scrollTopAddr = (m_scrollTopAddr / m_bytesPerLine) * m_bytesPerLine;
    update();
}

void HexViewWidget::ensureAddressVisible(qint64 addr)
{
    if (addr < 0 || !m_vScroll)
        return;

    const int rows = visibleRows();
    const qint64 viewSize = qint64(rows) * m_bytesPerLine;
    if (addr >= m_scrollTopAddr && addr < m_scrollTopAddr + viewSize)
        return;

    const int targetRow = int(addr / m_bytesPerLine) - rows / 2;
    const int clamped = qBound(m_vScroll->minimum(),
                               targetRow,
                               m_vScroll->maximum());
    m_vScroll->setValue(clamped);
}

// =========================================================================
//  Sidebar
// =========================================================================

void HexViewWidget::buildSidebar()
{
    m_sidebar = new QWidget(this);
    m_sidebar->setAutoFillBackground(true);

    m_sidebarLayout = new QVBoxLayout(m_sidebar);
    m_sidebarLayout->setContentsMargins(HV_SIDEBAR_MARGIN, HV_SIDEBAR_MARGIN,
                                        HV_SIDEBAR_MARGIN, HV_SIDEBAR_MARGIN);
    m_sidebarLayout->setSpacing(2);

    QFont btnFont = m_mono;
    btnFont.setPointSize(qMax(7, HV_FONT_PT - 1));

    auto makeBtn = [&](const QString &text, const QString &tip) -> QPushButton * {
        auto *b = new QPushButton(text, m_sidebar);
        b->setFont(btnFont);
        b->setToolTip(tip);
        b->setAutoDefault(false);
        b->setFocusPolicy(Qt::NoFocus);
        b->setFixedHeight(m_rowHeight + 4);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return b;
    };

	m_bplMinusBtn = makeBtn(QStringLiteral("-8"),
							tr("Decrease bytes per line by 8"));
	m_bplPlusBtn  = makeBtn(QStringLiteral("+8"),
							tr("Increase bytes per line by 8"));
	connect(m_bplMinusBtn, &QPushButton::clicked, this, [this]() {
		setBytesPerLine(m_bytesPerLine - HV_BPL_STEP);
	});
	connect(m_bplPlusBtn, &QPushButton::clicked, this, [this]() {
		setBytesPerLine(m_bytesPerLine + HV_BPL_STEP);
	});

    auto *gotoBtn = makeBtn(QStringLiteral(">>"), tr("Go to selection"));
    connect(gotoBtn, &QPushButton::clicked, this, [this]() {
        if (m_selectedAddr >= 0)
            ensureAddressVisible(m_selectedAddr);
        update();
    });
	
	m_fontMinusBtn = makeBtn(QStringLiteral("A-"), tr("Decrease font size"));
	m_fontPlusBtn  = makeBtn(QStringLiteral("A+"), tr("Increase font size"));
	connect(m_fontMinusBtn, &QPushButton::clicked, this, [this]() {
		setFontPt(m_fontPt - 1);
	});
	connect(m_fontPlusBtn, &QPushButton::clicked, this, [this]() {
		setFontPt(m_fontPt + 1);
	});

    // Заглушки на будущее
    auto *phOptions1  = makeBtn(QStringLiteral("?"),
                              tr("More options (coming soon)"));
    auto *phOptions2 = makeBtn(QStringLiteral("?"),
                              tr("More options (coming soon)"));
    phOptions1->setEnabled(false);
    phOptions2->setEnabled(false);

	m_sidebarLayout->addWidget(m_bplMinusBtn);
	m_sidebarLayout->addWidget(m_bplPlusBtn);
	m_sidebarLayout->addWidget(gotoBtn);
	m_sidebarLayout->addSpacing(6);
	m_sidebarLayout->addWidget(m_fontMinusBtn);
	m_sidebarLayout->addWidget(m_fontPlusBtn);
	m_sidebarLayout->addSpacing(6);
	m_sidebarLayout->addWidget(phOptions1);
	m_sidebarLayout->addWidget(phOptions2);
	m_sidebarLayout->addStretch();

    updateSidebarTheme();
}

void HexViewWidget::layoutChildren()
{
    const int sbW = m_vScroll ? qMax(10, m_vScroll->sizeHint().width())  : 14;
    const int sbH = m_hScroll ? qMax(10, m_hScroll->sizeHint().height()) : 14;

    const QRect sa = sidebarRect();
    if (m_sidebar)
        m_sidebar->setGeometry(sa);

    const int textX = HV_TEXT_LEFT_MARGIN;
    const int textY = HV_DRAG_HANDLE_HEIGHT;
    const int textW = qMax(0, width() - sa.width() - textX);
    const int textH = qMax(0, height() - textY);

    m_viewRect = QRect(textX, textY,
                       qMax(0, textW - sbW),
                       qMax(0, textH - sbH));

    if (m_vScroll)
        m_vScroll->setGeometry(textX + textW - sbW, textY, sbW, textH);
    if (m_hScroll)
        m_hScroll->setGeometry(textX, textY + textH - sbH, textW - sbW, sbH);
}

int HexViewWidget::contentPixels() const
{
    return (asciiStartChar() + m_bytesPerLine + 1) * m_charWidth;
}

void HexViewWidget::updateScrollbarRanges()
{
    const qint64 fs = fileSize();
    const qint64 rows = (fs + m_bytesPerLine - 1) / m_bytesPerLine;

    if (m_vScroll) {
        QSignalBlocker b(m_vScroll);
        m_vScroll->setRange(0, int(qMax<qint64>(0, rows - 1)));
        m_vScroll->setPageStep(qMax(1, visibleRows()));
        m_vScroll->setSingleStep(1);
        int line = int(m_scrollTopAddr / m_bytesPerLine);
        line = qBound(m_vScroll->minimum(), line, m_vScroll->maximum());
        m_vScroll->setValue(line);
        m_scrollTopAddr = qint64(line) * m_bytesPerLine;
    }

    if (m_hScroll) {
        QSignalBlocker b(m_hScroll);
        const int contentW = contentPixels();
        const int pageW    = m_viewRect.width();
        m_hScroll->setRange(0, qMax(0, contentW - pageW));
        m_hScroll->setPageStep(qMax(1, pageW));
        m_hScroll->setSingleStep(m_charWidth);
        const int off = qBound(m_hScroll->minimum(),
                               m_hScrollOffset,
                               m_hScroll->maximum());
        m_hScroll->setValue(off);
        m_hScrollOffset = off;
    }
}

void HexViewWidget::updateSidebarTheme()
{
    if (!m_sidebar)
        return;
    QPalette pal = m_sidebar->palette();
    pal.setColor(QPalette::Window, m_bg);
    m_sidebar->setPalette(pal);
}

void HexViewWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    layoutChildren();
    updateScrollbarRanges();
}

// =========================================================================
//  Отрисовка
// =========================================================================

void HexViewWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    QPainter p(this);

    p.fillRect(rect(), m_bg);
    p.setPen(m_border);
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    drawDragHandle(p);
    drawHexArea(p);
}

void HexViewWidget::drawDragHandle(QPainter &p)
{
    const QRect hr = dragHandleRect();
    p.fillRect(hr, m_dragMode == DragMode::Height ? m_handleActive : m_handle);

    p.setPen(QColor(220, 220, 220, 180));
    const int cx = hr.center().x();
    const int cy = hr.center().y();
    for (int i = -2; i <= 2; ++i)
        p.drawPoint(cx + i * 5, cy);
}

void HexViewWidget::drawHexArea(QPainter &p)
{
    p.setFont(m_mono);
    p.setClipRect(m_viewRect);

    const qint64 fs = fileSize();
    const qint64 lastAddr = qMax<qint64>(0, fs - 1);
    const int addrDigits = qMax(2, QString::number(lastAddr, 16).size());

    const int rows = visibleRows();
    const int baseX = m_viewRect.left() - m_hScrollOffset;
    const int baseY = m_viewRect.top();

    for (int row = 0; row < rows; ++row) {
        const qint64 baseAddr = m_scrollTopAddr + qint64(row) * m_bytesPerLine;
        if (baseAddr >= fs)
            break;
        drawRow(p, row, baseAddr, addrDigits, baseX, baseY);
    }
    p.setClipping(false);
}

void HexViewWidget::drawRow(QPainter &p, int row, qint64 baseAddr,
                            int addrDigits, int baseX, int baseY)
{
    const int y = baseY + row * m_rowHeight;
    const int baseline = y + (m_rowHeight + m_ascent) / 2 - 2;

	const QString addrText =
    QStringLiteral("%1:")
        .arg(baseAddr, addrDigits, 16, QLatin1Char('0'))
        .toUpper();
    p.setPen(m_addr);
    p.drawText(baseX, baseline, addrText);

    const qint64 fs = fileSize();
    for (int n = 0; n < m_bytesPerLine; ++n) {
        const qint64 addr = baseAddr + n;
        if (addr >= fs)
            break;

        const int hexX   = baseX + byteHexChar(n)   * m_charWidth;
        const int asciiX = baseX + byteAsciiChar(n) * m_charWidth;

        const bool selected = (addr == m_selectedAddr);
        const bool hovered  = !selected && (addr == m_hoverAddr);

        if (selected || hovered) {
            const QColor bg = selected ? m_selBg : m_hoverBg;
            p.fillRect(QRect(hexX,   y, m_charWidth * 2, m_rowHeight), bg);
            p.fillRect(QRect(asciiX, y, m_charWidth,     m_rowHeight), bg);
        }

        const quint8 byte = m_main ? m_main->readByte(addr) : 0;

        const QString hex = QStringLiteral("%1")
            .arg(byte, 2, 16, QLatin1Char('0')).toUpper();
        p.setPen(selected ? m_selText : m_text);
        p.drawText(hexX, baseline, hex);

        const QChar ch = (byte >= 0x20 && byte <= 0x7E)
            ? QChar(byte) : QChar(0x00B7);
        p.setPen(selected ? m_selText : m_text);
        p.drawText(asciiX, baseline, QString(ch));
    }
}

// =========================================================================
//  Мышь
// =========================================================================

void HexViewWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(e);
        return;
    }
    if (dragHandleRect().contains(e->pos())) {
        m_dragMode = DragMode::Height;
        m_dragStartGlobal = e->globalPos();
        m_dragStartHeight = height();
        e->accept();
        return;
    }
    const qint64 addr = byteAtPoint(e->pos());
    if (addr >= 0) {
        m_selectedAddr = addr;
        m_selectedBitOffset = 0;
        emit byteSelected(addr, 0);
        update();
        e->accept();
        return;
    }
    QWidget::mousePressEvent(e);
}

void HexViewWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragMode == DragMode::Height) {
        const int delta = e->globalPos().y() - m_dragStartGlobal.y();
        const int avail = parentWidget()
            ? parentWidget()->height() - HV_MAX_HEIGHT_RESERVE
            : 1500;
        const int maxH  = qMax(HV_MIN_HEIGHT, avail);
        const int newHeight = qBound(HV_MIN_HEIGHT,
                                     m_dragStartHeight - delta,
                                     maxH);
        if (newHeight != height()) {
            setFixedHeight(newHeight);
            update();
        }
        e->accept();
        return;
    }

    const qint64 hov = byteAtPoint(e->pos());
    if (hov != m_hoverAddr) {
        m_hoverAddr = hov;
        update();
    }
    setCursor(dragHandleRect().contains(e->pos())
                  ? Qt::SizeVerCursor : Qt::ArrowCursor);
    QWidget::mouseMoveEvent(e);
}

void HexViewWidget::mouseReleaseEvent(QMouseEvent *e)
{
	if (m_dragMode == DragMode::Height && e->button() == Qt::LeftButton) {
		m_dragMode = DragMode::None;
		emit heightChanged(height());
		e->accept();
		return;
	}
    QWidget::mouseReleaseEvent(e);
}

void HexViewWidget::wheelEvent(QWheelEvent *e)
{
    const int dy = e->angleDelta().y();
    if (dy == 0) {
        e->ignore();
        return;
    }
    int steps = dy / 120;
    if (steps == 0)
        steps = dy > 0 ? 1 : -1;
    scrollRows(-steps * HV_SCROLL_STEP_ROWS);
    e->accept();
}

void HexViewWidget::leaveEvent(QEvent *e)
{
    setCursor(Qt::ArrowCursor);
    if (m_hoverAddr != -1) {
        m_hoverAddr = -1;
        update();
    }
    QWidget::leaveEvent(e);
}