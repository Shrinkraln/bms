#include "plotwidget.h"
#include <QPainter>
#include <QPainterPath>     // Qt6: <QPainter> 不再间接带入
#include <QPaintEvent>

PlotWidget::PlotWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(220);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(25, 25, 30));
    setPalette(pal);
}

void PlotWidget::addSample(double t_sec, double voltage_V, double current_A)
{
    m_pts.append({t_sec, voltage_V, current_A});
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

    const int L = 48, R = 48, T = 10, B = 22;
    QRectF area(L, T, width() - L - R, height() - T - B);
    if (area.width() < 10 || area.height() < 10) return;

    // 背景网格
    p.setPen(QColor(60, 60, 70));
    p.drawRect(area);
    for (int g = 1; g < 5; ++g) {
        double y = area.top() + area.height() * g / 5.0;
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }

    // 轴标签
    p.setPen(QColor(120, 200, 255));
    p.drawText(2, T + 10, QString::number(m_vMax, 'f', 0) + "V");
    p.drawText(2, height() - B, QString::number(m_vMin, 'f', 0) + "V");
    p.setPen(QColor(255, 170, 120));
    p.drawText(width() - R + 4, T + 10, QString::number(m_iMax, 'f', 1) + "A");
    p.drawText(width() - R + 4, height() - B, QString::number(m_iMin, 'f', 1) + "A");

    if (m_pts.size() < 2) return;

    const double t0 = m_pts.first().t;
    const double t1 = m_pts.last().t;
    const double span = (t1 - t0) > 1.0 ? (t1 - t0) : 1.0;

    auto xOf = [&](double t) { return area.left() + (t - t0) / span * area.width(); };
    auto yV  = [&](double v) {
        double f = (v - m_vMin) / (m_vMax - m_vMin);
        return area.bottom() - f * area.height();
    };
    auto yI  = [&](double i) {
        double f = (i - m_iMin) / (m_iMax - m_iMin);
        return area.bottom() - f * area.height();
    };

    // 电压 (蓝)
    QPainterPath pv;
    pv.moveTo(xOf(m_pts[0].t), yV(m_pts[0].v));
    for (int k = 1; k < m_pts.size(); ++k) pv.lineTo(xOf(m_pts[k].t), yV(m_pts[k].v));
    p.setPen(QPen(QColor(90, 180, 255), 1.6));
    p.drawPath(pv);

    // 电流 (橙)
    QPainterPath pi;
    pi.moveTo(xOf(m_pts[0].t), yI(m_pts[0].i));
    for (int k = 1; k < m_pts.size(); ++k) pi.lineTo(xOf(m_pts[k].t), yI(m_pts[k].i));
    p.setPen(QPen(QColor(255, 150, 80), 1.6));
    p.drawPath(pi);

    // 图例
    p.setPen(QColor(90, 180, 255));
    p.drawText(L + 6, T + 14, "Pack V");
    p.setPen(QColor(255, 150, 80));
    p.drawText(L + 64, T + 14, "Current A");
}
