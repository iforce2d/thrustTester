#include <QSettings>
#include <QMessageBox>
#include <QKeyEvent>
#include <QDateTime>
#include <QMutexLocker>
#include <QThread>
#include <libserialport.h>
#include <stdio.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Log.h"
#include "messages.h"
#include "Settings.h"
#include "scriptParsing.h"
#include "parts.h"
#include "ExportDialog.h"

#include "TestTask_Wait.h"
#include "TestTask_Beep.h"
#include "TestTask_ConstantThrottle.h"
#include "TestTask_ConstantThrust.h"
#include "TestTask_ConstantRPM.h"

MainWindow* g_mainWindow = NULL;
QMutex* g_textAreaMutex;

extern QThread* g_mainThread;

color3 cols[7] = {
    color3(1,0.5,1),   // throttle
    color3(1,1,0.5),   // thrust
    color3(1,0.5,0.0), // voltage
    color3(0.5,0.5,1), // current
    color3(1,1,1),     // power
    color3(0.5,1,0.5), // efficiency
    color3(0,0.5,1)    // rpm
};

QString makeColoredStyleSheet(int dataIndex) {
    color3 col = cols[dataIndex];
    int r = col.red * 192;
    int g = col.green * 192;
    int b = col.blue * 192;
    return QString("color:rgb(%1,%2,%3);").arg(r).arg(g).arg(b);
}

QColor taskStatusColor(0, 64, 127);
QColor taskResultColor(0, 127, 0);
QColor panicColor(255, 0, 0);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    g_textAreaMutex = new QMutex(QMutex::Recursive);

    m_connectedPort = NULL;

    m_calibrationDialog = NULL;
    m_testSetupDialog = NULL;
    m_partsSetupDialog = NULL;

    statusBar()->showMessage( "Not connected" );

    setEnableLabels(false);
    setEnableConnectedButtons(false);

    m_disableSettingsWrites = false;

    m_throttle = 1000;
    m_sampling = false;
    m_calibrating = false;

    m_thrustCalibrationValueZero = g_thrustCalibrationValueZero;
    m_thrustCalibrationValueOne = g_thrustCalibrationValueOne;
    m_thrustCalibrationMeasurementOne = g_thrustCalibrationMeasurementOne;

    m_voltageCalibrationValueZero = g_voltageCalibrationValueZero;
    m_voltageCalibrationValueOne = g_voltageCalibrationValueOne;
    m_voltageCalibrationMeasurementOne = g_voltageCalibrationMeasurementOne;

    m_currentCalibrationValueZero = g_currentCalibrationValueZero;
    m_currentCalibrationValueOne = g_currentCalibrationValueOne;
    m_currentCalibrationMeasurementOne = g_currentCalibrationMeasurementOne;

    m_highestSample.setThrottle( 1000 );
    m_highestSample.setThrust( 100 );
    m_highestSample.setVoltage( 10 );
    m_highestSample.setCurrent( 0.1 );
    m_highestSample.setPower( 0.1 );
    m_highestSample.setEfficiency( 0.1 );
    m_highestSample.setRPM( 500 );

    for (int i = 0; i < 7; i++)
        m_lastSample.data[i] = 0;

    ui->currentServoSpeedLabel->hide();
    ui->loadCellReadingLabel->hide();
    ui->thrustLabel->hide();

    /*m_tasks.push_back( new TestTask_ConstantThrust(40, 2000));
    m_tasks.push_back( new TestTask_ConstantThrust(80, 2000));
    m_tasks.push_back( new TestTask_ConstantThrust(40, 2000));
    m_tasks.push_back( new TestTask_ConstantThrottle(1100, 2000));
    m_tasks.push_back( new TestTask_ConstantThrust(70, 3000));
    m_tasks.push_back( new TestTask_ConstantThrust(50, 3000));*/

    /*m_tasks.push_back( new TestTask_ConstantThrust(50, 2000));
    m_tasks.push_back( new TestTask_ConstantThrust(100, 2000));
    m_tasks.push_back( new TestTask_ConstantThrust(150, 2000));*/

    m_tasks.push_back( new TestTask_Beep(1000));
    //m_tasks.push_back( new TestTask_ConstantRPM(7000, 4000));
    //m_tasks.push_back( new TestTask_ConstantRPM(3000, 5000));
    /*m_tasks.push_back( new TestTask_ConstantRPM(4000, 2000));
    m_tasks.push_back( new TestTask_ConstantRPM(5000, 2000));
    m_tasks.push_back( new TestTask_ConstantRPM(6000, 2000));
    m_tasks.push_back( new TestTask_ConstantRPM(7000, 2000));
    m_tasks.push_back( new TestTask_ConstantThrottle(1000, 1000));
    m_tasks.push_back( new TestTask_ConstantThrottle(1084, 3000));
    m_tasks.push_back( new TestTask_ConstantThrust(100, 3000));
    m_tasks.push_back( new TestTask_ConstantThrottle(1100, 2000));
    m_tasks.push_back( new TestTask_ConstantThrust(100, 3000));*/

    for (int i = 1; i <= 10; i++)
        m_tasks.push_back( new TestTask_ConstantThrottle(1000 + i * 100, 1500));

    m_taskIndex = -1;

    ui->displayThrottleCheckBox->setStyleSheet(makeColoredStyleSheet(0)+"font-weight:bold;");
    ui->displayThrustCheckBox->setStyleSheet(makeColoredStyleSheet(1)+"font-weight:bold;");
    ui->displayVoltageCheckBox->setStyleSheet(makeColoredStyleSheet(2)+"font-weight:bold;");
    ui->displayCurrentCheckBox->setStyleSheet(makeColoredStyleSheet(3)+"font-weight:bold;");
    ui->displayPowerCheckBox->setStyleSheet(makeColoredStyleSheet(4)+"font-weight:bold;");
    ui->displayEfficiencyCheckBox->setStyleSheet(makeColoredStyleSheet(5)+"font-weight:bold;");
    ui->displayRPMCheckBox->setStyleSheet(makeColoredStyleSheet(6)+"font-weight:bold;");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::writePositionSettings()
{
    QSettings qsettings( QSettings::IniFormat, QSettings::UserScope, QString::fromUtf8("iforce2d"), QString::fromUtf8("rube") );

    qsettings.beginGroup(QString::fromUtf8("mainwindow"));

    qsettings.setValue(QString::fromUtf8("geometry"), saveGeometry());
    qsettings.setValue(QString::fromUtf8("savestate"), saveState());
    qsettings.setValue(QString::fromUtf8("maximized"), isMaximized());
    if ( !isMaximized() ) {
        qsettings.setValue(QString::fromUtf8("pos"), pos());
        qsettings.setValue(QString::fromUtf8("size"), size());
    }

    qsettings.endGroup();
}

void MainWindow::readPositionSettings()
{
    QSettings qsettings( QSettings::IniFormat, QSettings::UserScope, QString::fromUtf8("iforce2d"), QString::fromUtf8("rube") );

    qsettings.beginGroup(QString::fromUtf8("mainwindow"));

    restoreGeometry(qsettings.value(QString::fromUtf8("geometry"), saveGeometry()).toByteArray());
    restoreState(qsettings.value(QString::fromUtf8("savestate"), saveState()).toByteArray());
    move(qsettings.value(QString::fromUtf8("pos"), pos()).toPoint());
    resize(qsettings.value(QString::fromUtf8("size"), size()).toSize());
    if ( qsettings.value(QString::fromUtf8("maximized"), isMaximized()).toBool() )
        showMaximized();

    qsettings.endGroup();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Escape ) {
        if ( m_taskIndex > -1 ) {
            // TODO: needs to be passed to main thread to append
            ui->logTextBrowser->setTextColor( panicColor );
            ui->logTextBrowser->append( "Aborting test" );
        }
        panic();
    }
}

void MainWindow::moveEvent( QMoveEvent* /*event*/ )
{
    writePositionSettings();
}

void MainWindow::resizeEvent( QResizeEvent* /*event*/ )
{
    writePositionSettings();
}

void MainWindow::closeEvent(QCloseEvent* /*event*/)
{
    writePositionSettings();
}

void MainWindow::updateDisplaySettings()
{
    SetBoolTemporarily dummy(&m_disableSettingsWrites, true);

    ui->displaySettingsGroupBox->setChecked( g_showDisplaySettings );
    on_displaySettingsGroupBox_toggled( g_showDisplaySettings );

    ui->displayRawCheckBox->setChecked( g_showRawSamples );
    ui->displayThrottleCheckBox->setChecked( g_showThrottleSamples );
    ui->displayThrustCheckBox->setChecked( g_showThrustSamples );
    ui->displayVoltageCheckBox->setChecked( g_showVoltageSamples );
    ui->displayCurrentCheckBox->setChecked( g_showCurrentSamples );
    ui->displayPowerCheckBox->setChecked( g_showPowerSamples );
    ui->displayEfficiencyCheckBox->setChecked( g_showEfficiencySamples );
    ui->displayRPMCheckBox->setChecked( g_showRPMSamples );
}

bool MainWindow::updatePortList()
{
    g_log.log(LL_DEBUG, __PRETTY_FUNCTION__);

    SetBoolTemporarily dummy(&m_disableSettingsWrites, true);

    sp_port** ports;

    if ( SP_OK != sp_list_ports(&ports) ) {
        QString str("Could not sp_list_ports");
        g_log.log(LL_ERROR, str);
        ui->logTextBrowser->append( str );
        return false;
    }

    ui->portListComboBox->clear();

    int i = 0;
    for (; ports[i]; i++ ) {

        QString portName = sp_get_port_name(ports[i]);
        QString entryText = portName;

        if (sp_get_port_usb_product(ports[i])) {
            entryText += " (";
            entryText += sp_get_port_usb_product(ports[i]);
            entryText += ")";
        }

        g_log.log(LL_DEBUG, entryText);

        ui->portListComboBox->addItem(entryText,portName);
    }

    sp_free_port_list(ports);

    // now select the most recent port that had been selected
    for (int i = 0; i < ui->portListComboBox->count(); i++) {
        QString s = ui->portListComboBox->itemData( i ).toString();
        if ( s == g_lastSelectedPortName ) {
            ui->portListComboBox->setCurrentIndex(i);
            break;
        }
    }

    return i;
}

bool MainWindow::openPort()
{
    g_log.log(LL_DEBUG, __PRETTY_FUNCTION__);

    ui->servoSpeedSettingSlider->setValue(1000);

    QString portName = ui->portListComboBox->itemData( ui->portListComboBox->currentIndex() ).toString();
    g_log.log(LL_DEBUG, "Attempting connect to %s", portName.toStdString().c_str());

    sp_port* port;
    if ( SP_OK != sp_get_port_by_name(portName.toStdString().c_str(), &port) ) {
        QString str = QString("Could not find port %1").arg( portName );
        g_log.log(LL_ERROR, str);
        ui->logTextBrowser->append( str );
        return false;
    }

    /*if ( SP_OK != sp_set_baudrate(port, 9600) ) {
        g_log.log(LL_ERROR, "Could not set baud rate for port %s", portName.toStdString().c_str());
        return false;
    }*/

    /*if ( SP_OK != sp_set_dtr(port, SP_DTR_OFF) ) {
        g_log.log(LL_ERROR, "Could not disable DTR pin for port %s", portName.toStdString().c_str());
        return false;
    }*/

    if ( SP_OK != sp_open(port,SP_MODE_READ_WRITE) ) {
        QString str = QString( "Could not open port %1").arg(portName);
        g_log.log(LL_ERROR, str);
        ui->logTextBrowser->append( str );
        statusBar()->showMessage( "Not connected" );

        QMessageBox msgBox(this);
        msgBox.setWindowTitle( "Connection failed" );
        msgBox.setText( QString("Could not connect to port %1").arg(portName) );
        msgBox.addButton( "OK", QMessageBox::AcceptRole );
        msgBox.setIcon( QMessageBox::NoIcon );
        msgBox.exec();

        return false;
    }
    else {
        QString str = QString("Opened port %1").arg( portName );
        g_log.log(LL_DEBUG, str);
        ui->logTextBrowser->append( str );

        /*if ( SP_OK != sp_set_dtr(port, SP_DTR_OFF) ) {
            g_log.log(LL_ERROR, "Could not disable DTR pin for port %s", portName.toStdString().c_str());
        }*/

        if ( SP_OK != sp_set_baudrate(port, 9600) ) {
            g_log.log(LL_ERROR, "Could not set baud rate for port %s", portName.toStdString().c_str());
        }

        /*if ( SP_OK != sp_set_bits(port, 8) ) {
            g_log.log(LL_ERROR, "Could not set bits for port %s", portName.toStdString().c_str());
        }*/

        sp_port_config* config;
        sp_new_config(&config);
        sp_get_config(port, config);

        int bits = 0;
        int stopbits = 0;
        sp_parity parity;
        int baudrate = 0;
        sp_cts cts;
        sp_dsr dsr;
        sp_dtr dtr;
        sp_rts rts;
        sp_xonxoff xonxoff;
        sp_get_config_baudrate(config, &baudrate);
        sp_get_config_bits(config, &bits);
        sp_get_config_stopbits(config, &stopbits);
        sp_get_config_parity(config, &parity);
        sp_get_config_cts(config, &cts);
        sp_get_config_dsr(config, &dsr);
        sp_get_config_dtr(config, &dtr);
        sp_get_config_rts(config, &rts);
        sp_get_config_xon_xoff(config, &xonxoff);

        g_log.log(LL_DEBUG, "Proceeding with port settings:");
        g_log.log(LL_DEBUG, "    Baudrate: %d", baudrate);
        g_log.log(LL_DEBUG, "    Bits: %d", bits);
        g_log.log(LL_DEBUG, "    Stop bits: %d", stopbits);
        g_log.log(LL_DEBUG, "    Parity: %d", parity);

        g_log.log(LL_DEBUG, "    cts: %d", cts);
        g_log.log(LL_DEBUG, "    dsr: %d", dsr);
        g_log.log(LL_DEBUG, "    dtr: %d", dtr);
        g_log.log(LL_DEBUG, "    rts: %d", rts);
        g_log.log(LL_DEBUG, "    XonXoff: %d", xonxoff);

        m_connectedPort = port;
        m_connectedPortName = portName;
        ui->connectButton->setText("Close");

        setEnableConnectionButtons(false);

        m_timer.setSingleShot(true);
        m_timer.setInterval(2000);
        connect(&m_timer, SIGNAL(timeout()), this, SLOT(onAfterOpenPort()));
        m_timer.start();
    }

    return true;
}

void MainWindow::closePort()
{
    g_log.log(LL_DEBUG, __PRETTY_FUNCTION__);

    m_timer.stop();

    ui->connectButton->setText("Open");

    if ( !m_connectedPort )
        return;

    setBeeping(false);
    setSampling(false);
    setThrottle(1000);

    if ( SP_OK != sp_close(m_connectedPort) ) {
        g_log.log(LL_ERROR, "Error when closing port %s", m_connectedPortName.toStdString().c_str());
    }

    sp_free_port(m_connectedPort);

    statusBar()->showMessage( "Not connected" );
    setEnableLabels(false);
    setEnableConnectedButtons(false);

    ui->currentServoSpeedLabel->setText( "Current servo speed: -" );
    ui->loadCellReadingLabel->setText( "Load cell reading: -" );
    ui->thrustLabel->setText("Thrust: -");

    ui->logTextBrowser->setTextColor( QColor(0,0,0) );
    ui->logTextBrowser->append( QString("Closed port %1").arg(m_connectedPortName) );

    m_connectedPort = NULL;
    m_connectedPortName = "";

    ui->partsSetupButton->setEnabled(true);
    ui->setupTestButton->setEnabled(true);
}

void MainWindow::requestMessageVersion()
{
    if ( ! m_connectedPort )
        return;

    _message outgoingMessage;
    outgoingMessage.type = MT_REQUEST_VERSION;
    int outgoingPayloadSize = 1; // sizeof empty struct is 1

    unsigned char checksum[2];
    calcChecksum(checksum, &outgoingMessage, outgoingPayloadSize);

    sp_blocking_write(m_connectedPort, messageSync, 2, 100);
    sp_blocking_write(m_connectedPort, &outgoingMessage, outgoingPayloadSize, 100);
    sp_blocking_write(m_connectedPort, checksum, 2, 100);
}

void MainWindow::setThrottle(unsigned short throttle)
{
    if ( ! m_connectedPort )
        return;

    throttle = constrain(throttle, (unsigned short)1000, (unsigned short)2000);

    _message outgoingMessage;
    outgoingMessage.type = MT_SET_THROTTLE;
    int outgoingPayloadSize = sizeof(msg_setThrottle) + 1;
    outgoingMessage.setThrottle.throttle = throttle;

    unsigned char checksum[2];
    calcChecksum(checksum, &outgoingMessage, outgoingPayloadSize);

    sp_blocking_write(m_connectedPort, messageSync, 2, 100);
    sp_blocking_write(m_connectedPort, &outgoingMessage, outgoingPayloadSize, 100);
    sp_blocking_write(m_connectedPort, checksum, 2, 100);

    m_throttle = throttle;
}

void MainWindow::setBeeping(bool t)
{
    if ( ! m_connectedPort )
        return;

    _message outgoingMessage;
    outgoingMessage.type = t ? MT_START_BEEP : MT_STOP_BEEP;
    int outgoingPayloadSize = 1;

    unsigned char checksum[2];
    calcChecksum(checksum, &outgoingMessage, outgoingPayloadSize);

    sp_blocking_write(m_connectedPort, messageSync, 2, 100);
    sp_blocking_write(m_connectedPort, &outgoingMessage, outgoingPayloadSize, 100);
    sp_blocking_write(m_connectedPort, checksum, 2, 100);
}

void MainWindow::setSampling(bool t)
{
    if ( ! m_connectedPort )
        return;

    if ( m_taskIndex > -1 ) {
        // sampling is required during tests
        if ( !t )
            return;
    }

    _message outgoingMessage;
    outgoingMessage.type = t ? MT_START_SAMPLING : MT_STOP_SAMPLING;
    int outgoingPayloadSize = 1;

    unsigned char checksum[2];
    calcChecksum(checksum, &outgoingMessage, outgoingPayloadSize);

    sp_blocking_write(m_connectedPort, messageSync, 2, 100);
    sp_blocking_write(m_connectedPort, &outgoingMessage, outgoingPayloadSize, 100);
    sp_blocking_write(m_connectedPort, checksum, 2, 100);

    if ( !m_sampling ) {
        m_lastSampleTime = QDateTime::currentMSecsSinceEpoch();
    }

    m_sampling = t;
    ui->sampleButton->setText( m_sampling ? "Stop sampling" : "Start sampling" );
}

void MainWindow::setEnableConnectionButtons(bool t)
{
    ui->connectButton->setEnabled(t);
    ui->updatePortListButton->setEnabled(t);
}

void MainWindow::setEnableConnectedButtons(bool t)
{
    ui->sampleButton->setEnabled(t);
    ui->testButton->setEnabled(t);
    ui->calibrationButton->setEnabled(t);
}

void MainWindow::setEnableLabels(bool t)
{
    ui->servoSpeedSetting->setEnabled(t);
    ui->currentServoSpeedLabel->setEnabled(t);
    ui->servoSpeedSettingLabel->setEnabled(t);
    ui->loadCellReadingLabel->setEnabled(t);
    ui->thrustLabel->setEnabled(t);

    ui->servoSpeedSettingSlider->setValue(1000);
    ui->servoSpeedSettingSlider->setEnabled(t);
}

void MainWindow::recordSample(msg_sample &sample)
{
    float thrustVal = ((sample.thrust - m_thrustCalibrationValueZero) / (float)(m_thrustCalibrationValueOne - m_thrustCalibrationValueZero)) * m_thrustCalibrationMeasurementOne;

    sampleData oldSample = m_lastSample;

    m_lastSample.setThrottle( sample.throttle - 1000 );

    if ( thrustVal >= -20 && thrustVal < 5000 )
        m_lastSample.setThrust(thrustVal);
    else
        return;

    float voltageVal = ((sample.voltage - m_voltageCalibrationValueZero) / (float)(m_voltageCalibrationValueOne - m_voltageCalibrationValueZero)) * m_voltageCalibrationMeasurementOne;
    float currentVal = ((sample.current - m_currentCalibrationValueZero) / (float)(m_currentCalibrationValueOne - m_currentCalibrationValueZero)) * m_currentCalibrationMeasurementOne;

    m_lastSample.setVoltage( voltageVal );
    m_lastSample.setCurrent( currentVal );

    float watt = voltageVal * currentVal;
    float efficiency = watt > 5 ? (watt == 0 ? 0 : thrustVal / watt) : 0;

    //if ( (sample.speed - 1000) < m_lastSample.data[5] )
    //    efficiency = 0;

    m_lastSample.setPower( watt );
    m_lastSample.setEfficiency( efficiency );

    qint64 oldSampleTime = m_lastSampleTime;
    m_lastSampleTime = QDateTime::currentMSecsSinceEpoch();

    float numPropBlades = 2;
    float s = ((m_lastSampleTime - oldSampleTime) * 0.001);
    float rps = (sample.rpmcount / numPropBlades) / s;
    float rpm =  rps * 60;
    /*g_log.log(LL_DEBUG, QString("speed: %1").arg(sample.rpmcount));
    g_log.log(LL_DEBUG, QString("ms   : %1").arg(ms));
    g_log.log(LL_DEBUG, QString("rpm  : %1").arg(rpm));*/
    m_lastSample.setRPM( rpm );

    if ( m_samples.size() > 599 )
        m_samples.erase(m_samples.begin());

    for (int i = 1; i < 7; i++) { // skip this for throttle
        if ( m_lastSample.data[i] > m_highestSample.data[i] )
            m_highestSample.data[i] = m_lastSample.data[i];
    }

    m_samples.push_back( m_lastSample );

    for (int i = 1; i < 7; i++) { // skip this for throttle
        float uptake = 0.25;
        if ( i == 6 )
            uptake = 0.075; // do RPM slower
        m_lastSample.data[i] = uptake * m_lastSample.data[i] + (1-uptake) * oldSample.data[i];
    }
}

void MainWindow::panic()
{
    if ( m_calibrationDialog )
        return;

    m_taskIndex = -1;
    setBeeping(false);
    setThrottle(1000);
    ui->testButton->setText("Start test");

    bool connected = m_connectedPort != NULL;

    ui->updatePortListButton->setEnabled(true);
    ui->connectButton->setEnabled(true);
    ui->sampleButton->setEnabled(connected);
    ui->testButton->setEnabled(connected);
    ui->calibrationButton->setEnabled(connected);
    ui->partsSetupButton->setEnabled(true);
    ui->setupTestButton->setEnabled(true);
    ui->exportResultsButton->setEnabled(true);

    setEnableLabels(connected);
}

void MainWindow::toggleSampling()
{
    setSampling( !m_sampling );
}

void MainWindow::setOverlayProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, ui->liveGraphWidget->width(), 0, ui->liveGraphWidget->height(), -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

bool isLineEnabled(int dataIndex) {
    switch (dataIndex) {
    case 0: return g_showThrottleSamples; break;
    case 1: return g_showThrustSamples; break;
    case 2: return g_showVoltageSamples; break;
    case 3: return g_showCurrentSamples; break;
    case 4: return g_showPowerSamples; break;
    case 5: return g_showEfficiencySamples; break;
    case 6: return g_showRPMSamples; break;
    }
    return false;
}

void MainWindow::renderGraph()
{
    glClearColor( 0,0,0, 1 );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if ( g_showRawSamples ) {
        for (int i = 0; i < 7; i++) {
            if ( !isLineEnabled(i) )
                continue;
            setGraphProjection(i);
            setGLColor(cols[i],0.33);
            renderRawLine(i);
        }
    }

    for (int i = 0; i < 7; i++) { // skip this for throttle
        if ( !isLineEnabled(i) )
            continue;
        setGraphProjection(i);
        setGLColor(cols[i],1);
        renderMovingAverageLine(i);
    }

    glColor3f(1,1,0.5);

    setOverlayProjection();

    QString thrustStr, voltageStr, currentStr, wattStr, efficiencyStr, throttleStr, rpmStr;
    thrustStr.sprintf("Thrust: %.0f g",m_lastSample.thrust());
    voltageStr.sprintf("Voltage: %.2f V",m_lastSample.voltage());
    currentStr.sprintf("Current: %.3f A",m_lastSample.current());
    wattStr.sprintf("Power: %.1f W",m_lastSample.power());
    efficiencyStr.sprintf("Efficiency: %.1f g/W",m_lastSample.efficiency());
    throttleStr.sprintf("Throttle: %.0f us",m_lastSample.throttle() + 1000);
    rpmStr.sprintf("RPM: %.0f",m_lastSample.rpm());

    int fontSize = 20;

    QFont font = QFont();
    font.setPixelSize(fontSize);

    int row = 1;

    setGLColor(cols[0],1);
    ui->liveGraphWidget->renderText(10, ui->liveGraphWidget->height() - (row++ * fontSize), 0, throttleStr, font);
    setGLColor(cols[1],1);
    ui->liveGraphWidget->renderText(10, ui->liveGraphWidget->height() - (row++ * fontSize), 0, thrustStr, font);
    setGLColor(cols[2],1);
    ui->liveGraphWidget->renderText(10, ui->liveGraphWidget->height() - (row++ * fontSize), 0, voltageStr, font);
    setGLColor(cols[3],1);
    ui->liveGraphWidget->renderText(10, ui->liveGraphWidget->height() - (row++ * fontSize), 0, currentStr, font);
    setGLColor(cols[4],1);
    ui->liveGraphWidget->renderText(10, ui->liveGraphWidget->height() - (row++ * fontSize), 0, wattStr, font);
    setGLColor(cols[5],1);
    ui->liveGraphWidget->renderText(10, ui->liveGraphWidget->height() - (row++ * fontSize), 0, efficiencyStr, font);
    setGLColor(cols[6],1);
    ui->liveGraphWidget->renderText(10, ui->liveGraphWidget->height() - (row++ * fontSize), 0, rpmStr, font);

    if ( m_taskIndex > -1 ) {
        TestTask* task = m_tasks[m_taskIndex];
        glColor3f(1,1,1);
        ui->liveGraphWidget->renderText(10, ui->liveGraphWidget->height() - (row++ * fontSize), 0, QString("Running ") + task->description(), font);
    }

    if ( m_samples.size() > 2 && ui->liveGraphWidget->getMouseState().screenPosition.x > 0 ) {
        fontSize = 16;
        font.setPixelSize(fontSize);
        Vector2i p = ui->liveGraphWidget->getMouseState().screenPosition;

        int samplesInView = m_samples.size() - 2;
        int sampleIndex = qRound( (p.x / (float)ui->liveGraphWidget->width()) * (float)samplesInView );
        sampleIndex += 1;
        if ( sampleIndex > (int)m_samples.size()-1 )
            sampleIndex = m_samples.size()-1;

        thrustStr.sprintf("Thrust: %.0f g",m_samples[sampleIndex].thrust());
        voltageStr.sprintf("Voltage: %.2f V",m_samples[sampleIndex].voltage());
        currentStr.sprintf("Current: %.3f A",m_samples[sampleIndex].current());
        wattStr.sprintf("Power: %.1f W",m_samples[sampleIndex].power());
        efficiencyStr.sprintf("Efficiency: %.1f g/W",m_samples[sampleIndex].efficiency());
        throttleStr.sprintf("Throttle: %.0f us",m_samples[sampleIndex].throttle() + 1000);
        rpmStr.sprintf("RPM: %.0f",m_samples[sampleIndex].rpm());

        row = 0;
        setGLColor(cols[6],1);
        ui->liveGraphWidget->renderText(p.x, p.y + (row++ * fontSize), 0, rpmStr, font);
        setGLColor(cols[5],1);
        ui->liveGraphWidget->renderText(p.x, p.y + (row++ * fontSize), 0, efficiencyStr, font);
        setGLColor(cols[4],1);
        ui->liveGraphWidget->renderText(p.x, p.y + (row++ * fontSize), 0, wattStr, font);
        setGLColor(cols[3],1);
        ui->liveGraphWidget->renderText(p.x, p.y + (row++ * fontSize), 0, currentStr, font);
        setGLColor(cols[2],1);
        ui->liveGraphWidget->renderText(p.x, p.y + (row++ * fontSize), 0, voltageStr, font);
        setGLColor(cols[1],1);
        ui->liveGraphWidget->renderText(p.x, p.y + (row++ * fontSize), 0, thrustStr, font);
        setGLColor(cols[0],1);
        ui->liveGraphWidget->renderText(p.x, p.y + (row++ * fontSize), 0, throttleStr, font);

        setHybridProjection();

        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0xF0F0);

        glColor4f(1,1,1,0.5);
        glBegin(GL_LINES);
        glVertex2f(sampleIndex, 0);
        glVertex2f(sampleIndex, 1);
        glEnd();

        glDisable(GL_LINE_STIPPLE);

        for (int i = 0; i < 7; i++) {
            if ( ! isLineEnabled(i) )
                continue;
            setGraphProjection(i);
            setGLColor(cols[i],1);
            glPointSize(6);
            glBegin(GL_POINTS);
            glVertex2f(sampleIndex, m_samples[sampleIndex].data[i]);
            glEnd();
        }
    }

    ui->liveGraphWidget->update();
}

void MainWindow::setHybridProjection()
{
    // width is measured in samples, height is 0 - 1
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(1, m_samples.size()-1, 0, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void MainWindow::setGraphProjection(int dataIndex)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(1, m_samples.size()-1, 0, m_highestSample.data[dataIndex] * 1.1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void MainWindow::renderRawLine(int dataIndex)
{
    glBegin(GL_LINES);

    for (int i = 1; i < (int)m_samples.size(); i++) {
        glVertex2f(i-1, m_samples[i-1].data[dataIndex]);
        glVertex2f(i, m_samples[i].data[dataIndex]);
    }
    glEnd();
}

void MainWindow::renderMovingAverageLine(int dataIndex)
{
    glBegin(GL_LINES);
    //glVertex2f(0,0);

#define MA_COUNT 30

    // One reading seems to be about 33ms, so a spread of five readings is around 150ms
    int movingAverageSpread[7] = {
        1,  // throttle
        5,  // thrust
        5,  // voltage
        5,  // current
        5,  // power
        5,  // efficiency
        30  // rpm
    };

    int maxMaCount = movingAverageSpread[dataIndex];

    float ma[MA_COUNT];
    memset(ma, 0, MA_COUNT * sizeof(float));
    int maInd = 0;
    float maTotal = 0;

    float lastMA = 0;
    int offset = -maxMaCount / 2;

    for (int i = 0; i < (int)m_samples.size(); i++) {
        float f = m_samples[i].data[dataIndex];
        maTotal -= ma[maInd];
        maTotal += f;
        ma[maInd] = f;
        maInd = (maInd + 1) % maxMaCount;

        if ( i > 0 ) {
            glVertex2f(i-1 + offset, lastMA);
            glVertex2f(i + offset, maTotal / (float)maxMaCount);
        }

        lastMA = maTotal / (float)maxMaCount;
    }

    //glVertex2f(m_samples.size(),0);
    glEnd();
}

void MainWindow::on_connectButton_clicked()
{
    if ( m_connectedPort )
        closePort();
    else {
        statusBar()->showMessage( "Connecting..." );
        statusBar()->repaint();
        openPort();
    }
}

void MainWindow::on_updatePortListButton_clicked()
{
    updatePortList();
}

void MainWindow::onTimer()
{
    if ( ! m_connectedPort )
        return;

    unsigned char buf[8];

    bool gotSample = false;

    while ( sp_input_waiting(m_connectedPort) ) {
        int read = sp_blocking_read_next(m_connectedPort, buf, 8, 2);
        for (int i = 0; i < read; i++) {            
            int messageType = processSerialByte(buf[i]);
            if ( MT_NONE != messageType ) {
                //g_log.log(LL_DEBUG, "Got message %d", messageType);
                if ( messageType == MT_VERSION ) {
                    g_log.log(LL_DEBUG, "Version: %d", g_message.version.version);
                }
                else if ( messageType == MT_SAMPLE ) {
                    /*g_log.log(LL_DEBUG, "Speed: %d", g_message.sample.speed);
                    g_log.log(LL_DEBUG, "Thrust: %ld", g_message.sample.thrust);
                    g_log.log(LL_DEBUG, "Voltage: %ld", g_message.sample.voltage);
                    g_log.log(LL_DEBUG, "Current: %ld", g_message.sample.current);*/

                    gotSample = true;
                }
            }
        }
    }

    if ( m_connectedPort && gotSample ) {
        ui->currentServoSpeedLabel->setText( QString("Current servo throttle: %1").arg(g_message.sample.throttle) );
        ui->loadCellReadingLabel->setText( QString("Load cell reading: %1").arg(g_message.sample.thrust) );

        m_lastRawSample = g_message.sample;

        recordSample(g_message.sample);

        QString s;
        s.sprintf("Thrust: %.0f g",m_lastSample.thrust());
        ui->thrustLabel->setText( s );
    }

    if ( m_taskIndex > -1 ) {
        TestTask* task = m_tasks[m_taskIndex];
        if ( task->finished() ) {

            QString str = QString("Finished ") + task->description();
            g_log.log(LL_INFO, str);
            ui->logTextBrowser->setTextColor( taskStatusColor );
            ui->logTextBrowser->append( str );

            // results
            if ( task->hasResults() ) {
                ui->logTextBrowser->setTextColor( taskResultColor );
                QStringList lines = task->lastResult().split('\n');
                foreach (QString str, lines) {
                    ui->logTextBrowser->append( "   " + str );
                }
            }

            m_taskIndex++;

            if ( m_taskIndex >= (int)m_tasks.size() ) {
                panic();

                ui->sampleButton->setEnabled(true);
                ui->servoSpeedSettingSlider->setEnabled(true);
            }
            else {
                TestTask* previousTask = task;
                task = m_tasks[m_taskIndex];
                ui->logTextBrowser->setTextColor( taskStatusColor );
                QString str = QString("Starting ") + task->description();
                g_log.log(LL_INFO, str);
                ui->logTextBrowser->append( "" );
                ui->logTextBrowser->append( str );
                task->start(previousTask, m_throttle);
            }
        }
        else {
            int speed = task->step(m_throttle, m_lastSample);
            setThrottle(speed);
        }
    }

    renderGraph();
}


void MainWindow::onAfterOpenPort()
{
    //g_log.log(LL_DEBUG, __PRETTY_FUNCTION__);

    m_timer.stop();
    disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(onAfterOpenPort()));

    setEnableConnectionButtons(true);

    if ( ! m_connectedPort )
        return;

    // make sure arduino is not spewing out messages from a previous connection
    setSampling(false);
    while ( sp_input_waiting(m_connectedPort) ) {
        unsigned char buf[128];
        sp_blocking_read(m_connectedPort, buf, 128, 2);
    }

    // all should be silent on the connection now

    m_timer.setSingleShot(false);
    m_timer.setInterval(16);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    m_timer.start();

    requestMessageVersion();
    setThrottle(1000);
    setSampling(true);

    QString portName = ui->portListComboBox->itemData( ui->portListComboBox->currentIndex() ).toString();
    statusBar()->showMessage( "Connected to " + portName );
    statusBar()->repaint();

    setEnableLabels(true);
    setEnableConnectedButtons(true);

    /*m_timer.setSingleShot(false);
    m_timer.setInterval(100);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(onRecheckMessageVersion()));
    m_timer.start();*/
}

void MainWindow::onRecheckMessageVersion()
{
    requestMessageVersion();
}

/*void MainWindow::on_pushButton_clicked()
{
    requestMessageVersion();
}*/

void MainWindow::log(int level, std::string timestr, char* buffer)
{
    if ( level == LL_DEBUG )
        return;

    if ( ! g_mainThread )
        return; // app is closing down, logTextBrowser is already destroyed

    QColor color(0,0,0);
    switch ( level ) {
    case LL_SYSTEM_INFO :   color = QColor(127,   0,    127); break;
    case LL_SYSTEM_OUTPUT : color = QColor( 64, 127,    192); break;
    case LL_SCRIPT :        color = QColor(  0,   0,    192); break;
    case LL_DEBUG :         color = QColor(192, 192,    192); break;
    case LL_INFO :          color = QColor(0,    92,      0); break;
    case LL_WARNING :       color = QColor(255, 128,      0); break;
    case LL_ERROR :         color = QColor(255,  32,      0); break;
    case LL_FATAL :         color = QColor(255,   0,      0); break;
    }

    QMutexLocker lock(g_textAreaMutex);
    if ( QThread::currentThread() == g_mainThread ) {
        ui->logTextBrowser->setTextColor( color );
        QString thisMsg = QString::fromUtf8(timestr.c_str()) + QString::fromUtf8(": ");
        thisMsg += QString::fromUtf8( buffer );
        ui->logTextBrowser->append( thisMsg );
    }
    else {
        LogMessage lm;
        lm.color = color;
        lm.message = QString::fromUtf8(timestr.c_str()) + QString::fromUtf8(": ");
        lm.message += QString::fromUtf8( buffer );
        m_logMessageQueue.append(lm);
    }
}

void MainWindow::processLogMessageQueues()
{
    foreach(LogMessage lm, m_logMessageQueue) {
        ui->logTextBrowser->setTextColor( lm.color );
        ui->logTextBrowser->append( lm.message );
    }
    m_logMessageQueue.clear();
}

int MainWindow::tasks(vector<TestTask*>& tasks)
{
    for (int i = 0; i < (int)m_tasks.size(); i++) {
        TestTask* task = m_tasks[i];
        tasks.push_back(task);
    }
    return (int)tasks.size();
}

void MainWindow::on_portListComboBox_currentIndexChanged(int /*index*/)
{
    if ( m_disableSettingsWrites )
        return;

    g_lastSelectedPortName = ui->portListComboBox->itemData( ui->portListComboBox->currentIndex() ).toString();
    writeSettings();
}

void MainWindow::on_servoSpeedSettingSlider_valueChanged(int value)
{
    setThrottle(value);
    ui->servoSpeedSetting->setText( QString("%1 us").arg(value) );
}

void MainWindow::on_sampleButton_clicked()
{
    setSampling( !m_sampling );
}

void MainWindow::on_testButton_clicked()
{
    if ( m_taskIndex > -1 ) {
        panic();
        ui->sampleButton->setEnabled(true);
        ui->calibrationButton->setEnabled(true);
        ui->exportResultsButton->setEnabled(true);
        ui->partsSetupButton->setEnabled(true);
        ui->setupTestButton->setEnabled(true);
        ui->connectButton->setEnabled(true);
        return;
    }

    QMessageBox confirmDialog(this);
    confirmDialog.setWindowTitle("Run test");
    confirmDialog.setText( QString("Test parts will be recorded as:\n\nMotor: %1\nESC:   %2\nProp:  %3")
                           .arg(getDisplayNameForMotor(g_selectedMotorId))
                           .arg(getDisplayNameForESC(g_selectedEscId))
                           .arg(getDisplayNameForProp(g_selectedPropId))
                           );
    confirmDialog.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    confirmDialog.setDefaultButton(QMessageBox::Cancel);
    int ret = confirmDialog.exec();
    if ( ret != QMessageBox::Ok )
        return;

    setSampling(true);
    ui->sampleButton->setEnabled(false);
    ui->calibrationButton->setEnabled(false);
    ui->exportResultsButton->setEnabled(false);
    ui->partsSetupButton->setEnabled(false);
    ui->setupTestButton->setEnabled(false);
    ui->connectButton->setEnabled(false);
    setEnableLabels(false);

    m_taskIndex = 0;
    TestTask* task = m_tasks[m_taskIndex];
    QString str = QString("Starting ") + task->description();
    g_log.log(LL_INFO, str);
    ui->logTextBrowser->setTextColor( taskStatusColor );
    ui->logTextBrowser->append( "" );
    ui->logTextBrowser->append( str );
    task->start(NULL, m_throttle);

    ui->testButton->setText("Stop test");
}

void MainWindow::on_calibrationButton_clicked()
{
    if ( m_calibrationDialog )
        return;

    panic();
    setSampling(true);

    ui->updatePortListButton->setEnabled(false);
    ui->connectButton->setEnabled(false);
    ui->sampleButton->setEnabled(false);
    ui->testButton->setEnabled(false);
    ui->calibrationButton->setEnabled(false);
    //ui->servoSpeedSetting->setEnabled(false);
    //ui->servoSpeedSettingLabel->setEnabled(false);
    //ui->servoSpeedSettingSlider->setEnabled(false);
    ui->exportResultsButton->setEnabled(false);
    ui->setupTestButton->setEnabled(false);
    ui->partsSetupButton->setEnabled(false);

    m_calibrationDialog = new CalibrationDialog(this);
    m_calibrationDialog->show();
}

void MainWindow::finishCalibration()
{
    g_thrustCalibrationValueZero = m_thrustCalibrationValueZero;
    g_thrustCalibrationValueOne = m_thrustCalibrationValueOne;
    g_thrustCalibrationMeasurementOne = m_thrustCalibrationMeasurementOne;

    g_voltageCalibrationValueZero = m_voltageCalibrationValueZero;
    g_voltageCalibrationValueOne = m_voltageCalibrationValueOne;
    g_voltageCalibrationMeasurementOne = m_voltageCalibrationMeasurementOne;

    g_currentCalibrationValueZero = m_currentCalibrationValueZero;
    g_currentCalibrationValueOne = m_currentCalibrationValueOne;
    g_currentCalibrationMeasurementOne = m_currentCalibrationMeasurementOne;

    writeSettings();

    m_calibrationDialog->hide();
    m_calibrationDialog->deleteLater();
    m_calibrationDialog = NULL;

    ui->updatePortListButton->setEnabled(true);
    ui->connectButton->setEnabled(true);
    ui->sampleButton->setEnabled(true);
    ui->testButton->setEnabled(true);
    ui->calibrationButton->setEnabled(true);
    //ui->servoSpeedSetting->setEnabled(true);
    //ui->servoSpeedSettingLabel->setEnabled(true);
    //ui->servoSpeedSettingSlider->setEnabled(true);
    ui->exportResultsButton->setEnabled(true);
    ui->setupTestButton->setEnabled(true);
    ui->partsSetupButton->setEnabled(true);
}

void MainWindow::setThrustCalibrationZero()
{
    m_thrustCalibrationValueZero = m_lastRawSample.thrust;
}

void MainWindow::setThrustCalibrationOne(float knownMass)
{
    m_thrustCalibrationMeasurementOne = knownMass;
    m_thrustCalibrationValueOne = m_lastRawSample.thrust;
}

void MainWindow::setVoltageCalibrationZero()
{
    m_voltageCalibrationValueZero = m_lastRawSample.voltage;
}

void MainWindow::setVoltageCalibrationOne(float knownVoltage)
{
    m_voltageCalibrationMeasurementOne = knownVoltage;
    m_voltageCalibrationValueOne = m_lastRawSample.voltage;
}

void MainWindow::setCurrentCalibrationZero()
{
    m_currentCalibrationValueZero = m_lastRawSample.current;
}

void MainWindow::setCurrentCalibrationOne(float knownCurrent)
{
    m_currentCalibrationMeasurementOne = knownCurrent;
    m_currentCalibrationValueOne = m_lastRawSample.current;
}

void MainWindow::updateTestList()
{
    if ( m_taskIndex > -1 )
        return;

    QStringList lines = parseScriptIntoLines( g_testScriptString );

    for (int i = 0; i < (int)m_tasks.size(); i++) {
        delete m_tasks[i];
    }
    m_tasks.clear();

    parseLinesIntoTasks(lines, m_tasks);
}

void MainWindow::on_exportResultsButton_clicked()
{
    if ( m_taskIndex > -1 )
        return;

    for (int i = 0; i < (int)m_tasks.size(); i++) {
        TestTask* task = m_tasks[i];
        task->showAllResults();
    }

    ExportDialog dialog(this);
    dialog.setModal(true);
    dialog.exec();
}

void MainWindow::on_displaySettingsGroupBox_toggled(bool b)
{
    ui->displayRawCheckBox->setShown(b);
    ui->displayThrottleCheckBox->setShown(b);
    ui->displayThrustCheckBox->setShown(b);
    ui->displayVoltageCheckBox->setShown(b);
    ui->displayCurrentCheckBox->setShown(b);
    ui->displayPowerCheckBox->setShown(b);
    ui->displayEfficiencyCheckBox->setShown(b);
    ui->displayRPMCheckBox->setShown(b);

    g_showDisplaySettings = b;

    if ( ! m_disableSettingsWrites )
        writeSettings();
}

void MainWindow::on_displayRawCheckBox_toggled(bool checked)
{
    g_showRawSamples = checked;

    if ( ! m_disableSettingsWrites )
        writeSettings();
}

void MainWindow::on_displayThrottleCheckBox_toggled(bool checked)
{
    g_showThrottleSamples = checked;

    if ( ! m_disableSettingsWrites )
        writeSettings();

}

void MainWindow::on_displayThrustCheckBox_toggled(bool checked)
{
    g_showThrustSamples = checked;

    if ( ! m_disableSettingsWrites )
        writeSettings();

}

void MainWindow::on_displayVoltageCheckBox_toggled(bool checked)
{
    g_showVoltageSamples = checked;

    if ( ! m_disableSettingsWrites )
        writeSettings();

}

void MainWindow::on_displayCurrentCheckBox_toggled(bool checked)
{
    g_showCurrentSamples = checked;

    if ( ! m_disableSettingsWrites )
        writeSettings();

}

void MainWindow::on_displayPowerCheckBox_toggled(bool checked)
{
    g_showPowerSamples = checked;

    if ( ! m_disableSettingsWrites )
        writeSettings();

}

void MainWindow::on_displayEfficiencyCheckBox_toggled(bool checked)
{
    g_showEfficiencySamples = checked;

    if ( ! m_disableSettingsWrites )
        writeSettings();

}

void MainWindow::on_displayRPMCheckBox_toggled(bool checked)
{
    g_showRPMSamples = checked;

    if ( ! m_disableSettingsWrites )
        writeSettings();

}

void MainWindow::on_setupTestButton_clicked()
{
    if ( m_testSetupDialog )
        return;

    panic();

    ui->updatePortListButton->setEnabled(false);
    ui->connectButton->setEnabled(false);
    ui->sampleButton->setEnabled(false);
    ui->testButton->setEnabled(false);
    ui->calibrationButton->setEnabled(false);
    ui->partsSetupButton->setEnabled(false);
    ui->setupTestButton->setEnabled(false);

    m_testSetupDialog = new TestSetupDialog(this);
    m_testSetupDialog->exec();

    g_testScriptString = m_testSetupDialog->getScriptString();
    writeSettings();

    updateTestList();

    finishTestSetup();
}

void MainWindow::finishTestSetup()
{
    m_testSetupDialog->hide();
    m_testSetupDialog->deleteLater();
    m_testSetupDialog = NULL;

    bool connected = m_connectedPort != NULL;

    ui->updatePortListButton->setEnabled(true);
    ui->connectButton->setEnabled(true);
    ui->sampleButton->setEnabled(connected);
    ui->testButton->setEnabled(connected);
    ui->calibrationButton->setEnabled(connected);
    ui->partsSetupButton->setEnabled(true);
    ui->setupTestButton->setEnabled(true);

    //ui->servoSpeedSetting->setEnabled(true);
    //ui->servoSpeedSettingLabel->setEnabled(true);
    //ui->servoSpeedSettingSlider->setEnabled(true);

}

void MainWindow::on_partsSetupButton_clicked()
{
    if ( m_partsSetupDialog )
        return;

    panic();

    ui->updatePortListButton->setEnabled(false);
    ui->connectButton->setEnabled(false);
    ui->sampleButton->setEnabled(false);
    ui->testButton->setEnabled(false);
    ui->calibrationButton->setEnabled(false);
    ui->partsSetupButton->setEnabled(false);
    ui->setupTestButton->setEnabled(false);

    m_partsSetupDialog = new PartSelectionDialog(this);
    m_partsSetupDialog->exec();

    //g_testScriptString = m_partsSetupDialog->getScriptString();

    writeSettings();

    finishPartsSetup();
}

void MainWindow::finishPartsSetup()
{
    m_partsSetupDialog->hide();
    m_partsSetupDialog->deleteLater();
    m_partsSetupDialog = NULL;

    bool connected = m_connectedPort != NULL;

    ui->updatePortListButton->setEnabled(true);
    ui->connectButton->setEnabled(true);
    ui->sampleButton->setEnabled(connected);
    ui->testButton->setEnabled(connected);
    ui->calibrationButton->setEnabled(connected);
    ui->partsSetupButton->setEnabled(true);
    ui->setupTestButton->setEnabled(true);
}















































