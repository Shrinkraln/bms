#pragma once
#include <QWidget>
#include <QVector>

/**
 * PlotWidget — 滚动曲线 (双 Y 轴 + 5 单体电压)
 *
 * 左 Y: Pack 电压 (V) — 粗蓝线
 *        Cell 1~5 电压 (V) — 细彩色线, 独立刻度 2.0~4.5V
 * 右 Y: 电流 (A) — 粗橙线
 * X:    时间 (秒)
 *
 * 不依赖 QtCharts。
 */
class PlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

    void addSample(double t_sec, double voltage_V, double current_A,
                   const double cellV[5]);
    void clear();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    struct Pt {
        double t, v, i;
        double cell[5];
    };
    QVector<Pt> m_pts;
    int m_maxPts = 1200;

    double m_vMin = 0.0,   m_vMax = 22.0;
    double m_iMin = -3.0,  m_iMax = 3.0;
    double m_cMin = 2.0,   m_cMax = 4.5;   // cell 电压范围
};
