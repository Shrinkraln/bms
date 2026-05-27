#pragma once
#include <QMainWindow>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include "protocol.h"

class QCanBusDevice;
class QComboBox;
class QPushButton;
class QLabel;
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

private:
    void buildUi();
    void updateLabels();
    void setConnected(bool connected);
    void writeCsvRow(double t_sec);

    QComboBox   *m_iface = nullptr;
    QPushButton *m_connectBtn = nullptr;
    QPushButton *m_logBtn = nullptr;
    QLabel      *m_status = nullptr;
    QLabel      *m_logLbl = nullptr;

    QLabel *m_lState = nullptr, *m_lPack = nullptr, *m_lCur = nullptr,
           *m_lTemp = nullptr, *m_lChg = nullptr, *m_lDsg = nullptr,
           *m_lErr = nullptr, *m_lCell[5] = {nullptr,nullptr,nullptr,nullptr,nullptr};

    PlotWidget *m_plot = nullptr;

    QCanBusDevice *m_dev = nullptr;
    bms::BmsData   m_data;
    QElapsedTimer  m_clock;
    QFile          m_logFile;
    QTextStream    m_logStream;
    bool           m_logging = false;
};
