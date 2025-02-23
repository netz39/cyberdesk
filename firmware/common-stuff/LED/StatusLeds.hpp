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
            // ledRedGreen.updateState(lastWakeTime);
            // vTaskDelayUntil(&lastWakeTime, toOsTicks(100.0_Hz));

            ledRedGreen.setColor(util::led::binary::DualLedColor::Red);
            ledRedGreen.updateState(lastWakeTime);
            vTaskDelay(toOsTicks(0.5_s));
            ledRedGreen.setColor(util::led::binary::DualLedColor::Yellow);
            ledRedGreen.updateState(lastWakeTime);
            vTaskDelay(toOsTicks(0.5_s));
            ledRedGreen.setColor(util::led::binary::DualLedColor::Green);
            ledRedGreen.updateState(lastWakeTime);
            vTaskDelay(toOsTicks(0.5_s));
        }
    }

private:
    const util::Gpio &ledRedGpio;
    const util::Gpio &ledGreenGpio;

    using DualLed = util::led::binary::DualLed;

    DualLed ledRedGreen{ledRedGpio, ledGreenGpio};
};