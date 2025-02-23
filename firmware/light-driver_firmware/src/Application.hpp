#pragma once

// #include "LED/LedStrip.hpp"
#include "LED/StatusLeds.hpp"

/// The entry point of users C++ firmware. This comes after CubeHAL and FreeRTOS initialization.
/// All needed classes and objects have the root here.
class Application
{
public:
    const util::Gpio ledRedGpio{ledRed_GPIO_Port, ledRed_Pin};
    const util::Gpio ledGreenGpio{ledGreen_GPIO_Port, ledGreen_Pin};

    // static constexpr auto LedStripPwmTimer = &htim?;
    // static constexpr auto WarmWhiteChannel = TIM_CHANNEL_?;
    // static constexpr auto ColdWhiteChannel = TIM_CHANNEL_?;

    Application();
    [[noreturn]] void run();

    static Application &getApplicationInstance();

private:
    static inline Application *instance{nullptr};

    void registerCallbacks();

    StatusLeds statusLeds{ledRedGpio, ledGreenGpio};
    // LedStrip ledStrip{LedStripPwmTimer, WarmWhiteChannel, ColdWhiteChannel};
};
