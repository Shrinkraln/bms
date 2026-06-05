#pragma once
#include <QMainWindow>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QProgressBar>
#include <QTimer>
#include "protocol.h"

class QCanBusDevice;
class QComboBox;
class QPushButton;
class QLabel;
class QSpinBox;
class PlotWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onLogClicked();
    void onFramesReceived();
    void onCanError();
    void onStartClicked();
    void onStopClicked();
    void onClearErrClicked();
    void onSetChgIClicked();
    void onSetDsgIClicked();
    void onUiTimer();

private:
    void buildUi();
    void applyDarkTheme();
    void updateLabels();
    void updateButtons();
    void setConnected(bool connected);
    void writeCsvRow(double t_sec);
    void sendCmd(uint8_t opcode, uint16_t param);
    QString fmtTime(int sec);

    /* 顶部连接 */
    QComboBox   *m_iface = nullptr;
    QPushButton *m_connectBtn = nullptr;
    QLabel      *m_status = nullptr;

    /* 数值面板 */
    QLabel *m_lState = nullptr, *m_lPack = nullptr, *m_lCur = nullptr,
           *m_lTemp = nullptr, *m_lChg = nullptr, *m_lDsg = nullptr,
           *m_lErr = nullptr, *m_lDelta = nullptr,
           *m_lCell[5] = {};
    QProgressBar *m_cellBar[5] = {};

    /* 阶段计时 */
    QLabel *m_lPhaseTime = nullptr;

    /* 控制按钮 */
    QPushButton *m_startBtn = nullptr, *m_stopBtn = nullptr,
                *m_clearBtn = nullptr, *m_setChgBtn = nullptr,
                *m_setDsgBtn = nullptr;
    QSpinBox    *m_chgISpin = nullptr, *m_dsgISpin = nullptr;

    /* CSV */
    QPushButton *m_logBtn = nullptr;
    QLabel      *m_logLbl = nullptr;

    /* 曲线 */
    PlotWidget *m_plot = nullptr;

    /* UI 刷新定时器 (按钮联动/计时, 独立于 CAN 帧率) */
    QTimer      *m_uiTimer = nullptr;
    int          m_phaseSeconds = 0;

    /* CAN + 数据 */
    QCanBusDevice *m_dev = nullptr;
    bms::BmsData   m_data;
    QElapsedTimer  m_clock;
    QFile          m_logFile;
    QTextStream    m_logStream;
    bool           m_logging = false;
    uint8_t        m_prevState = 0xFF;
};
