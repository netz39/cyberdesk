#pragma once

#include "tim.h"
#include "util/led/BinaryLed.hpp"
#include "wrappers/Task.hpp"

class StatusLeds : public util::wrappers::TaskWithMemberFunctionBase
{
public:
    StatusLeds(const util::Gpio &ledRedGpio, const util::Gpio &ledGreenGpio)
        : TaskWithMemberFunctionBase("statusLedTask", 128, osPriorityLow2), //
          ledRedGpio(ledRedGpio),                                           //
          ledGreenGpio(ledGreenGpio)                                        //
    {
    }

protected:
    [[noreturn]] void taskMain(void *)
    {
        auto lastWakeTime = xTaskGetTickCount();

        while (true)
        {
            ledRedGreen.updateState(lastWakeTime);
            vTaskDelayUntil(&lastWakeTime, toOsTicks(100.0_Hz));
        }
    }

private:
    const util::Gpio &ledRedGpio;
    const util::Gpio &ledGreenGpio;

public:
    using DualLed = util::led::binary::DualLed;

    DualLed ledRedGreen{ledRedGpio, ledGreenGpio};
};