#pragma once

#include "helpers/freertos.hpp"
#include "wrappers/Task.hpp"

#include "CanMesssageSender.hpp"
#include "encoder/EncoderManager.hpp"
#include "led/FeedbackLedBar.hpp"
#include "led/LedDataTypes.hpp"

class StateMachine : public util::wrappers::TaskWithMemberFunctionBase
{
public:
    //-------------------------------------------------------------------------------------------------
    StateMachine(uint8_t address, CanMessageSender &canMessageSender, FeedbackLedBar &feedbackLedBar) //
        : TaskWithMemberFunctionBase("stateMachine", 512, osPriorityBelowNormal4),                    //
          address(address),                                                                           //
          numberOfEncoders(address == 0 ? 4 : 2),                                                     //
          canMessageSender(canMessageSender),                                                         //
          feedbackLedBar(feedbackLedBar),                                                             //
          encoderManager(numberOfEncoders)
    {
        // assign lambda to encoder button callbacks to insert encoder index
        for (size_t i = 0; i < numberOfEncoders; i++)
        {
            encoderManager.encoders[i].setButtonCallback([this, i](util::Button::Action action)
                                                         { commonButtonCallback(action, i); });
        }
    }

protected:
    //-------------------------------------------------------------------------------------------------
    void taskMain(void *) override
    {
        auto lastWakeTime = xTaskGetTickCount();
        constexpr auto TaskSamplingInterval = 10.0_ms;

        while (true)
        {
            // evaluate encoder deltas and do button sampling
            for (size_t i = 0; i < numberOfEncoders; i++)
            {
                encoderDeltas[i] = encoderManager.encoders[i].getEncoderDelta();
                encoderManager.encoders[i].handleButtonSampling(TaskSamplingInterval);
            }

            handleEncoderDeltas();

            vTaskDelayUntil(&lastWakeTime, toOsTicks(TaskSamplingInterval));
        }
    }
    //-------------------------------------------------------------------------------------------------

private:
    uint8_t address;
    uint8_t numberOfEncoders;
    CanMessageSender &canMessageSender;
    FeedbackLedBar &feedbackLedBar;
    EncoderManager encoderManager;
    std::array<int, 4> encoderDeltas;

    static constexpr auto NumberOfLedStrips = 2;
    static constexpr auto MaximumLevel = NumberOfFeedbackLeds;
    static constexpr auto DefaultBrightnessLevel = 9;       // 80% brightness
    static constexpr auto DefaultColorTemperatureLevel = 6; // 4200K color temperature
    static constexpr auto StepPerBrightnessLevel = 8;
    static constexpr auto StepPerColorTemperatureLevel = 300;
    static constexpr auto StartColorTemperature = 2700 - StepPerColorTemperatureLevel; // because of 1-based index

    std::array<uint8_t, NumberOfLedStrips> brightnessLevels{DefaultBrightnessLevel, //
                                                            DefaultBrightnessLevel};
    std::array<uint8_t, NumberOfLedStrips> colorTemperatureLevels{DefaultColorTemperatureLevel, //
                                                                  DefaultColorTemperatureLevel};
    std::array<bool, NumberOfLedStrips> powerState{false, false};

    //-------------------------------------------------------------------------------------------------
    void handleEncoderDeltas()
    {
        for (size_t encoderIndex = 0; encoderIndex < numberOfEncoders; encoderIndex++)
        {
            int delta = encoderDeltas[encoderIndex];
            if (delta == 0)
                continue;

            const uint8_t LedIndex = encoderIndex / 2;
            switch (encoderIndex)
            {
            case 0:
            case 2:
                updateBrightness(delta, LedIndex);
                break;

            case 1:
            case 3:
                updateColorTemperature(delta, LedIndex);
                break;

            default:
                break;
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    void updateBrightness(int delta, size_t ledIndex)
    {
        brightnessLevels[ledIndex] = //
            std::clamp(brightnessLevels[ledIndex] + delta, 0, MaximumLevel);

        publishBrightness(brightnessLevels[ledIndex], ledIndex);
    }

    //-------------------------------------------------------------------------------------------------
    void updateColorTemperature(int delta, size_t ledIndex)
    {
        colorTemperatureLevels[ledIndex] = //
            std::clamp(colorTemperatureLevels[ledIndex] + delta, 1, MaximumLevel);

        publishColorTemperature(colorTemperatureLevels[ledIndex], ledIndex);
    }

    //-------------------------------------------------------------------------------------------------
    void togglePower(size_t ledIndex)
    {
        powerState[ledIndex] = !powerState[ledIndex];
        publishBrightness(powerState[ledIndex] ? brightnessLevels[ledIndex] : 0, ledIndex);
    }

    //-------------------------------------------------------------------------------------------------
    void resetBrightnessToDefault(size_t ledIndex)
    {
        brightnessLevels[ledIndex] = DefaultBrightnessLevel;
        publishBrightness(brightnessLevels[ledIndex], ledIndex);
    }

    //-------------------------------------------------------------------------------------------------
    void resetColorTemperatureToDefault(size_t ledIndex)
    {
        colorTemperatureLevels[ledIndex] = DefaultColorTemperatureLevel;
        publishColorTemperature(colorTemperatureLevels[ledIndex], ledIndex);
    }

    //-------------------------------------------------------------------------------------------------
    void publishBrightness(uint8_t brightnessLevel, size_t ledIndex)
    {
        uint8_t percentage = brightnessLevel * StepPerBrightnessLevel;

        if (percentage > 100)
            percentage = 100;

        if (address == 0)
            // central control panel controls the long led strips
            canMessageSender.sendCanMessage(can_id::IdBase::Brightness, 1 + ledIndex, 0, percentage);

        else
            // other control panels controls the short led strips on side
            canMessageSender.sendCanMessage(can_id::IdBase::Brightness, address, 1, percentage);

        feedbackLedBar.showStatusAnimation.showBrightness(brightnessLevel);
    }

    //-------------------------------------------------------------------------------------------------
    void publishColorTemperature(uint8_t colorTemperatureLevel, size_t ledIndex)
    {
        uint16_t colorTemperature = StartColorTemperature + colorTemperatureLevel * StepPerColorTemperatureLevel;

        if (colorTemperature > 6500)
            colorTemperature = 6500;

        if (address == 0)
            // central control panel controls the long led strips
            canMessageSender.sendCanMessage(can_id::IdBase::ColorTemperature, 1 + ledIndex, 0, colorTemperature);

        else
            // other control panels controls the short led strips on side
            canMessageSender.sendCanMessage(can_id::IdBase::ColorTemperature, address, 1, colorTemperature);

        feedbackLedBar.showStatusAnimation.showColorTemperature(colorTemperatureLevel);
    }

    //-------------------------------------------------------------------------------------------------
    void commonButtonCallback(util::Button::Action action, size_t encoderIndex)
    {
        const uint8_t LedIndex = encoderIndex / 2;
        switch (action)
        {
        case util::Button::Action::ShortPress:
            togglePower(LedIndex);
            break;

        case util::Button::Action::LongPress:
        {
            switch (encoderIndex)
            {
            case 0:
            case 2:
                resetBrightnessToDefault(LedIndex);
                break;

            case 1:
            case 3:
                resetColorTemperatureToDefault(LedIndex);
                break;

            default:
                break;
            }
        }
        default:
            break;
        }
    }
};