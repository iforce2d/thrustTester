#ifndef TESTTASK_WAIT_H
#define TESTTASK_WAIT_H

#include "TestTask.h"

class TestTask_Wait : public TestTask
{

public:
    TestTask_Wait(int durationMsec);

    QString description();
    QString shortDescription();
    bool hasResults() { return false; }
    void start(TestTask* previousTask, int currentThrottle);
    int step(int currentThrottle, sampleData &currentSample);
};

#endif // TESTTASK_WAIT_H
