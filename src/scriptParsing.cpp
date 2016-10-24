#include <QRegExp>
#include "scriptParsing.h"

#include "TestTask_Wait.h"
#include "TestTask_Beep.h"
#include "TestTask_ConstantThrottle.h"
#include "TestTask_ConstantThrust.h"
#include "TestTask_ConstantRPM.h"


QStringList parseScriptIntoLines(QString script)
{
    QRegExp whitespace("\\s+");

    QStringList inputLines = script.split('\n');
    QStringList outputLines;
    foreach (QString line, inputLines) {
        outputLines.append( line.trimmed().replace(whitespace,"") );
    }

    return outputLines;
}


int parseLinesIntoTasks(QStringList lines, std::vector<TestTask *>& tasks)
{
    foreach (QString line, lines) {

        if ( line.startsWith("//") )
            continue;

        bool ok = false;
        QStringList parts = line.split(',');
        if ( parts.length() < 2 )
            continue; // all tasks must have at least a type and duration

        float durationSec = parts[1].toFloat(&ok);
        if (ok && durationSec > 0) {
            int mSec = durationSec * 1000;
            if ( parts[0] == "w" || parts[0] == "wait" ) {
                tasks.push_back( new TestTask_Wait(mSec) );
            }
            else if ( parts[0] == "b" || parts[0] == "beep" ) {
                tasks.push_back( new TestTask_Beep(mSec) );
            }
            else if ( parts[0] == "s" || parts[0] == "speed" ) {
                if ( parts.length() > 2 ) {
                    int throttle = parts[2].toInt(&ok);
                    if ( ok ) {
                        tasks.push_back( new TestTask_ConstantThrottle(throttle,mSec) );
                    }
                }
            }
            else if ( parts[0] == "t" || parts[0] == "thrust" ) {
                if ( parts.length() > 2 ) {
                    float thrust = parts[2].toFloat(&ok);
                    if ( ok ) {
                        tasks.push_back( new TestTask_ConstantThrust(thrust,mSec) );
                    }
                }
            }
            else if ( parts[0] == "r" || parts[0] == "rpm" ) {
                if ( parts.length() > 2 ) {
                    float rpm = parts[2].toFloat(&ok);
                    if ( ok ) {
                        tasks.push_back( new TestTask_ConstantRPM(rpm,mSec) );
                    }
                }
            }
        }
    }

    return (int)tasks.size();
}
