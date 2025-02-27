#pragma once

#include "AddressableLedDriver.hpp"
#include "tim.h"
#include "util/led/PwmLed.hpp"
#include "wrappers/Task.hpp"

#include "animations/RainbowAnimation.hpp"
#include "animations/ShowStatusAnimation.hpp"
#include "animations/TestAllColorsAnimation.hpp"

#include <array>

/// Controls the LED bar consisting of 12 LEDs as feedback for the user.
class FeedbackLedBar : public util::wrappers::TaskWithMemberFunctionBase
{
public:
    FeedbackLedBar(SPI_HandleTypeDef *spiPeripherie)
        : TaskWithMemberFunctionBase("feedbackLedBar", 512, osPriorityLow4), //
          ledDriver(spiPeripherie) {};

    // ------------------------------------------------------------------------------------------
    void notifySpiIsFinished()
    {
        auto higherPriorityTaskWoken = pdFALSE;
        notifyFromISR(1, util::wrappers::NotifyAction::SetBits, &higherPriorityTaskWoken);
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
    // ------------------------------------------------------------------------------------------

    TestAllColorsAnimation testAllColorsAnimation{ledSegments};
    ShowStatusAnimation showStatusAnimation{ledSegments};

protected:
    // ------------------------------------------------------------------------------------------
    [[noreturn]] void taskMain(void *) override
    {
        targetAnimation = &testAllColorsAnimation;
        targetAnimation->resetAnimation();

        uint32_t lastWakeTime = xTaskGetTickCount();

        while (true)
        {
            if (targetAnimation->isAnimationFinished() && targetAnimation == &testAllColorsAnimation)
            {
                targetAnimation = &showStatusAnimation;
                break;
            }

            processLedControl(lastWakeTime);
        }

        while (true)
        {
            processLedControl(lastWakeTime);
        }
    }
    // ------------------------------------------------------------------------------------------

private:
    AddressableLedDriver ledDriver;
    LedSegmentArray ledSegments{};

    LedAnimationBase *targetAnimation{&showStatusAnimation};

    // ------------------------------------------------------------------------------------------
    void processLedControl(uint32_t &lastWakeTime)
    {
        targetAnimation->doAnimationStep();
        ledDriver.sendBuffer(ledSegments);

        vTaskDelayUntil(&lastWakeTime, toOsTicks(targetAnimation->getDelay()));
    }
};