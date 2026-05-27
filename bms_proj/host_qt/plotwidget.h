#pragma once
#include <QWidget>
#include <QVector>

// 轻量滚动曲线: 双 Y 轴 (左=电压V, 右=电流A), X=时间(秒)。
// 不依赖 QtCharts, 避免 Qt5/Qt6 命名空间差异。
class PlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

    void addSample(double t_sec, double voltage_V, double current_A);
    void clear();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    struct Pt { double t, v, i; };
    QVector<Pt> m_pts;
    int m_maxPts = 1200;        // ~20 min @1Hz

    // 固定量程 (5S: 0~22V, 电流 ±3A); 可按需调整
    double m_vMin = 0.0,  m_vMax = 22.0;
    double m_iMin = -3.0, m_iMax = 3.0;
};
