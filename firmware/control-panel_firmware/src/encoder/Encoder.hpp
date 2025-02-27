#pragma once

#include "gcem.hpp"
#include "helpers/freertos.hpp"
#include "util/Button.hpp"
#include "wrappers/Task.hpp"

#include "tim.h"

class Encoder
{
public:
    Encoder(TIM_HandleTypeDef *encoderTimer, util::Button &encoderButton) //
        : encoderTimer(encoderTimer),                                     //
          encoderButton(encoderButton)
    {
        configASSERT(encoderTimer != nullptr);
    }

    // ----------------------------------------------------------------------------
    void startEncoderDetection()
    {
        configASSERT(HAL_TIM_Encoder_Start(encoderTimer, TIM_CHANNEL_ALL) == HAL_OK);
    }

    // ----------------------------------------------------------------------------
    void handleButtonSampling(units::si::Time buttonSamplingInterval)
    {
        encoderButton.update(buttonSamplingInterval);
    }

    // ----------------------------------------------------------------------------
    int getEncoderDelta()
    {
        const uint16_t EncoderValue = __HAL_TIM_GET_COUNTER(encoderTimer);
        int diff = (EncoderValue - prevEncoderValue);

        if (diff == 0 || gcem::abs(diff) < 4)
            return 0;

        // handle underflow (transition 0 -> 65535)
        if (diff >= std::numeric_limits<uint16_t>::max() / 2)
            diff = std::numeric_limits<uint16_t>::max() - diff;

        // handle overflow (transition 65535 -> 0)
        else if (diff <= -std::numeric_limits<uint16_t>::max() / 2)
            diff = std::numeric_limits<uint16_t>::max() + diff;

        // quadrature encoder has 4 electrical steps per mechanic step
        diff = (diff / 4);
        prevEncoderValue = EncoderValue;

        return diff;
    }

    // ----------------------------------------------------------------------------
    void setButtonCallback(std::function<void(util::Button::Action)> callback)
    {
        encoderButton.setCallback(callback);
    }

private:
    TIM_HandleTypeDef *encoderTimer = nullptr;
    util::Button &encoderButton;

    uint16_t prevEncoderValue = 0;
};