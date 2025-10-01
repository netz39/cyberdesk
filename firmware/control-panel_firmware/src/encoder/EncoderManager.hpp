#pragma once

#include "Encoder.hpp"

#include <memory>
#include <vector>

class EncoderManager
{
public:
    //-------------------------------------------------------------------------------------------------
    EncoderManager(size_t numberOfEncoders)
    {
        // either 1 or 2 encoders are supported
        configASSERT(numberOfEncoders == 1 || numberOfEncoders == 2);

        for (size_t i = 0; i < numberOfEncoders; i++)
        {
            encoders[i].startEncoderDetection();
        }
    }
    //-------------------------------------------------------------------------------------------------

    std::array<util::Button, 2> encoderButtons{
        util::Button{{Encoder0_Button_GPIO_Port, Encoder0_Button_Pin}}, // button at SW1 on PCB,
        util::Button{{Encoder2_Button_GPIO_Port, Encoder2_Button_Pin}}  // SW3 on PCB
    };

    std::array<util::Button, 2> powerButtons{
        util::Button{{Encoder1_Button_GPIO_Port, Encoder1_Button_Pin}}, // SW2 on PCB,
        util::Button{{Encoder3_Button_GPIO_Port, Encoder3_Button_Pin}}  // SW4 on PCB
    };

    std::array<Encoder, 2> encoders{
        Encoder{&htim2, encoderButtons[0], powerButtons[0]}, // encoder SW1 with power button SW2 on PCB
        Encoder{&htim4, encoderButtons[1], powerButtons[1]}  // encoder SW3 with power button SW4 on PCB
    };
};
