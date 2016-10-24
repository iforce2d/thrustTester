#include <QDateTime>
#include <QUuid>
#include "TestTask.h"
#include "Log.h"
#include "Settings.h"
#include "parts.h"

TestTask::TestTask()
{
    m_finished = false;
}

TestTask::~TestTask()
{

}

_taskType TestTask::type()
{
    return m_type;
}

void TestTask::start(TestTask* /*previousTask*/, int /*currentThrottle*/)
{
    m_samples.clear();
    m_finished = false;
    m_startTime = QDateTime::currentMSecsSinceEpoch();
}

bool TestTask::finished()
{
    return m_finished;
}

void TestTask::showReport()
{
    g_log.log(LL_DEBUG, QString("Recorded %1 good samples").arg(m_samples.size()));

    if ( m_samples.size() > 10 ) {

        float totals[5];
        memset(totals, 0, sizeof(totals));
        for (int i = 0; i < (int)m_samples.size(); i++) {
            totals[0] += m_samples[i].thrust();
            totals[1] += m_samples[i].voltage();
            totals[2] += m_samples[i].current();
            totals[3] += m_samples[i].throttle();
            totals[4] += m_samples[i].rpm();
        }
        for (int i = 0; i < 5; i++)
            totals[i] /= (float)m_samples.size();

        g_log.log(LL_DEBUG, QString("Averages: %1 g, %2 V, %3 A, %4 W, %5 g/W, %6 us, %7 rpm").arg(totals[0]).arg(totals[1]).arg(totals[2]).arg(totals[1]*totals[2]).arg( totals[0]/(totals[1]*totals[2]) ).arg(1000+totals[3]).arg(totals[4]));

        testReport report;
        report.uuid = QUuid::createUuid().toString();
        report.motorId = g_selectedMotorId;
        report.escId = g_selectedEscId;
        report.propId = g_selectedPropId;
        report.numSamples = m_samples.size();
        report.thrust = totals[0];
        report.voltage = totals[1];
        report.current = totals[2];
        report.throttle = totals[3];
        report.rpm = totals[4];

        m_reports.push_back(report);
    }
}

QStringList TestTask::resultAsStringList(int i)
{
    QStringList list;

    if ( i >= (int)m_reports.size() )
        return list;

    testReport report = m_reports[i];

    list.append( QString("   UUID:  %1").arg(report.uuid));
    list.append( QString("   Motor: %1").arg(getDisplayNameForMotor(report.motorId)));
    list.append( QString("   ESC:   %1").arg(getDisplayNameForESC(report.escId)));
    list.append( QString("   Prop:  %1").arg(getDisplayNameForProp(report.propId)));
    list.append( QString("   Samples:    %1").arg(report.numSamples));
    list.append( QString("   Throttle:   %1 us").arg(1000 + report.throttle));
    list.append( QString("   Thrust:     %1 g").arg(report.thrust));
    list.append( QString("   Voltage:    %1 V").arg(report.voltage));
    list.append( QString("   Current:    %1 A").arg(report.current));
    list.append( QString("   Power:      %1 W").arg( report.voltage * report.current ));
    list.append( QString("   Efficiency: %1 g/w").arg( report.thrust/(report.voltage * report.current) ));
    list.append( QString("   RPM:        %1 rpm").arg( report.rpm ));

    return list;
}

Json::Value TestTask::resultAsJSON(int i)
{
    if ( i >= (int)m_reports.size() )
        return "";

    testReport report = m_reports[i];

    Json::Value reportValue;

    reportValue["uuid"] = report.uuid.toStdString().c_str();

    reportValue["motor"] = report.motorId;
    reportValue["motor_text"] = getDisplayNameForMotor(report.motorId).toStdString().c_str();
    reportValue["esc"] = report.escId;
    reportValue["esc_text"] = getDisplayNameForESC(report.escId).toStdString().c_str();
    reportValue["prop"] = report.propId;
    reportValue["prop_text"] = getDisplayNameForProp(report.propId).toStdString().c_str();

    reportValue["samples"] = report.numSamples;
    reportValue["throttle"] = 1000 + report.throttle;
    reportValue["thrust"] = report.thrust;
    reportValue["voltage"] = report.voltage;
    reportValue["current"] = report.current;
    reportValue["rpm"] = report.rpm;

    return reportValue;
}

void TestTask::allResultsAsJSON(Json::Value& array)
{
    for (int i = 0; i < (int)m_reports.size(); i++) {
        Json::Value reportValue = resultAsJSON(i);
        array.append(reportValue);
    }
}

QString TestTask::lastResult()
{
    if ( m_reports.empty() )
        return "";

    QStringList list = resultAsStringList(m_reports.size()-1);

    QString ret = "";
    foreach(QString s, list) {
        ret += s + "\n";
    }

    return ret;
}

void TestTask::showAllResults()
{
    if ( ! hasResults() )
        return;

    g_log.log(LL_DEBUG, QString(""));
    g_log.log(LL_DEBUG, QString("---------------------"));
    g_log.log(LL_DEBUG, QString("Results for task: %1").arg(description()));
    for (int i = 0; i < (int)m_reports.size(); i++) {

        g_log.log(LL_DEBUG, QString("----- Run %1 ---------").arg(i+1));

        /*QStringList list = resultAsStringList(i);
        foreach(QString s, list) {
            g_log.log(LL_DEBUG, s);
        }*/

        Json::Value json = resultAsJSON(i);

        Json::FastWriter writer;
        QString str = QString::fromUtf8( writer.write(json).c_str() );

        g_log.log(LL_DEBUG, str);
    }
    g_log.log(LL_DEBUG, QString("---------------------"));
    g_log.log(LL_DEBUG, QString(""));
}
