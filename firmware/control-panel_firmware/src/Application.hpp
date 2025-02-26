#pragma once

#include "StatusLed.hpp"
#include "can/CanInterface.hpp"
#include "led/FeedbackLedBar.hpp"

/// The entry point of users C++ firmware. This comes after CubeHAL and FreeRTOS initialization.
/// All needed classes and objects have the root here.
class Application
{
public:
    util::Gpio ledRedGpio{ledRed_GPIO_Port, ledRed_Pin};
    util::Gpio ledGreenGpio{ledGreen_GPIO_Port, ledGreen_Pin};

    static constexpr auto CanPeripherie = &hfdcan1;
    static constexpr auto LedSpiPeripherie = &hspi1;

    Application();
    [[noreturn]] void run();

    static Application &getApplicationInstance();

    static inline Application *instance{nullptr};

    static constexpr size_t CanBusBufferSize = 128;
    util::wrappers::StreamBuffer canBusRxStream{CanBusBufferSize, 0};
    util::wrappers::StreamBuffer canBusTxStream{CanBusBufferSize, 0};

    util::Gpio addressBit0{addressBit0_GPIO_Port, addressBit0_Pin};
    util::Gpio addressBit1{addressBit1_GPIO_Port, addressBit1_Pin};
    util::Gpio addressBit2{addressBit2_GPIO_Port, addressBit2_Pin};
    uint8_t controlPanelIndex = 0;

    void registerCallbacks();
    void determineAddressBits();
    void setupCanBus();

    StatusLed statusLed{ledRedGpio, ledGreenGpio};
    CanInterface canInterface{CanPeripherie, canBusRxStream, canBusTxStream};
    FeedbackLedBar feedbackLedBar{LedSpiPeripherie};
};
