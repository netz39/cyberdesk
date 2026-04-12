#pragma once

#include "StateMachine.hpp"
#include "StatusLed.hpp"
#include "can/CanInterface.hpp"
#include "led/FeedbackLedBar.hpp"

/// The entry point of users C++ firmware. This comes after CubeHAL and FreeRTOS initialization.
/// All needed classes and objects have the root here.
class Application
{
public:
    struct LedRedGreen
    {
        static constexpr auto PwmTimer = &htim1;
        static constexpr auto RedChannel = TIM_CHANNEL_3;
        static constexpr auto GreenChannel = TIM_CHANNEL_4;
    };

    static constexpr auto CanPeripherie = &hfdcan1;
    static constexpr auto LedSpiPeripherie = &hspi1;

    Application();
    [[noreturn]] void run();

    static Application &getApplicationInstance();

    static inline Application *instance{nullptr};

    util::Gpio addressBit0{addressBit0_GPIO_Port, addressBit0_Pin};
    util::Gpio addressBit1{addressBit1_GPIO_Port, addressBit1_Pin};
    util::Gpio addressBit2{addressBit2_GPIO_Port, addressBit2_Pin};

    // clang-format off
    // Determine the control panel index at startup by reading the three solder pads
    const size_t ControlPanelIndex{addressBit0.read() ? 1U : 0U 
                                 | addressBit1.read() ? 2U : 0U 
                                 | addressBit2.read() ? 4U : 0U};
    // clang-format on

    static constexpr size_t CanBusBufferSize = 128;
    util::wrappers::StreamBuffer canBusRxStream{CanBusBufferSize, 0};
    util::wrappers::StreamBuffer canBusTxStream{CanBusBufferSize, 0};

    StatusLed statusLed{LedRedGreen::PwmTimer, LedRedGreen::RedChannel, LedRedGreen::GreenChannel};
    CanInterface canInterface{CanPeripherie, canBusRxStream, canBusTxStream};
    CanMessageSender canMessageSender{canBusTxStream};

    FeedbackLedBar feedbackLedBar{LedSpiPeripherie};
    StateMachine stateMachine{ControlPanelIndex, canMessageSender, feedbackLedBar};

    void registerCallbacks();
    void setupCanBus();
};
