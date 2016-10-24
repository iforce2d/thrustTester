#ifndef TESTTASK_CONSTANTRPM_H
#define TESTTASK_CONSTANTRPM_H

#include "TestTask.h"
#include "PIDController.h"

class TestTask_ConstantRPM : public TestTask
{
    int m_rpm;
    static PIDController m_pid;

public:
    TestTask_ConstantRPM(int rpm, int durationMsec);

    QString description();
    QString shortDescription();
    bool hasResults() { return true; }
    void start(TestTask* previousTask, int currentThrottle);
    int step(int currentThrottle, sampleData &currentSample);
};

#endif // TESTTASK_CONSTANTRPM_H
