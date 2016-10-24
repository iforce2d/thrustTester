#include <QDateTime>
#include <math.h>
#include "TestTask_ConstantRPM.h"
#include "Log.h"

PIDController TestTask_ConstantRPM::m_pid;

TestTask_ConstantRPM::TestTask_ConstantRPM(int rpm, int durationMsec)
{
    m_type = TT_CONSTANT_RPM;
    m_rpm = rpm;
    m_duration = durationMsec;
    m_pid.SetTunings(0.05, 0.002, 0.2);
    m_pid.outMin = 0;
    m_pid.outMax = 1000;
}

QString TestTask_ConstantRPM::description()
{
    return QString("constant RPM test, %1 rpm").arg(m_rpm);
}

QString TestTask_ConstantRPM::shortDescription()
{
    return QString("RPM = %1 rpm, %2 sec").arg( m_rpm ).arg( m_duration / 1000.0f );
}

void TestTask_ConstantRPM::start(TestTask *previousTask, int currentThrottle)
{
    m_pid.Output = currentThrottle - 1000;
    m_pid.Initialize();

    m_pid.Setpoint = m_rpm;

    TestTask::start(previousTask, currentThrottle);
}

int TestTask_ConstantRPM::step(int currentThrottle, sampleData &currentSample)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if ( now - m_startTime > m_duration ) {
        m_finished = true;

        showReport();

        return currentThrottle;
    }

    if ( fabsf( (currentSample.rpm() / (float)m_rpm) - 1 ) < 0.02 ) {
        m_samples.push_back(currentSample);
    }

    m_pid.Input = currentSample.rpm();
    m_pid.Compute();
    float change = m_pid.Output;

    return 1000 + (int)change;
}
