#ifndef TESTTASK_CONSTANTTHRUST_H
#define TESTTASK_CONSTANTTHRUST_H

#include "TestTask.h"
#include "PIDController.h"

class TestTask_ConstantThrust : public TestTask
{
    int m_thrust;
    static PIDController m_pid;

public:
    TestTask_ConstantThrust(int thrust, int durationMsec);

    QString description();
    QString shortDescription();
    bool hasResults() { return true; }
    void start(TestTask* previousTask, int currentThrottle);
    int step(int currentThrottle, sampleData &currentSample);
};

#endif // TESTTASK_CONSTANTTHRUST_H
