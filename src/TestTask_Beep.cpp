#include <QDateTime>
#include "TestTask_Beep.h"
#include "mainwindow.h"

TestTask_Beep::TestTask_Beep(int durationMsec) : TestTask()
{
    m_type = TT_BEEP;
    m_duration = durationMsec;
}

QString TestTask_Beep::description()
{
    return QString("beep, duration = %1").arg(m_duration);
}

QString TestTask_Beep::shortDescription()
{
    return QString("Beep %2 sec").arg( m_duration / 1000.0f );
}

void TestTask_Beep::start(TestTask* previousTask, int currentThrottle)
{
    g_mainWindow->setBeeping(true);

    TestTask::start(previousTask, currentThrottle);
}

int TestTask_Beep::step(int /*currentThrottle*/, sampleData& /*currentSample*/)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if ( now - m_startTime > m_duration ) {
        m_finished = true;
        g_mainWindow->setBeeping(false);
    }

    return 1000;
}
