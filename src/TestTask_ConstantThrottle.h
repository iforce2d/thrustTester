#ifndef TESTTASK_CONSTANTTHROTTLE_H
#define TESTTASK_CONSTANTTHROTTLE_H

#include "TestTask.h"

class TestTask_ConstantThrottle : public TestTask
{
    int m_throttle;

public:
    TestTask_ConstantThrottle(int throttle, int durationMsec);

    QString description();
    QString shortDescription();
    bool hasResults() { return true; }
    int step(int currentThrottle, sampleData &currentSample);
};

#endif // TESTTASK_CONSTANTTHROTTLE_H
