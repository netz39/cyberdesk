#pragma once

#include "tim.h"
#include "units/si/temperature.hpp"
#include "util/MapValue.hpp"
#include "util/led/PwmLed.hpp"
#include "wrappers/Task.hpp"

// #include "Fading.hpp"

class LedStrip : public util::wrappers::TaskWithMemberFunctionBase
{
public:
    LedStrip(TIM_HandleTypeDef *ledTimerHandle, const uint32_t &warmWhiteChannel, const uint32_t &coldWhiteChannel)
        : TaskWithMemberFunctionBase("ledstripTask", 256, osPriorityLow2), ledTimerHandle(ledTimerHandle), //
          warmWhiteChannel(warmWhiteChannel),                                                              //
          coldWhiteChannel(coldWhiteChannel)                                                               //
    {
        SafeAssert(this->ledTimerHandle != nullptr);
    }

    void toggleState()
    {
        isEnabled = !isEnabled;

        warmWhiteLedStrip.setState(isEnabled);
        coldWhiteLedStrip.setState(isEnabled);
    }

    void incrementBrightness()
    {
        if (globalBrightness < 100)
            globalBrightness += 5;

        updateBrightness();
    }

    void decrementBrightness()
    {
        if (globalBrightness > 0)
            globalBrightness -= 5;

        updateBrightness();
    }

    void incrementColorTemperature()
    {
        if (colorTemperature < ColdColorTemperature)
            colorTemperature += ColorStep;

        mapColorTemperatureToStrip();
    }

    void decrementColorTemperature()
    {
        if (colorTemperature > WarmColorTemperature)
            colorTemperature -= ColorStep;

        mapColorTemperatureToStrip();
    }

    uint8_t getGlobalBrightness() const
    {
        return globalBrightness;
    }

    units::si::Temperature getColorTemperature() const
    {
        return colorTemperature;
    }

protected:
    [[noreturn]] void taskMain(void *)
    {
        auto lastWakeTime = xTaskGetTickCount();
        mapColorTemperatureToStrip();
        updateBrightness();

        while (true)
        {
            warmWhiteLedStrip.updateState(lastWakeTime);
            coldWhiteLedStrip.updateState(lastWakeTime);
            vTaskDelayUntil(&lastWakeTime, toOsTicks(100.0_Hz));
        }
    }

private:
    TIM_HandleTypeDef *ledTimerHandle = nullptr;
    const uint32_t &warmWhiteChannel;
    const uint32_t &coldWhiteChannel;

    static constexpr auto WarmColorTemperature = 2700.0_K;
    static constexpr auto ColdColorTemperature = 6500.0_K;
    static constexpr auto NeutralColorTemperature = 4000.0_K;
    static constexpr auto ColorStep = 100.0_K;
    units::si::Temperature colorTemperature{NeutralColorTemperature};
    size_t warmWhiteBrightness = 0;
    size_t coldWhiteBrightness = 0;

    bool isEnabled = false;
    uint8_t globalBrightness = 50;

    // APB1 for timers: 80MHz -> 1024 PWM steps and clock divison by 4 -> 19.5kHz PWM frequency
    static constexpr size_t PwmSteps = 1024;
    static constexpr auto ResolutionBits = std::bit_width<size_t>(PwmSteps - 1);
    using GammaCorrection_t = util::led::pwm::GammaCorrection<ResolutionBits, 10.0f>;
    static constexpr GammaCorrection_t GammaCorrection{};

    using SingleLed = util::led::pwm::SingleLed<ResolutionBits, GammaCorrection_t>;

    SingleLed warmWhiteLedStrip{util::PwmOutput<ResolutionBits>{ledTimerHandle, warmWhiteChannel}, GammaCorrection};
    SingleLed coldWhiteLedStrip{util::PwmOutput<ResolutionBits>{ledTimerHandle, coldWhiteChannel}, GammaCorrection};

    void mapColorTemperatureToStrip()
    {
        const auto ClampedColorTemperature = std::clamp(colorTemperature, WarmColorTemperature, ColdColorTemperature);

        if (ClampedColorTemperature < NeutralColorTemperature)
        {
            // warm white has full brightness
            warmWhiteBrightness = PwmSteps;
            const float Factor =
                ((ClampedColorTemperature - WarmColorTemperature) / (NeutralColorTemperature - WarmColorTemperature))
                    .getMagnitude();
            coldWhiteBrightness = Factor * PwmSteps;
        }
        else
        {
            // cold white has full brightness
            coldWhiteBrightness = PwmSteps;
            const float Factor = 1.0f - ((ClampedColorTemperature - NeutralColorTemperature) /
                                         (ColdColorTemperature - NeutralColorTemperature))
                                            .getMagnitude();
            warmWhiteBrightness = Factor * PwmSteps;
        }

        warmWhiteLedStrip.setTargetPwmValue(warmWhiteBrightness);
        coldWhiteLedStrip.setTargetPwmValue(coldWhiteBrightness);
    }

    void updateBrightness()
    {
        warmWhiteLedStrip.setBrightness(globalBrightness);
        coldWhiteLedStrip.setBrightness(globalBrightness);
    }
};