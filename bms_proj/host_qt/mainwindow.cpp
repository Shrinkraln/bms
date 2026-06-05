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
#include <QSpinBox>
#include <QLabel>
#include <QFont>
#include <QFileDialog>
#include <QDateTime>
#include <QProgressBar>
#include <QApplication>

/* ===== 深色主题 QSS ===== */
static const char *kDarkQss = R"(
    QMainWindow, QWidget { background: #1a1a2e; color: #e0e0e0; }
    QGroupBox { border: 1px solid #333355; border-radius: 6px;
                margin-top: 8px; padding-top: 14px; font-weight: bold; color: #90a0b0; }
    QGroupBox::title { subcontrol-origin: margin; left: 12px; }
    QLabel { color: #c0c8d0; }
    QPushButton { background: #2a2a4a; color: #e0e0e0; border: 1px solid #444;
                  border-radius: 4px; padding: 5px 12px; font-weight: bold; }
    QPushButton:hover { background: #3a3a5a; }
    QPushButton:disabled { background: #222; color: #555; border-color: #333; }
    QComboBox, QSpinBox { background: #222244; color: #e0e0e0; border: 1px solid #444;
                          border-radius: 3px; padding: 3px 6px; }
    QProgressBar { background: #1e1e30; border: 1px solid #333; border-radius: 3px;
                   text-align: center; color: #ccc; }
    QProgressBar::chunk { border-radius: 2px; }
)";

/* 状态→颜色 */
static QString stateColor(uint8_t s)
{
    switch (s) {
    case 1: case 2: return "#1976D2";   // CC/CV 充电 蓝
    case 3: case 5: return "#616161";   // 静置 灰
    case 4:         return "#F57C00";   // 放电 橙
    case 6:         return "#388E3C";   // 完成 绿
    case 7:         return "#D32F2F";   // 错误 红
    default:        return "#9E9E9E";   // IDLE 灰白
    }
}

/* 单体条形颜色 QSS */
static QString cellBarQss(uint16_t mv)
{
    if (mv >= 4200) return "QProgressBar::chunk { background: #D32F2F; }";  // 过压红
    if (mv <= 2800) return "QProgressBar::chunk { background: #FFC107; }";  // 欠压黄
    return "QProgressBar::chunk { background: #4CAF50; }";                   // 正常绿
}

/* 数值字段 helper */
static QLabel *addField(QGridLayout *g, int row, int col, const QString &title,
                        int fontSize = 0)
{
    auto *box = new QVBoxLayout();
    auto *t = new QLabel(title);
    t->setStyleSheet("color:#7788aa; font-size:11px;");
    auto *v = new QLabel("--");
    QFont f = v->font();
    f.setPointSize(fontSize > 0 ? fontSize : f.pointSize() + 4);
    f.setBold(true);
    v->setFont(f);
    box->addWidget(t);
    box->addWidget(v);
    box->setSpacing(1);
    auto *w = new QWidget();
    w->setLayout(box);
    g->addWidget(w, row, col);
    return v;
}

/* ================================================================ */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    applyDarkTheme();
    buildUi();
    setWindowTitle("BMS-5S 化成上位机");
    resize(960, 700);

    m_uiTimer = new QTimer(this);
    connect(m_uiTimer, &QTimer::timeout, this, &MainWindow::onUiTimer);
    m_uiTimer->start(1000);
}

MainWindow::~MainWindow()
{
    if (m_dev) { m_dev->disconnectDevice(); delete m_dev; }
    if (m_logFile.isOpen()) m_logFile.close();
}

void MainWindow::applyDarkTheme()
{
    qApp->setStyleSheet(kDarkQss);
}

void MainWindow::buildUi()
{
    auto *central = new QWidget();
    auto *root = new QVBoxLayout(central);
    root->setSpacing(4);
    root->setContentsMargins(6, 6, 6, 6);

    /* ---- 顶部: 连接 ---- */
    auto *top = new QHBoxLayout();
    top->addWidget(new QLabel("PCAN:"));
    m_iface = new QComboBox();
    m_iface->addItems({"usb0", "usb1", "usb2", "usb3"});
    top->addWidget(m_iface);
    m_connectBtn = new QPushButton("连接");
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    top->addWidget(m_connectBtn);
    m_status = new QLabel("未连接");
    m_status->setStyleSheet("color:#78909C;");
    top->addWidget(m_status, 1);
    root->addLayout(top);

    /* ---- 数值面板 ---- */
    auto *gb = new QGroupBox("实时数据");
    auto *g = new QGridLayout(gb);
    g->setSpacing(2);
    m_lState = addField(g, 0, 0, "状态", 16);
    m_lPack  = addField(g, 0, 1, "总电压");
    m_lCur   = addField(g, 0, 2, "电流");
    m_lTemp  = addField(g, 0, 3, "温度");
    m_lChg   = addField(g, 1, 0, "已充 mAh");
    m_lDsg   = addField(g, 1, 1, "已放 mAh");
    m_lErr   = addField(g, 1, 2, "告警");
    m_lDelta = addField(g, 1, 3, "压差 mV");

    /* Cell 条形图 + 数值 */
    for (int i = 0; i < 5; ++i) {
        auto *cellBox = new QVBoxLayout();
        auto *nameL = new QLabel(QString("C%1").arg(i + 1));
        nameL->setStyleSheet("color:#7788aa; font-size:10px;");
        m_cellBar[i] = new QProgressBar();
        m_cellBar[i]->setRange(2500, 4250);
        m_cellBar[i]->setValue(0);
        m_cellBar[i]->setFormat("%v mV");
        m_cellBar[i]->setFixedHeight(18);
        m_cellBar[i]->setStyleSheet(cellBarQss(3700));
        m_lCell[i] = new QLabel("--");
        m_lCell[i]->setStyleSheet("font-weight:bold; font-size:12px;");
        cellBox->addWidget(nameL);
        cellBox->addWidget(m_cellBar[i]);
        cellBox->addWidget(m_lCell[i]);
        cellBox->setSpacing(1);
        auto *cw = new QWidget();
        cw->setLayout(cellBox);
        g->addWidget(cw, 2, i);
    }
    root->addWidget(gb);

    /* ---- 阶段计时 + 控制 ---- */
    auto *ctrl = new QGroupBox("控制");
    auto *cv = new QVBoxLayout(ctrl);

    /* 阶段计时行 */
    m_lPhaseTime = new QLabel("阶段: --  时间: --:--:--");
    m_lPhaseTime->setStyleSheet("color:#90CAF9; font-size:13px; font-weight:bold;");
    cv->addWidget(m_lPhaseTime);

    /* 按钮行 */
    auto *ch = new QHBoxLayout();
    m_startBtn = new QPushButton("▶ START");
    m_startBtn->setStyleSheet("background:#2E7D32; color:white; font-size:13px; padding:8px 18px;");
    m_stopBtn  = new QPushButton("■ STOP");
    m_stopBtn->setStyleSheet("background:#C62828; color:white; font-size:13px; padding:8px 18px;");
    m_clearBtn = new QPushButton("⟳ Clear");
    m_clearBtn->setStyleSheet("background:#E65100; color:white; font-size:13px; padding:8px 18px;");
    ch->addWidget(m_startBtn);
    ch->addWidget(m_stopBtn);
    ch->addWidget(m_clearBtn);
    ch->addSpacing(16);

    ch->addWidget(new QLabel("充电:"));
    m_chgISpin = new QSpinBox();
    m_chgISpin->setRange(10, 5000); m_chgISpin->setSingleStep(50); m_chgISpin->setValue(1250);
    m_chgISpin->setSuffix(" mA");
    ch->addWidget(m_chgISpin);
    m_setChgBtn = new QPushButton("Set");
    ch->addWidget(m_setChgBtn);

    ch->addWidget(new QLabel("放电:"));
    m_dsgISpin = new QSpinBox();
    m_dsgISpin->setRange(10, 5000); m_dsgISpin->setSingleStep(50); m_dsgISpin->setValue(1250);
    m_dsgISpin->setSuffix(" mA");
    ch->addWidget(m_dsgISpin);
    m_setDsgBtn = new QPushButton("Set");
    ch->addWidget(m_setDsgBtn);
    ch->addStretch(1);
    cv->addLayout(ch);

    root->addWidget(ctrl);

    connect(m_startBtn,  &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(m_stopBtn,   &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_clearBtn,  &QPushButton::clicked, this, &MainWindow::onClearErrClicked);
    connect(m_setChgBtn, &QPushButton::clicked, this, &MainWindow::onSetChgIClicked);
    connect(m_setDsgBtn, &QPushButton::clicked, this, &MainWindow::onSetDsgIClicked);

    /* ---- 曲线 ---- */
    m_plot = new PlotWidget();
    root->addWidget(m_plot, 1);

    /* ---- 底部: CSV ---- */
    auto *bot = new QHBoxLayout();
    m_logBtn = new QPushButton("📁 开始记录 CSV");
    m_logBtn->setEnabled(false);
    connect(m_logBtn, &QPushButton::clicked, this, &MainWindow::onLogClicked);
    bot->addWidget(m_logBtn);
    m_logLbl = new QLabel("");
    m_logLbl->setStyleSheet("color:#78909C;");
    bot->addWidget(m_logLbl, 1);
    root->addLayout(bot);

    setCentralWidget(central);
}

/* ---- 按钮联动: 根据状态禁用不合理操作 ---- */
void MainWindow::updateButtons()
{
    if (!m_dev) return;
    uint8_t s = m_data.state;
    // START 只在 IDLE/COMPLETE/ERROR 可点
    m_startBtn->setEnabled(s == 0 || s == 6 || s == 7);
    // STOP 只在活动态可点
    m_stopBtn->setEnabled(s >= 1 && s <= 5);
    // Clear 只在 ERROR 可点
    m_clearBtn->setEnabled(s == 7);
    // 设置电流在非活动态可点 (运行中改参数不安全)
    bool canSetI = (s == 0 || s == 6 || s == 7);
    m_setChgBtn->setEnabled(canSetI);
    m_setDsgBtn->setEnabled(canSetI);
}

void MainWindow::setConnected(bool connected)
{
    m_iface->setEnabled(!connected);
    m_connectBtn->setText(connected ? "断开" : "连接");
    m_logBtn->setEnabled(connected);
    if (!connected) {
        m_startBtn->setEnabled(false);
        m_stopBtn->setEnabled(false);
        m_clearBtn->setEnabled(false);
        m_setChgBtn->setEnabled(false);
        m_setDsgBtn->setEnabled(false);
    }
    m_status->setText(connected ? QString("已连接 %1 @500k").arg(m_iface->currentText())
                                : "未连接");
}

QString MainWindow::fmtTime(int sec)
{
    return QString("%1:%2:%3")
        .arg(sec / 3600, 2, 10, QChar('0'))
        .arg((sec / 60) % 60, 2, 10, QChar('0'))
        .arg(sec % 60, 2, 10, QChar('0'));
}

/* ===== CAN 命令 ===== */
void MainWindow::sendCmd(uint8_t opcode, uint16_t param)
{
    if (!m_dev) return;
    QByteArray d(8, '\0');
    d[0] = static_cast<char>(opcode);
    d[1] = static_cast<char>(param & 0xFF);
    d[2] = static_cast<char>((param >> 8) & 0xFF);
    QCanBusFrame f(bms::ID_CMD, d);
    f.setFrameType(QCanBusFrame::DataFrame);
    if (!m_dev->writeFrame(f))
        m_status->setText("发送失败: " + m_dev->errorString());
}

void MainWindow::onStartClicked()    { sendCmd(bms::CMD_START,     0); }
void MainWindow::onStopClicked()     { sendCmd(bms::CMD_STOP,      0); }
void MainWindow::onClearErrClicked() { sendCmd(bms::CMD_CLEAR_ERR, 0); }
void MainWindow::onSetChgIClicked()  { sendCmd(bms::CMD_SET_CHG_I, static_cast<uint16_t>(m_chgISpin->value())); }
void MainWindow::onSetDsgIClicked()  { sendCmd(bms::CMD_SET_DSG_I, static_cast<uint16_t>(m_dsgISpin->value())); }

/* ===== CAN 连接 ===== */
void MainWindow::onConnectClicked()
{
    if (m_dev) {
        if (m_logging) onLogClicked();
        m_dev->disconnectDevice();
        m_dev->deleteLater();
        m_dev = nullptr;
        setConnected(false);
        return;
    }
    if (!QCanBus::instance()->plugins().contains(QStringLiteral("peakcan"))) {
        m_status->setText("peakcan 插件不可用");
        return;
    }
    QString err;
    m_dev = QCanBus::instance()->createDevice(QStringLiteral("peakcan"),
                                              m_iface->currentText(), &err);
    if (!m_dev) { m_status->setText("创建失败: " + err); return; }
    m_dev->setConfigurationParameter(QCanBusDevice::BitRateKey, 500000);
    connect(m_dev, &QCanBusDevice::framesReceived, this, &MainWindow::onFramesReceived);
    connect(m_dev, &QCanBusDevice::errorOccurred, this, &MainWindow::onCanError);
    if (!m_dev->connectDevice()) {
        m_status->setText("连接失败: " + m_dev->errorString());
        m_dev->deleteLater(); m_dev = nullptr; return;
    }
    m_clock.restart();
    m_plot->clear();
    m_prevState = 0xFF;
    m_phaseSeconds = 0;
    setConnected(true);
}

void MainWindow::onCanError()
{
    if (m_dev) m_status->setText("CAN 错误: " + m_dev->errorString());
}

/* ===== CAN 帧接收 ===== */
void MainWindow::onFramesReceived()
{
    if (!m_dev) return;
    while (m_dev->framesAvailable()) {
        const QCanBusFrame f = m_dev->readFrame();
        if (f.frameType() != QCanBusFrame::DataFrame) continue;
        const QByteArray pl = f.payload();

        if (f.frameId() == bms::ID_ACK && pl.size() >= 4) {
            const auto *b = reinterpret_cast<const uint8_t *>(pl.constData());
            m_status->setText(QString("ACK %1 → %2")
                              .arg(bms::opName(b[0]), bms::ackName(b[1])));
            continue;
        }

        const bool isStatus = bms::applyFrame(
            m_data, f.frameId(),
            reinterpret_cast<const uint8_t *>(pl.constData()), pl.size());
        if (isStatus) {
            const double t = m_clock.elapsed() / 1000.0;
            double cv[5];
            for (int i = 0; i < 5; ++i) cv[i] = m_data.cell_mV[i] / 1000.0;
            m_plot->addSample(t, m_data.pack_mV / 1000.0,
                              m_data.current_mA / 1000.0, cv);
            if (m_logging) writeCsvRow(t);
        }
    }
    updateLabels();
    updateButtons();
}

/* ===== UI 刷新 ===== */
void MainWindow::updateLabels()
{
    if (!m_data.valid) return;

    /* 状态 (颜色编码) */
    m_lState->setText(bms::stateName(m_data.state));
    m_lState->setStyleSheet(
        QString("color:%1; font-weight:bold; font-size:16px;").arg(stateColor(m_data.state)));

    m_lPack->setText(QString("%1 V").arg(m_data.pack_mV / 1000.0, 0, 'f', 3));
    m_lCur->setText(QString("%1 mA").arg(m_data.current_mA));
    m_lTemp->setText(QString("%1 °C").arg(m_data.temp_C));
    m_lChg->setText(QString("%1 mAh").arg(m_data.charged_mAh));
    m_lDsg->setText(QString("%1 mAh").arg(m_data.discharged_mAh));

    /* 告警 (红色高亮) */
    if (m_data.error != 0) {
        m_lErr->setText(bms::errorName(m_data.error));
        m_lErr->setStyleSheet("color:#FF5252; font-weight:bold;");
    } else {
        m_lErr->setText("NONE");
        m_lErr->setStyleSheet("color:#66BB6A;");
    }

    /* 压差 */
    uint16_t cmax = 0, cmin = 0xFFFF;
    for (int i = 0; i < 5; ++i) {
        if (m_data.cell_mV[i] > cmax) cmax = m_data.cell_mV[i];
        if (m_data.cell_mV[i] < cmin) cmin = m_data.cell_mV[i];
    }
    uint16_t delta = cmax - cmin;
    m_lDelta->setText(QString::number(delta));
    m_lDelta->setStyleSheet(delta > 200 ? "color:#FF5252; font-weight:bold;"
                                        : "color:#66BB6A;");

    /* Cell 条形图 + 数值 */
    for (int i = 0; i < 5; ++i) {
        uint16_t mv = m_data.cell_mV[i];
        m_cellBar[i]->setValue(mv);
        m_cellBar[i]->setStyleSheet(cellBarQss(mv));
        m_lCell[i]->setText(QString("%1 V").arg(mv / 1000.0, 0, 'f', 3));
    }

    /* 状态变化 → 重置阶段计时 */
    if (m_data.state != m_prevState) {
        m_prevState = m_data.state;
        m_phaseSeconds = 0;
    }
}

/* 1Hz 定时器: 阶段计时 + 按钮联动 */
void MainWindow::onUiTimer()
{
    if (!m_data.valid) return;
    uint8_t s = m_data.state;
    if (s >= 1 && s <= 5) m_phaseSeconds++;  // 活动态才计时

    m_lPhaseTime->setText(
        QString("阶段: %1  |  运行 %2")
            .arg(bms::stateName(s), fmtTime(m_phaseSeconds)));

    updateButtons();
}

/* ===== CSV ===== */
void MainWindow::onLogClicked()
{
    if (m_logging) {
        m_logStream.flush(); m_logFile.close();
        m_logging = false;
        m_logBtn->setText("📁 开始记录 CSV");
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
        m_logLbl->setText("打开失败");
        return;
    }
    m_logStream.setDevice(&m_logFile);
    m_logStream << "datetime,t_s,state,pack_mV,current_mA,temp_C,"
                   "charged_mAh,discharged_mAh,cell1,cell2,cell3,cell4,cell5,error\n";
    m_logging = true;
    m_logBtn->setText("⏹ 停止记录");
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
