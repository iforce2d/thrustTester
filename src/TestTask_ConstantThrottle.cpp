#include <math.h>
#include <QDateTime>
#include "TestTask_ConstantThrottle.h"
#include "Log.h"

TestTask_ConstantThrottle::TestTask_ConstantThrottle(int throttle, int durationMsec) : TestTask()
{
    m_type = TT_CONSTANT_THROTTLE;
    m_throttle = throttle;
    m_duration = durationMsec;
}

QString TestTask_ConstantThrottle::description()
{
    return QString("constant throttle test, throttle = %1").arg(m_throttle);
}

QString TestTask_ConstantThrottle::shortDescription()
{
    return QString("Throttle = %1 us, %2 sec").arg( m_throttle).arg( m_duration / 1000.0f );
}

int TestTask_ConstantThrottle::step(int currentThrottle, sampleData& currentSample)
{
    //g_log.log(LL_DEBUG, __PRETTY_FUNCTION__);

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if ( now - m_startTime > m_duration ) {
        m_finished = true;

        showReport();

        return currentThrottle;
    }

    if ( now - m_startTime > 1000 ) {
        if ( fabsf( (currentSample.throttle() / (float)(m_throttle-1000)) - 1 ) < 0.02 ) {
            m_samples.push_back(currentSample);
        }
    }

    int change = m_throttle - currentThrottle;

    change = constrain(change, -10, 10);

    return currentThrottle + change;
}
