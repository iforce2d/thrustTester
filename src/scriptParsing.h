#ifndef SCRIPTPARSING_H
#define SCRIPTPARSING_H

#include <QString>
#include <vector>
#include "TestTask.h"

QStringList parseScriptIntoLines(QString script);
int parseLinesIntoTasks(QStringList lines, std::vector<TestTask*>& tasks);

#endif // SCRIPTPARSING_H
