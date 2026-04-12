#pragma once

#include "Encoder.hpp"

class UserInterface
{
public:
    //-------------------------------------------------------------------------------------------------
    UserInterface()
    {
        encoder.startEncoderDetection();
    }

    //-------------------------------------------------------------------------------------------------
    void doButtonsSampling(units::si::Time buttonSamplingInterval)
    {
        encoderButton.update(buttonSamplingInterval);
        powerButton.update(buttonSamplingInterval);
        syncButton.update(buttonSamplingInterval);
    }

    // -------------------------------------------------------------------------------------------------
    // Each user interface corresponds to one encoder, its push button, a power button and a sync button
    Encoder encoder{&htim2};                                                      // SW1
    util::Button encoderButton{{Encoder0_Button_GPIO_Port, Encoder0_Button_Pin}}; // SW1
    util::Button powerButton{{Encoder1_Button_GPIO_Port, Encoder1_Button_Pin}};   // SW2
    util::Button syncButton{{Encoder2_Button_GPIO_Port, Encoder2_Button_Pin}};    // SW3
};
