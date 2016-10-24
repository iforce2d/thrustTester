#ifndef TESTTASK_H
#define TESTTASK_H

#include <QString>
#include <qglobal.h> // for qint64
#include "util.h"

enum _taskType {
    TT_WAIT,
    TT_BEEP,
    TT_CONSTANT_THROTTLE,
    TT_CONSTANT_THRUST,
    TT_CONSTANT_RPM
};

struct sampleData {
    // thrust, voltage, current, power, efficiency, throttle
    float data[7];

    float throttle() { return data[0]; }
    float thrust() { return data[1]; }
    float voltage() { return data[2]; }
    float current() { return data[3]; }
    float power() { return data[4]; }
    float efficiency() { return data[5]; }
    float rpm() { return data[6]; }

    void setThrottle(float f) { data[0] = f; }
    void setThrust(float f) { data[1] = f; }
    void setVoltage(float f) { data[2] = f; }
    void setCurrent(float f) { data[3] = f; }
    void setPower(float f) { data[4] = f; }
    void setEfficiency(float f) { data[5] = f; }
    void setRPM(float f) { data[6] = f; }
};

struct testReport {

    QString uuid;

    int motorId;
    int escId;
    int propId;

    int numSamples;
    float throttle;
    float thrust;
    float voltage;
    float current;
    float rpm;
};

class TestTask
{
protected:
    _taskType m_type;
    int m_duration;
    qint64 m_startTime;
    bool m_finished;    
    std::vector<sampleData> m_samples;
    std::vector<testReport> m_reports;

public:
    TestTask();
    virtual ~TestTask();

    virtual _taskType type();
    virtual QString description() = 0;
    virtual QString shortDescription() = 0;
    virtual bool hasResults() = 0;
    virtual void start(TestTask* previousTask, int currentThrottle);
    virtual int step(int currentThrottle, sampleData& currentSample) = 0;
    virtual bool finished();
    virtual void showReport();
    virtual QStringList resultAsStringList(int i);
    virtual Json::Value resultAsJSON(int i);
    virtual void allResultsAsJSON(Json::Value &array);
    virtual QString lastResult();
    virtual void showAllResults();
    int reportCount() { return (int)m_reports.size(); }
};

#endif // TESTTASK_H
