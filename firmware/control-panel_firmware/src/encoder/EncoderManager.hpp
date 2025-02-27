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
        // either 2 or 4 encoders are supported
        configASSERT(numberOfEncoders == 2 || numberOfEncoders == 4);

        for (size_t i = 0; i < numberOfEncoders; i++)
        {
            encoders[i].startEncoderDetection();
        }
    }
    //-------------------------------------------------------------------------------------------------

    std::array<util::Button, 4> encoderButtons{
        util::Button{{Encoder0_Button_GPIO_Port, Encoder0_Button_Pin}},
        util::Button{{Encoder1_Button_GPIO_Port, Encoder1_Button_Pin}},
        util::Button{{Encoder2_Button_GPIO_Port, Encoder2_Button_Pin}},
        util::Button{{Encoder3_Button_GPIO_Port, Encoder3_Button_Pin}} //
    };

    std::array<Encoder, 4> encoders{
        Encoder{&htim2, encoderButtons[0]}, //
        Encoder{&htim1, encoderButtons[1]}, //
        Encoder{&htim3, encoderButtons[2]}, //
        Encoder{&htim4, encoderButtons[3]}  //
    };
};
