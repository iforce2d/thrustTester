#ifndef TESTTASK_BEEP_H
#define TESTTASK_BEEP_H

#include "TestTask.h"

class TestTask_Beep : public TestTask
{

public:
    TestTask_Beep(int durationMsec);

    QString description();
    QString shortDescription();
    bool hasResults() { return false; }
    void start(TestTask* previousTask, int currentThrottle);
    int step(int currentThrottle, sampleData &currentSample);
};

#endif // TESTTASK_BEEP_H
