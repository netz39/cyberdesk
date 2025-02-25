#pragma once

#include "MessageProcessor.hpp"
#include "can/CanInterface.hpp"
#include "led/LedStrip.hpp"
#include "led/StatusLeds.hpp"

/// The entry point of users C++ firmware. This comes after CubeHAL and FreeRTOS initialization.
/// All needed classes and objects have the root here.
class Application
{
public:
    const util::Gpio ledRedGpio{ledRed_GPIO_Port, ledRed_Pin};
    const util::Gpio ledGreenGpio{ledGreen_GPIO_Port, ledGreen_Pin};

    static constexpr auto CanPeripherie = &hfdcan1;

    struct LedStrip0
    {
        static constexpr auto PwmTimer = &htim3;
        static constexpr auto WarmWhiteChannel = TIM_CHANNEL_1;
        static constexpr auto ColdWhiteChannel = TIM_CHANNEL_2;
    };

    struct LedStrip1
    {
        static constexpr auto PwmTimer = &htim4;
        static constexpr auto WarmWhiteChannel = TIM_CHANNEL_1;
        static constexpr auto ColdWhiteChannel = TIM_CHANNEL_2;
    };

    Application();
    [[noreturn]] void run();

    static Application &getApplicationInstance();

    static inline Application *instance{nullptr};

    StatusLeds statusLeds{ledRedGpio, ledGreenGpio};
    LedStrip ledStrip0{LedStrip0::PwmTimer, LedStrip0::WarmWhiteChannel, LedStrip0::ColdWhiteChannel};
    LedStrip ledStrip1{LedStrip1::PwmTimer, LedStrip1::WarmWhiteChannel, LedStrip1::ColdWhiteChannel};

    util::Gpio addressBit0{addressBit0_GPIO_Port, addressBit0_Pin};
    util::Gpio addressBit1{addressBit1_GPIO_Port, addressBit1_Pin};
    util::Gpio addressBit2{addressBit2_GPIO_Port, addressBit2_Pin};
    uint8_t lightDriverIndex = 0;

    static constexpr auto StreamBufferSize = 256;
    util::wrappers::StreamBuffer canBusRxStream{StreamBufferSize, 0};
    util::wrappers::StreamBuffer canBusTxStream{StreamBufferSize, 0};
    CanInterface canInterface{CanPeripherie, canBusRxStream, canBusTxStream};

    MessageProcessor messageProcessor{ledStrip0, ledStrip1, lightDriverIndex, canBusRxStream, canBusTxStream};

    void registerCallbacks();
    void determineAddressBits();
    void setupCanBus();
};
