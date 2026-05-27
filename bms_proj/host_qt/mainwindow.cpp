#include "mainwindow.h"
#include "plotwidget.h"

#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusFrame>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QFileDialog>
#include <QDateTime>

// 在网格里放一个 "标题 + 数值" 字段，返回数值 label。
static QLabel *addField(QGridLayout *g, int row, int col, const QString &title)
{
    auto *box = new QVBoxLayout();
    auto *t = new QLabel(title);
    t->setStyleSheet("color:#9aa;");
    auto *v = new QLabel("--");
    QFont f = v->font();
    f.setPointSize(f.pointSize() + 5);
    f.setBold(true);
    v->setFont(f);
    box->addWidget(t);
    box->addWidget(v);
    auto *w = new QWidget();
    w->setLayout(box);
    g->addWidget(w, row, col);
    return v;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    buildUi();
    setWindowTitle("BMS-5S 化成上位机 (PEAK CAN)");
    resize(820, 620);
}

MainWindow::~MainWindow()
{
    if (m_dev) {
        m_dev->disconnectDevice();
        delete m_dev;
    }
    if (m_logFile.isOpen()) m_logFile.close();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget();
    auto *root = new QVBoxLayout(central);

    // —— 顶部：连接控制 ——
    auto *top = new QHBoxLayout();
    top->addWidget(new QLabel("PCAN:"));
    m_iface = new QComboBox();
    m_iface->addItems({"usb0", "usb1", "usb2", "usb3"});
    top->addWidget(m_iface);
    m_connectBtn = new QPushButton("连接");
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    top->addWidget(m_connectBtn);
    m_status = new QLabel("未连接");
    top->addWidget(m_status, 1);
    root->addLayout(top);

    // —— 数值面板 ——
    auto *gb = new QGroupBox("实时数据");
    auto *g = new QGridLayout(gb);
    m_lState = addField(g, 0, 0, "状态");
    m_lPack  = addField(g, 0, 1, "总电压");
    m_lCur   = addField(g, 0, 2, "电流");
    m_lTemp  = addField(g, 0, 3, "温度");
    m_lChg   = addField(g, 1, 0, "已充 mAh");
    m_lDsg   = addField(g, 1, 1, "已放 mAh");
    m_lErr   = addField(g, 1, 2, "错误");
    for (int i = 0; i < 5; ++i)
        m_lCell[i] = addField(g, 2, i, QString("Cell%1").arg(i + 1));
    root->addWidget(gb);

    // —— 曲线 ——
    m_plot = new PlotWidget();
    root->addWidget(m_plot, 1);

    // —— 底部：CSV 记录 ——
    auto *bot = new QHBoxLayout();
    m_logBtn = new QPushButton("开始记录 CSV");
    m_logBtn->setEnabled(false);
    connect(m_logBtn, &QPushButton::clicked, this, &MainWindow::onLogClicked);
    bot->addWidget(m_logBtn);
    m_logLbl = new QLabel("");
    bot->addWidget(m_logLbl, 1);
    root->addLayout(bot);

    setCentralWidget(central);
}

void MainWindow::setConnected(bool connected)
{
    m_iface->setEnabled(!connected);
    m_connectBtn->setText(connected ? "断开" : "连接");
    m_logBtn->setEnabled(connected);
    m_status->setText(connected ? QString("已连接 %1 @500k").arg(m_iface->currentText())
                                : "未连接");
}

void MainWindow::onConnectClicked()
{
    if (m_dev) {                 // 断开
        if (m_logging) onLogClicked();
        m_dev->disconnectDevice();
        m_dev->deleteLater();
        m_dev = nullptr;
        setConnected(false);
        return;
    }

    if (!QCanBus::instance()->plugins().contains(QStringLiteral("peakcan"))) {
        m_status->setText("peakcan 插件不可用 — 检查 Qt SerialBus 模块是否安装");
        return;
    }

    QString err;
    m_dev = QCanBus::instance()->createDevice(QStringLiteral("peakcan"),
                                              m_iface->currentText(), &err);
    if (!m_dev) {
        m_status->setText("创建设备失败: " + err);
        return;
    }
    m_dev->setConfigurationParameter(QCanBusDevice::BitRateKey, 500000);
    connect(m_dev, &QCanBusDevice::framesReceived, this, &MainWindow::onFramesReceived);
    connect(m_dev, &QCanBusDevice::errorOccurred, this, &MainWindow::onCanError);

    if (!m_dev->connectDevice()) {
        m_status->setText("连接失败: " + m_dev->errorString());
        m_dev->deleteLater();
        m_dev = nullptr;
        return;
    }
    m_clock.restart();
    m_plot->clear();
    setConnected(true);
}

void MainWindow::onCanError()
{
    if (m_dev) m_status->setText("CAN 错误: " + m_dev->errorString());
}

void MainWindow::onFramesReceived()
{
    if (!m_dev) return;
    while (m_dev->framesAvailable()) {
        const QCanBusFrame f = m_dev->readFrame();
        if (f.frameType() != QCanBusFrame::DataFrame) continue;
        const QByteArray pl = f.payload();
        const bool isStatus = bms::applyFrame(
            m_data, f.frameId(),
            reinterpret_cast<const uint8_t *>(pl.constData()), pl.size());
        if (isStatus) {
            const double t = m_clock.elapsed() / 1000.0;
            m_plot->addSample(t, m_data.pack_mV / 1000.0, m_data.current_mA / 1000.0);
            if (m_logging) writeCsvRow(t);
        }
    }
    updateLabels();
}

void MainWindow::updateLabels()
{
    if (!m_data.valid) return;
    m_lState->setText(bms::stateName(m_data.state));
    m_lPack->setText(QString("%1 V").arg(m_data.pack_mV / 1000.0, 0, 'f', 3));
    m_lCur->setText(QString("%1 mA").arg(m_data.current_mA));
    m_lTemp->setText(QString("%1 °C").arg(m_data.temp_C));
    m_lChg->setText(QString::number(m_data.charged_mAh));
    m_lDsg->setText(QString::number(m_data.discharged_mAh));
    m_lErr->setText(bms::errorName(m_data.error));
    for (int i = 0; i < 5; ++i)
        m_lCell[i]->setText(QString("%1 V").arg(m_data.cell_mV[i] / 1000.0, 0, 'f', 3));
}

void MainWindow::onLogClicked()
{
    if (m_logging) {                       // 停止
        m_logStream.flush();
        m_logFile.close();
        m_logging = false;
        m_logBtn->setText("开始记录 CSV");
        m_logLbl->setText("已保存: " + m_logFile.fileName());
        return;
    }
    const QString fn = QFileDialog::getSaveFileName(
        this, "保存 CSV",
        QString("bms_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "CSV (*.csv)");
    if (fn.isEmpty()) return;
    m_logFile.setFileName(fn);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_logLbl->setText("打开文件失败");
        return;
    }
    m_logStream.setDevice(&m_logFile);
    m_logStream << "datetime,t_s,state,pack_mV,current_mA,temp_C,"
                   "charged_mAh,discharged_mAh,cell1,cell2,cell3,cell4,cell5,error\n";
    m_logging = true;
    m_logBtn->setText("停止记录");
    m_logLbl->setText("记录中: " + fn);
}

void MainWindow::writeCsvRow(double t_sec)
{
    m_logStream << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz") << ','
                << QString::number(t_sec, 'f', 1) << ','
                << bms::stateName(m_data.state) << ','
                << m_data.pack_mV << ',' << m_data.current_mA << ',' << m_data.temp_C << ','
                << m_data.charged_mAh << ',' << m_data.discharged_mAh << ','
                << m_data.cell_mV[0] << ',' << m_data.cell_mV[1] << ',' << m_data.cell_mV[2] << ','
                << m_data.cell_mV[3] << ',' << m_data.cell_mV[4] << ','
                << bms::errorName(m_data.error) << '\n';
}
