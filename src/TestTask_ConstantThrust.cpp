#include <QDateTime>
#include <math.h>
#include "TestTask_ConstantThrust.h"
#include "Log.h"

PIDController TestTask_ConstantThrust::m_pid;

TestTask_ConstantThrust::TestTask_ConstantThrust(int thrust, int durationMsec)
{
    m_type = TT_CONSTANT_THRUST;
    m_thrust = thrust;
    m_duration = durationMsec;
    m_pid.SetTunings(1, 0.09, 5);
    m_pid.outMin = 0;
    m_pid.outMax = 1000;
}

QString TestTask_ConstantThrust::description()
{
    return QString("constant thrust test, thrust = %1 g").arg(m_thrust);
}

QString TestTask_ConstantThrust::shortDescription()
{
    return QString("Thrust = %1 g, %2 sec").arg( m_thrust).arg( m_duration / 1000.0f );
}

void TestTask_ConstantThrust::start(TestTask *previousTask, int currentThrottle)
{
    m_pid.Output = currentThrottle - 1000;
    m_pid.Initialize();

    m_pid.Setpoint = m_thrust;

    TestTask::start(previousTask, currentThrottle);
}

int TestTask_ConstantThrust::step(int currentThrottle, sampleData &currentSample)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if ( now - m_startTime > m_duration ) {
        m_finished = true;

        showReport();

        return currentThrottle;
    }

    if ( fabsf( (currentSample.thrust() / (float)m_thrust) - 1 ) < 0.02 ) {
        m_samples.push_back(currentSample);
    }

    m_pid.Input = currentSample.thrust();
    m_pid.Compute();
    float change = m_pid.Output;

    return 1000 + (int)change;
}
