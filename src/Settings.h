#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QColor>

extern QString g_lastSelectedPortName;
extern QString g_lastUsedSaveFileDirectory;
extern QString g_lastUsedOpenFileDirectory;

extern bool g_showDisplaySettings;
extern bool g_showRawSamples;
extern bool g_showThrottleSamples;
extern bool g_showThrustSamples;
extern bool g_showVoltageSamples;
extern bool g_showCurrentSamples;
extern bool g_showPowerSamples;
extern bool g_showEfficiencySamples;
extern bool g_showRPMSamples;

extern int32_t g_thrustCalibrationValueZero;
extern int32_t g_thrustCalibrationValueOne;
extern float g_thrustCalibrationMeasurementOne;

extern int32_t g_voltageCalibrationValueZero;
extern int32_t g_voltageCalibrationValueOne;
extern float g_voltageCalibrationMeasurementOne;

extern int32_t g_currentCalibrationValueZero;
extern int32_t g_currentCalibrationValueOne;
extern float g_currentCalibrationMeasurementOne;

extern QString g_testScriptString;

extern QString g_motorData;
extern QString g_escData;
extern QString g_propData;
extern QString g_mcuData;
extern QString g_materialData;

extern int g_selectedMotorId;
extern int g_selectedEscId;
extern int g_selectedPropId;

void readSettings();
void writeSettings();

#endif // SETTINGS_H
