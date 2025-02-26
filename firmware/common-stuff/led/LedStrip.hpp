#pragma once

#include "tim.h"
#include "units/si/temperature.hpp"
#include "util/MapValue.hpp"
#include "util/led/PwmLed.hpp"
#include "wrappers/Task.hpp"

class LedStrip : public util::wrappers::TaskWithMemberFunctionBase
{
public:
    LedStrip(TIM_HandleTypeDef *ledTimerHandle, const uint32_t &warmWhiteChannel, const uint32_t &coldWhiteChannel)
        : TaskWithMemberFunctionBase("ledstripTask", 256, osPriorityLow2), ledTimerHandle(ledTimerHandle), //
          warmWhiteChannel(warmWhiteChannel),                                                              //
          coldWhiteChannel(coldWhiteChannel)                                                               //
    {
        configASSERT(this->ledTimerHandle != nullptr);
    }

    void setGlobalBrightness(uint8_t newBrightness, bool fading = true)
    {
        globalBrightness = newBrightness;
        fading ? updateWithFade() : updateImmediately();
    }

    void setColorTemperature(units::si::Temperature newColorTemperature, bool fading = true)
    {
        colorTemperature = newColorTemperature;
        fading ? updateWithFade() : updateImmediately();
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

        while (true)
        {
            warmWhiteLedStrip.updateState(lastWakeTime);
            coldWhiteLedStrip.updateState(lastWakeTime);

            bool areStripsFading = warmWhiteLedStrip.isFading() || coldWhiteLedStrip.isFading();
            constexpr auto MinimumDelayTime = 1.0_ms / (static_cast<float>(configTICK_RATE_HZ) / 1000);
            vTaskDelayUntil(&lastWakeTime, toOsTicks(areStripsFading ? MinimumDelayTime : 20.0_ms));
        }
    }

private:
    TIM_HandleTypeDef *ledTimerHandle = nullptr;
    const uint32_t &warmWhiteChannel;
    const uint32_t &coldWhiteChannel;

    static constexpr auto WarmColorTemperature = 2700.0_K;
    static constexpr auto ColdColorTemperature = 6000.0_K;
    static constexpr auto NeutralColorTemperature = 4200.0_K; // color mixed by warm and cold white with equal intensity
    static constexpr auto ColorStep = 100.0_K;
    units::si::Temperature colorTemperature{NeutralColorTemperature};

    uint8_t globalBrightness = 50;

    // APB for timers: 64MHz -> 1024 PWM steps and prescaler (2+1) -> 20.83kHz PWM frequency
    static constexpr size_t PwmSteps = 1024;
    static constexpr auto ResolutionBits = std::bit_width<size_t>(PwmSteps - 1);
    using GammaCorrection_t = util::led::pwm::GammaCorrection<ResolutionBits, 10.0f>;
    static constexpr GammaCorrection_t GammaCorrection{};

    using SingleLed = util::led::pwm::SingleLed<ResolutionBits, GammaCorrection_t>;

    SingleLed warmWhiteLedStrip{util::PwmOutput<ResolutionBits>{ledTimerHandle, warmWhiteChannel}, GammaCorrection};
    SingleLed coldWhiteLedStrip{util::PwmOutput<ResolutionBits>{ledTimerHandle, coldWhiteChannel}, GammaCorrection};

    // -----------------------------------------------------------------------------------------------
    size_t calculateWarmWhiteLevel()
    {
        if (colorTemperature < NeutralColorTemperature)
        {
            return PwmSteps;
        }
        else
        {
            return (1.0f - calculateFactor(colorTemperature, NeutralColorTemperature, ColdColorTemperature)) * PwmSteps;
        }
    }

    // -----------------------------------------------------------------------------------------------
    size_t calculateColdWhiteLevel()
    {
        if (colorTemperature < NeutralColorTemperature)
        {
            return calculateFactor(colorTemperature, WarmColorTemperature, NeutralColorTemperature) * PwmSteps;
        }
        else
        {
            return PwmSteps;
        }
    }

    // -----------------------------------------------------------------------------------------------
    float calculateFactor(const units::si::Temperature &clampedTemp, const units::si::Temperature &minTemp,
                          const units::si::Temperature &maxTemp)
    {
        return ((clampedTemp - minTemp) / (maxTemp - minTemp)).getMagnitude();
    }

    // -----------------------------------------------------------------------------------------------
    void updateImmediately(bool state = true)
    {
        warmWhiteLedStrip.setLightLevel(state ? (calculateWarmWhiteLevel() * globalBrightness) / 100 : 0);
        coldWhiteLedStrip.setLightLevel(state ? (calculateColdWhiteLevel() * globalBrightness) / 100 : 0);
    }

    // -----------------------------------------------------------------------------------------------
    void updateWithFade(bool state = true)
    {
        warmWhiteLedStrip.fadeLightLevelTo(state ? (calculateWarmWhiteLevel() * globalBrightness) / 100 : 0);
        coldWhiteLedStrip.fadeLightLevelTo(state ? (calculateColdWhiteLevel() * globalBrightness) / 100 : 0);
    }
};