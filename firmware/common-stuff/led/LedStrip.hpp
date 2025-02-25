#pragma once

#include "tim.h"
#include "units/si/temperature.hpp"
#include "util/MapValue.hpp"
#include "util/led/PwmLed.hpp"
#include "wrappers/Task.hpp"

// #include "Fading.hpp"

class LedStrip : public util::wrappers::TaskWithMemberFunctionBase
{
private:
    enum class State
    {
        Off,
        On,
        FadingOff,
        FadingOn
    } currentLedState = State::Off;

public:
    LedStrip(TIM_HandleTypeDef *ledTimerHandle, const uint32_t &warmWhiteChannel, const uint32_t &coldWhiteChannel)
        : TaskWithMemberFunctionBase("ledstripTask", 256, osPriorityLow2), ledTimerHandle(ledTimerHandle), //
          warmWhiteChannel(warmWhiteChannel),                                                              //
          coldWhiteChannel(coldWhiteChannel)                                                               //
    {
        configASSERT(this->ledTimerHandle != nullptr);
    }

    void turnOnImmediately()
    {
        setState(State::On);
    }

    void turnOffImmediately()
    {
        setState(State::Off);
    }

    void turnOnWithFade()
    {
        setState(State::FadingOn);
    }

    void turnOffWithFade()
    {
        setState(State::FadingOff);
    }

    void toggleState()
    {
        switch (currentLedState)
        {
        case State::On:
        case State::FadingOn:
            setState(State::FadingOff);
            break;

        case State::Off:
        case State::FadingOff:
            setState(State::FadingOn);
            break;
        }
    }

    void setState(State newState)
    {
        switch (newState)
        {
        case State::On:
            warmWhiteLedStrip.setState(true);
            coldWhiteLedStrip.setState(true);
            break;
        case State::Off:
            warmWhiteLedStrip.setState(false);
            coldWhiteLedStrip.setState(false);
            break;
        case State::FadingOn:
            warmWhiteLedStrip.setState(true);
            coldWhiteLedStrip.setState(true);
            resetFade = true;
            break;
        case State::FadingOff:
            resetFade = true;
            break;
        }

        currentLedState = newState;
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

    void incrementCCT()
    {
        if (colorTemperature < ColdColorTemperature)
            colorTemperature += ColorStep;

        updateBrightness();
    }

    void decrementCCT()
    {
        if (colorTemperature > WarmColorTemperature)
            colorTemperature -= ColorStep;

        updateBrightness();
    }

    uint8_t getGlobalBrightness() const
    {
        return globalBrightness;
    }

    units::si::Temperature getColorTemperature() const
    {
        return colorTemperature;
    }

    bool isLedStripEnabled() const
    {
        return currentLedState == State::On || currentLedState == State::FadingOn ||
               currentLedState == State::FadingOff;
    }

protected:
    [[noreturn]] void taskMain(void *)
    {
        auto lastWakeTime = xTaskGetTickCount();
        updateBrightness();

        while (true)
        {
            auto taskDelay = processFading();

            warmWhiteLedStrip.updateState(lastWakeTime);
            coldWhiteLedStrip.updateState(lastWakeTime);
            vTaskDelayUntil(&lastWakeTime, toOsTicks(taskDelay));
        }
    }

private:
    TIM_HandleTypeDef *ledTimerHandle = nullptr;
    const uint32_t &warmWhiteChannel;
    const uint32_t &coldWhiteChannel;

    static constexpr auto TaskFrequency = 100.0_Hz;
    static constexpr auto FadeDuration = 300.0_ms;

    static constexpr auto WarmColorTemperature = 2700.0_K;
    static constexpr auto ColdColorTemperature = 6000.0_K;
    static constexpr auto NeutralColorTemperature = 4200.0_K; // color mixed by warm and cold white with equal intensity
    static constexpr auto ColorStep = 100.0_K;
    units::si::Temperature colorTemperature{NeutralColorTemperature};

    uint8_t globalBrightness = 50;

    bool resetFade = true;

    // APB for timers: 64MHz -> 1024 PWM steps and prescaler (2+1) -> 20.83kHz PWM frequency
    static constexpr size_t PwmSteps = 1024;
    static constexpr auto ResolutionBits = std::bit_width<size_t>(PwmSteps - 1);
    using GammaCorrection_t = util::led::pwm::GammaCorrection<ResolutionBits, 10.0f>;
    static constexpr GammaCorrection_t GammaCorrection{};

    using SingleLed = util::led::pwm::SingleLed<ResolutionBits, GammaCorrection_t>;

    SingleLed warmWhiteLedStrip{util::PwmOutput<ResolutionBits>{ledTimerHandle, warmWhiteChannel}, GammaCorrection};
    SingleLed coldWhiteLedStrip{util::PwmOutput<ResolutionBits>{ledTimerHandle, coldWhiteChannel}, GammaCorrection};

    // -----------------------------------------------------------------------------------------------
    size_t getWarmWhiteLightLevel()
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
    size_t getColdWhiteLightLevel()
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
    void updateBrightness()
    {
        warmWhiteLedStrip.setLightLevel(getWarmWhiteLightLevel() * globalBrightness / 100);
        coldWhiteLedStrip.setLightLevel(getColdWhiteLightLevel() * globalBrightness / 100);
    }

    // -----------------------------------------------------------------------------------------------
    units::si::Time processFading()
    {
        if (currentLedState != State::FadingOn && currentLedState != State::FadingOff)
            return 1.0_ / TaskFrequency;

        static size_t currentBrightness = 0;
        static size_t targetBrightness = 0;
        static size_t stepSize = 1;
        static bool isBrightnessDecreasing = false;
        static units::si::Time delayPerStep = 10.0_ms;

        if (resetFade)
        {
            resetFade = false;

            auto resultTuple = prepareFade();
            currentBrightness = std::get<0>(resultTuple);
            targetBrightness = std::get<1>(resultTuple);
            stepSize = std::get<2>(resultTuple);
            isBrightnessDecreasing = std::get<3>(resultTuple);
            delayPerStep = std::get<4>(resultTuple);
        }

        currentBrightness += isBrightnessDecreasing ? -stepSize : stepSize;

        warmWhiteLedStrip.setLightLevel(getWarmWhiteLightLevel() * currentBrightness / 1000);
        coldWhiteLedStrip.setLightLevel(getColdWhiteLightLevel() * currentBrightness / 1000);

        if (currentBrightness == targetBrightness)
        { // finished fading
            if (currentLedState == State::FadingOff)
                setState(State::Off);

            else if (currentLedState == State::FadingOn)
                setState(State::On);

            return 1.0_ / TaskFrequency;
        }

        return delayPerStep;
    }

    // -----------------------------------------------------------------------------------------------
    std::tuple<size_t, size_t, size_t, bool, units::si::Time> prepareFade()
    {
        const size_t TargetBrightness = 10 * (currentLedState == State::FadingOn ? globalBrightness : 0);

        const size_t CurrentBrightness = 10 * (currentLedState == State::FadingOn ? 0 : globalBrightness);
        int16_t brightnessDiff = CurrentBrightness - TargetBrightness;
        uint16_t numberOfSteps = gcem::abs(brightnessDiff);

        size_t stepSize = 1;
        auto delayPerStep = FadeDuration / numberOfSteps;
        const size_t PossibleSteps = toOsTicks(FadeDuration);

        if (numberOfSteps > PossibleSteps)
        {
            // increase step size to fit into task minimum delay
            const float StepSizeFactor = static_cast<float>(numberOfSteps) / PossibleSteps;
            stepSize = ceil(StepSizeFactor); // round up to next integer

            // adjust delay per step to frame given fade duration
            delayPerStep = 1.0_ms * (stepSize / StepSizeFactor);

            // recalculate number of steps to align it with step size
            numberOfSteps -= numberOfSteps % stepSize;
        }

        const bool IsBrightnessDecreasing = brightnessDiff > 0;

        // set start point aligned to step size
        const uint16_t NewBrightness = TargetBrightness - (IsBrightnessDecreasing ? -numberOfSteps : numberOfSteps);

        return {NewBrightness, TargetBrightness, stepSize, IsBrightnessDecreasing, delayPerStep};
    }
};