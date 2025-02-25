#pragma once

#include "wrappers/Task.hpp"

class Fading : public util::wrappers::TaskWithMemberFunctionBase
{
public:
    Fading() : TaskWithMemberFunctionBase("fadingTask", 128, osPriorityLow2) {};

    void fadeTo(const uint) {};

private:
    int diffColdWhite = 0;
    int diffWarmWhite = 0;
    int diffBrightness = 0;
};