#pragma once

#include "LedStrip.hpp"
#include "MessageProcessor.hpp"
#include "StatusLed.hpp"
#include "can/CanInterface.hpp"

/// The entry point of users C++ firmware. This comes after CubeHAL and FreeRTOS initialization.
/// All needed classes and objects have the root here.
class Application
{
public:
    static constexpr auto CanPeripherie = &hfdcan1;

    struct LedRedGreen
    {
        static constexpr auto PwmTimer = &htim1;
        static constexpr auto RedChannel = TIM_CHANNEL_1;
        static constexpr auto GreenChannel = TIM_CHANNEL_2;
    };

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

    StatusLed statusLeds{LedRedGreen::PwmTimer, LedRedGreen::RedChannel, LedRedGreen::GreenChannel};
    LedStrip ledStrip0{LedStrip0::PwmTimer, LedStrip0::WarmWhiteChannel, LedStrip0::ColdWhiteChannel};
    LedStrip ledStrip1{LedStrip1::PwmTimer, LedStrip1::WarmWhiteChannel, LedStrip1::ColdWhiteChannel};
    std::array<LedStrip *, 2> ledStrips{&ledStrip0, &ledStrip1};

    util::Gpio addressBit0{addressBit0_GPIO_Port, addressBit0_Pin};
    util::Gpio addressBit1{addressBit1_GPIO_Port, addressBit1_Pin};
    util::Gpio addressBit2{addressBit2_GPIO_Port, addressBit2_Pin};
    const uint8_t LightDriverIndex{determineAddressBits()};

    static constexpr auto StreamBufferSize = 256;
    util::wrappers::StreamBuffer canBusRxStream{StreamBufferSize, 0};
    util::wrappers::StreamBuffer canBusTxStream{StreamBufferSize, 0};
    CanInterface canInterface{CanPeripherie, canBusRxStream, canBusTxStream};

    MessageProcessor messageProcessor{ledStrips, LightDriverIndex, canBusRxStream, canBusTxStream};

    void registerCallbacks();
    uint8_t determineAddressBits();
    void setupCanBus();
};
