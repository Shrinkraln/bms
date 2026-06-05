#include "plotwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

static const QColor kCellColors[5] = {
    QColor(100, 220, 100, 140),   // C1 绿
    QColor(100, 180, 255, 140),   // C2 蓝
    QColor(255, 200, 80,  140),   // C3 黄
    QColor(255, 120, 180, 140),   // C4 粉
    QColor(180, 140, 255, 140),   // C5 紫
};

PlotWidget::PlotWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(260);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(18, 18, 24));
    setPalette(pal);
}

void PlotWidget::addSample(double t_sec, double voltage_V, double current_A,
                           const double cellV[5])
{
    Pt pt;
    pt.t = t_sec; pt.v = voltage_V; pt.i = current_A;
    for (int i = 0; i < 5; ++i) pt.cell[i] = cellV[i];
    m_pts.append(pt);
    while (m_pts.size() > m_maxPts) m_pts.removeFirst();
    update();
}

void PlotWidget::clear()
{
    m_pts.clear();
    update();
}

void PlotWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int L = 52, R = 52, T = 14, B = 24;
    QRectF area(L, T, width() - L - R, height() - T - B);
    if (area.width() < 20 || area.height() < 20) return;

    // 背景网格
    p.setPen(QPen(QColor(40, 42, 54), 1));
    p.drawRect(area);
    for (int g = 1; g < 5; ++g) {
        double y = area.top() + area.height() * g / 5.0;
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }
    // 垂直网格
    for (int g = 1; g < 6; ++g) {
        double x = area.left() + area.width() * g / 6.0;
        p.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
    }

    // 左轴标签 (Pack V)
    p.setFont(QFont("Consolas", 8));
    p.setPen(QColor(90, 180, 255));
    p.drawText(2, T + 10, QString::number(m_vMax, 'f', 0) + "V");
    p.drawText(2, (int)area.center().y() + 4, QString::number((m_vMax + m_vMin) / 2, 'f', 0));
    p.drawText(2, height() - B, QString::number(m_vMin, 'f', 0) + "V");

    // 右轴标签 (Current A)
    p.setPen(QColor(255, 150, 80));
    p.drawText(width() - R + 4, T + 10, QString::number(m_iMax, 'f', 1) + "A");
    p.drawText(width() - R + 4, (int)area.center().y() + 4, "0A");
    p.drawText(width() - R + 4, height() - B, QString::number(m_iMin, 'f', 1) + "A");

    if (m_pts.size() < 2) {
        p.setPen(QColor(100, 100, 120));
        p.drawText(area.center().toPoint(), "等待数据...");
        return;
    }

    const double t0 = m_pts.first().t;
    const double t1 = m_pts.last().t;
    const double span = (t1 - t0) > 1.0 ? (t1 - t0) : 1.0;

    auto xOf = [&](double t) { return area.left() + (t - t0) / span * area.width(); };
    auto yPack = [&](double v) {
        return area.bottom() - (v - m_vMin) / (m_vMax - m_vMin) * area.height();
    };
    auto yI = [&](double i) {
        return area.bottom() - (i - m_iMin) / (m_iMax - m_iMin) * area.height();
    };
    auto yCell = [&](double v) {
        return area.bottom() - (v - m_cMin) / (m_cMax - m_cMin) * area.height();
    };

    // 5 条单体电压线 (细, 半透明, 独立刻度)
    for (int c = 0; c < 5; ++c) {
        QPainterPath cp;
        cp.moveTo(xOf(m_pts[0].t), yCell(m_pts[0].cell[c]));
        for (int k = 1; k < m_pts.size(); ++k)
            cp.lineTo(xOf(m_pts[k].t), yCell(m_pts[k].cell[c]));
        p.setPen(QPen(kCellColors[c], 1.0));
        p.drawPath(cp);
    }

    // Pack 电压 (粗蓝)
    {
        QPainterPath pv;
        pv.moveTo(xOf(m_pts[0].t), yPack(m_pts[0].v));
        for (int k = 1; k < m_pts.size(); ++k)
            pv.lineTo(xOf(m_pts[k].t), yPack(m_pts[k].v));
        p.setPen(QPen(QColor(90, 180, 255), 2.0));
        p.drawPath(pv);
    }

    // 电流 (粗橙)
    {
        QPainterPath pi;
        pi.moveTo(xOf(m_pts[0].t), yI(m_pts[0].i));
        for (int k = 1; k < m_pts.size(); ++k)
            pi.lineTo(xOf(m_pts[k].t), yI(m_pts[k].i));
        p.setPen(QPen(QColor(255, 150, 80), 2.0));
        p.drawPath(pi);
    }

    // 图例
    int lx = L + 6, ly = T + 12;
    auto drawLegend = [&](const QColor &c, const QString &s) {
        p.setPen(QPen(c, 2)); p.drawLine(lx, ly - 3, lx + 16, ly - 3);
        p.setPen(c); p.drawText(lx + 20, ly, s);
        lx += p.fontMetrics().horizontalAdvance(s) + 30;
    };
    drawLegend(QColor(90, 180, 255), "Pack V");
    drawLegend(QColor(255, 150, 80), "I (A)");
    for (int c = 0; c < 5; ++c)
        drawLegend(kCellColors[c], QString("C%1").arg(c + 1));

    // 时间轴标签
    p.setPen(QColor(100, 100, 120));
    p.drawText((int)area.left(), height() - 4,
               QString::number(t0, 'f', 0) + "s");
    p.drawText((int)area.right() - 40, height() - 4,
               QString::number(t1, 'f', 0) + "s");
}
