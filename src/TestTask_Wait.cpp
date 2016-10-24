#include <QDateTime>
#include "TestTask_Wait.h"
#include "mainwindow.h"

TestTask_Wait::TestTask_Wait(int durationMsec) : TestTask()
{
    m_type = TT_WAIT;
    m_duration = durationMsec;
}

QString TestTask_Wait::description()
{
    return QString("wait, duration = %1").arg(m_duration);
}

QString TestTask_Wait::shortDescription()
{
    return QString("Wait %2 sec").arg( m_duration / 1000.0f );
}

void TestTask_Wait::start(TestTask* previousTask, int currentThrottle)
{
    TestTask::start(previousTask, currentThrottle);
}

int TestTask_Wait::step(int /*currentThrottle*/, sampleData& /*currentSample*/)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if ( now - m_startTime > m_duration ) {
        m_finished = true;
    }

    return 1000;
}
