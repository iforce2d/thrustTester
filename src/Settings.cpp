#include <QSettings>
#include <QTextCodec>
#include <QFont>
#include "Settings.h"
#include "Log.h"


QString g_lastSelectedPortName = QString::fromUtf8("");
QString g_lastUsedSaveFileDirectory = "";
QString g_lastUsedOpenFileDirectory = "";

bool g_showDisplaySettings = true;
bool g_showRawSamples = true;
bool g_showThrottleSamples = true;
bool g_showThrustSamples = true;
bool g_showVoltageSamples = true;
bool g_showCurrentSamples = true;
bool g_showPowerSamples = true;
bool g_showEfficiencySamples = true;
bool g_showRPMSamples = true;

int32_t g_thrustCalibrationValueZero = 8349000;
int32_t g_thrustCalibrationValueOne = 8390420;
float g_thrustCalibrationMeasurementOne = 100;

int32_t g_voltageCalibrationValueZero = 0;
int32_t g_voltageCalibrationValueOne = -9046;
float g_voltageCalibrationMeasurementOne = 12.45;

int32_t g_currentCalibrationValueZero = -18742;
int32_t g_currentCalibrationValueOne = -18742 + 514;
float g_currentCalibrationMeasurementOne = 1.06;

QString g_testScriptString = "b,1";

QString g_motorData = "";
QString g_escData = "";
QString g_propData = "";
QString g_mcuData = "";
QString g_materialData = "";

int g_selectedMotorId = -1;
int g_selectedEscId = -1;
int g_selectedPropId = -1;

void readFont( QSettings* qsettings, QFont* font, QString settingName )
{
    QString fontFamily = qsettings->value(QString::fromUtf8("fonts/%1.family").arg(settingName), QString::fromUtf8("Sans")).toString();
    int fontSize = qsettings->value(QString::fromUtf8("fonts/%1.size").arg(settingName), 10).toInt();
    *font = QFont( fontFamily, fontSize );
}

void writeFont( QSettings* qsettings, QFont* font, QString settingName )
{
    qsettings->setValue(QString::fromUtf8("fonts/%1.family").arg(settingName), font->family());
    qsettings->setValue(QString::fromUtf8("fonts/%1.size").arg(settingName), font->pointSize());
}

void readSettings()
{
    QSettings qsettings( QSettings::IniFormat, QSettings::UserScope, QString::fromUtf8("iforce2d"), QString::fromUtf8("tts") );
    g_lastSelectedPortName = qsettings.value(QString::fromUtf8("ui/lastSelectedPortName"), g_lastSelectedPortName).toString();

    g_lastUsedSaveFileDirectory = qsettings.value(QString::fromUtf8("file/lastusedsavefiledirectory"), g_lastUsedSaveFileDirectory).toString();
    g_lastUsedOpenFileDirectory = qsettings.value(QString::fromUtf8("file/lastusedopenfiledirectory"), g_lastUsedOpenFileDirectory).toString();

    g_showDisplaySettings = qsettings.value(QString::fromUtf8("display/showdisplaysettings"), g_showDisplaySettings).toBool();
    g_showRawSamples = qsettings.value(QString::fromUtf8("display/showrawsamples"), g_showRawSamples).toBool();
    g_showThrottleSamples = qsettings.value(QString::fromUtf8("display/showthrottlesamples"), g_showThrottleSamples).toBool();
    g_showThrustSamples = qsettings.value(QString::fromUtf8("display/showthrustsamples"), g_showThrustSamples).toBool();
    g_showVoltageSamples = qsettings.value(QString::fromUtf8("display/showvoltagesamples"), g_showVoltageSamples).toBool();
    g_showCurrentSamples = qsettings.value(QString::fromUtf8("display/showcurrentsamples"), g_showCurrentSamples).toBool();
    g_showPowerSamples = qsettings.value(QString::fromUtf8("display/showpowersamples"), g_showPowerSamples).toBool();
    g_showEfficiencySamples = qsettings.value(QString::fromUtf8("display/showefficiencysamples"), g_showEfficiencySamples).toBool();
    g_showRPMSamples = qsettings.value(QString::fromUtf8("display/showrpmsamples"), g_showRPMSamples).toBool();


    g_thrustCalibrationValueZero = qsettings.value(QString::fromUtf8("display/thrustcalibrationvaluezero"), g_thrustCalibrationValueZero).toInt();
    g_thrustCalibrationValueOne = qsettings.value(QString::fromUtf8("display/thrustcalibrationvalueone"), g_thrustCalibrationValueOne).toInt();
    g_thrustCalibrationMeasurementOne = qsettings.value(QString::fromUtf8("display/thrustcalibrationmeasurementone"), g_thrustCalibrationMeasurementOne).toFloat();

    g_voltageCalibrationValueZero = qsettings.value(QString::fromUtf8("display/voltagecalibrationvaluezero"), g_voltageCalibrationValueZero).toInt();
    g_voltageCalibrationValueOne = qsettings.value(QString::fromUtf8("display/voltagecalibrationvalueone"), g_voltageCalibrationValueOne).toInt();
    g_voltageCalibrationMeasurementOne = qsettings.value(QString::fromUtf8("display/voltagecalibrationmeasurementone"), g_voltageCalibrationMeasurementOne).toFloat();

    g_currentCalibrationValueZero = qsettings.value(QString::fromUtf8("display/currentcalibrationvaluezero"), g_currentCalibrationValueZero).toInt();
    g_currentCalibrationValueOne = qsettings.value(QString::fromUtf8("display/currentcalibrationvalueone"), g_currentCalibrationValueOne).toInt();
    g_currentCalibrationMeasurementOne = qsettings.value(QString::fromUtf8("display/currentcalibrationmeasurementone"), g_currentCalibrationMeasurementOne).toFloat();

    g_testScriptString = qsettings.value(QString::fromUtf8("tests/testscriptstring"), g_testScriptString).toString();

    g_motorData = qsettings.value(QString::fromUtf8("parts/motordata"), g_motorData).toString();
    g_escData = qsettings.value(QString::fromUtf8("parts/escdata"), g_escData).toString();
    g_propData = qsettings.value(QString::fromUtf8("parts/propdata"), g_propData).toString();
    g_mcuData = qsettings.value(QString::fromUtf8("parts/mcudata"), g_mcuData).toString();
    g_materialData = qsettings.value(QString::fromUtf8("parts/materialdata"), g_materialData).toString();

    g_selectedMotorId = qsettings.value(QString::fromUtf8("report/selectedmotorid"), g_selectedMotorId).toInt();
    g_selectedEscId =   qsettings.value(QString::fromUtf8("report/selectedescid"),   g_selectedEscId).toInt();
    g_selectedPropId =  qsettings.value(QString::fromUtf8("report/selectedpropid"),  g_selectedPropId).toInt();
}

void writeSettings()
{
    QSettings qsettings( QSettings::IniFormat, QSettings::UserScope, QString::fromUtf8("iforce2d"), QString::fromUtf8("tts") );
    qsettings.setValue(QString::fromUtf8("ui/lastSelectedPortName"), g_lastSelectedPortName);

    qsettings.setValue(QString::fromUtf8("file/lastusedsavefiledirectory"), g_lastUsedSaveFileDirectory);
    qsettings.setValue(QString::fromUtf8("file/lastusedopenfiledirectory"), g_lastUsedOpenFileDirectory);

    qsettings.setValue(QString::fromUtf8("display/showdisplaysettings"), g_showDisplaySettings);
    qsettings.setValue(QString::fromUtf8("display/showrawsamples"), g_showRawSamples);
    qsettings.setValue(QString::fromUtf8("display/showthrottlesamples"), g_showThrottleSamples);
    qsettings.setValue(QString::fromUtf8("display/showthrustsamples"), g_showThrustSamples);
    qsettings.setValue(QString::fromUtf8("display/showvoltagesamples"), g_showVoltageSamples);
    qsettings.setValue(QString::fromUtf8("display/showcurrentsamples"), g_showCurrentSamples);
    qsettings.setValue(QString::fromUtf8("display/showpowersamples"), g_showPowerSamples);
    qsettings.setValue(QString::fromUtf8("display/showefficiencysamples"), g_showEfficiencySamples);
    qsettings.setValue(QString::fromUtf8("display/showrpmsamples"), g_showRPMSamples);

    qsettings.setValue(QString::fromUtf8("display/thrustcalibrationvaluezero"), g_thrustCalibrationValueZero);
    qsettings.setValue(QString::fromUtf8("display/thrustcalibrationvalueone"), g_thrustCalibrationValueOne);
    qsettings.setValue(QString::fromUtf8("display/thrustcalibrationmeasurementone"), g_thrustCalibrationMeasurementOne);

    qsettings.setValue(QString::fromUtf8("display/voltagecalibrationvaluezero"), g_voltageCalibrationValueZero);
    qsettings.setValue(QString::fromUtf8("display/voltagecalibrationvalueone"), g_voltageCalibrationValueOne);
    qsettings.setValue(QString::fromUtf8("display/voltagecalibrationmeasurementone"), g_voltageCalibrationMeasurementOne);

    qsettings.setValue(QString::fromUtf8("display/currentcalibrationvaluezero"), g_currentCalibrationValueZero);
    qsettings.setValue(QString::fromUtf8("display/currentcalibrationvalueone"), g_currentCalibrationValueOne);
    qsettings.setValue(QString::fromUtf8("display/currentcalibrationmeasurementone"), g_currentCalibrationMeasurementOne);

    qsettings.setValue(QString::fromUtf8("tests/testscriptstring"), g_testScriptString);

    qsettings.setValue(QString::fromUtf8("parts/motordata"), g_motorData);
    qsettings.setValue(QString::fromUtf8("parts/escdata"), g_escData);
    qsettings.setValue(QString::fromUtf8("parts/propdata"), g_propData);
    qsettings.setValue(QString::fromUtf8("parts/mcudata"), g_mcuData);
    qsettings.setValue(QString::fromUtf8("parts/materialdata"), g_materialData);

    qsettings.setValue(QString::fromUtf8("report/selectedmotorid"), g_selectedMotorId);
    qsettings.setValue(QString::fromUtf8("report/selectedescid"),   g_selectedEscId);
    qsettings.setValue(QString::fromUtf8("report/selectedpropid"),  g_selectedPropId);
}






