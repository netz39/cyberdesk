#pragma once

#include "tim.h"
#include "util/led/BinaryLed.hpp"
#include "wrappers/Task.hpp"

class StatusLed : public util::wrappers::TaskWithMemberFunctionBase
{
public:
    StatusLed(TIM_HandleTypeDef *const LedTimerHandle, const uint32_t &RedChannel, const uint32_t &GreenChannel)
        : TaskWithMemberFunctionBase("statusLedTask", 128, osPriorityLow2), //
          LedTimerHandle(LedTimerHandle),                                   //
          RedChannel(RedChannel),                                           //
          GreenChannel(GreenChannel)                                        //
    {
        configASSERT(this->LedTimerHandle != nullptr);
    }

protected:
    [[noreturn]] void taskMain(void *)
    {
        auto lastWakeTime = xTaskGetTickCount();

        while (true)
        {
            ledRedGreen.updateState(lastWakeTime);
            vTaskDelayUntil(&lastWakeTime, toOsTicks(50.0_Hz));
        }
    }

private:
    TIM_HandleTypeDef *const LedTimerHandle = nullptr;
    const uint32_t &RedChannel;
    const uint32_t &GreenChannel;

    static constexpr auto PwmSteps = 256;
    static constexpr auto ResolutionBits = std::bit_width<size_t>(PwmSteps - 1);
    using LedGammaCorrection = util::led::pwm::GammaCorrection<ResolutionBits>;
    static constexpr LedGammaCorrection GammaCorrection{};

public:
    using DualLed = util::led::pwm::DualLed<ResolutionBits, LedGammaCorrection>;
    DualLed ledRedGreen{util::PwmOutput<ResolutionBits>{LedTimerHandle, RedChannel},
                        util::PwmOutput<ResolutionBits>{LedTimerHandle, GreenChannel}, GammaCorrection};
};
