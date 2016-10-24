#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <vector>
#include <libserialport.h>
#include "messages.h"
#include "TestTask.h"
#include "CalibrationDialog.h"
#include "TestSetupDialog.h"
#include "PartSelectionDialog.h"

class LogMessage {
public:
    QColor color;
    QString message;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void writePositionSettings();
    void readPositionSettings();

    void keyPressEvent(QKeyEvent *event);
    void moveEvent( QMoveEvent*);
    void resizeEvent( QResizeEvent*);
    void closeEvent(QCloseEvent*);

    void updateDisplaySettings();
    bool updatePortList();
    bool openPort();
    void closePort();
    void requestMessageVersion();
    void setThrottle(unsigned short throttle);
    void setBeeping(bool t);
    void setSampling(bool t);

    void setEnableConnectionButtons(bool t);
    void setEnableConnectedButtons(bool t);
    void setEnableLabels(bool t);

    void recordSample(msg_sample &sample);

    void panic();
    void toggleSampling();

    void setOverlayProjection();

    void renderGraph();
    void setHybridProjection();
    void setGraphProjection(int dataIndex);
    void renderRawLine(int dataIndex);
    void renderMovingAverageLine(int dataIndex);

    void finishCalibration();
    void finishTestSetup();
    void finishPartsSetup();

    void setThrustCalibrationZero();
    void setThrustCalibrationOne(float knownMass);

    void setVoltageCalibrationZero();
    void setVoltageCalibrationOne(float knownVoltage);

    void setCurrentCalibrationZero();
    void setCurrentCalibrationOne(float knownCurrent);

    void updateTestList();

    void log(int level, std::string timestr, char* buffer);
    void processLogMessageQueues();

    int tasks(std::vector<TestTask*>& tasks);

private slots:
    void on_connectButton_clicked();

    void on_updatePortListButton_clicked();
    //void on_pushButton_clicked();

    void on_portListComboBox_currentIndexChanged(int index);

    void on_servoSpeedSettingSlider_valueChanged(int value);

    void on_sampleButton_clicked();

    void on_testButton_clicked();

    void on_calibrationButton_clicked();

    void on_exportResultsButton_clicked();

    void on_displaySettingsGroupBox_toggled(bool arg1);

    void on_displayRawCheckBox_toggled(bool checked);

    void on_displayThrottleCheckBox_toggled(bool checked);

    void on_displayThrustCheckBox_toggled(bool checked);

    void on_displayVoltageCheckBox_toggled(bool checked);

    void on_displayCurrentCheckBox_toggled(bool checked);

    void on_displayPowerCheckBox_toggled(bool checked);

    void on_displayEfficiencyCheckBox_toggled(bool checked);

    void on_displayRPMCheckBox_toggled(bool checked);

    void on_setupTestButton_clicked();

    void on_partsSetupButton_clicked();

private:
    Ui::MainWindow *ui;
    CalibrationDialog* m_calibrationDialog;
    TestSetupDialog* m_testSetupDialog;
    PartSelectionDialog* m_partsSetupDialog;

    QList<LogMessage> m_logMessageQueue;

    QString m_connectedPortName;
    sp_port* m_connectedPort;

    QTimer m_timer;

    bool m_disableSettingsWrites;

    int32_t m_thrustCalibrationValueZero;
    int32_t m_thrustCalibrationValueOne;
    int32_t m_thrustCalibrationValueTwo;
    float m_thrustCalibrationMeasurementOne;
    float m_thrustCalibrationMeasurementTwo;

    int32_t m_voltageCalibrationValueZero;
    int32_t m_voltageCalibrationValueOne;
    int32_t m_voltageCalibrationValueTwo;
    float m_voltageCalibrationMeasurementOne;
    float m_voltageCalibrationMeasurementTwo;

    int32_t m_currentCalibrationValueZero;
    int32_t m_currentCalibrationValueOne;
    int32_t m_currentCalibrationValueTwo;
    float m_currentCalibrationMeasurementOne;
    float m_currentCalibrationMeasurementTwo;

    int m_throttle;
    bool m_sampling;
    bool m_calibrating;

    msg_sample m_lastRawSample;
    sampleData m_lastSample;
    std::vector<sampleData> m_samples;

    sampleData m_highestSample;
    qint64 m_lastSampleTime;

    std::vector<TestTask*> m_tasks;
    int m_taskIndex;

public slots:
    void onAfterOpenPort();
    void onTimer();
    void onRecheckMessageVersion();
};

extern MainWindow* g_mainWindow;

#endif // MAINWINDOW_H
