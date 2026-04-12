#pragma once

#include "helpers/freertos.hpp"
#include "wrappers/Task.hpp"

#include "CanMesssageSender.hpp"
#include "led/FeedbackLedBar.hpp"
#include "led/LedDataTypes.hpp"
#include "user_interface/UserInterface.hpp"

class StateMachine : public util::wrappers::TaskWithMemberFunctionBase
{
public:
    //-------------------------------------------------------------------------------------------------
    StateMachine(const size_t ControlPanelIndex, CanMessageSender &canMessageSender, FeedbackLedBar &feedbackLedBar) //
        : TaskWithMemberFunctionBase("stateMachine", 512, osPriorityBelowNormal4),                                   //
          ControlPanelIndex(ControlPanelIndex),                                                                      //
          canMessageSender(canMessageSender),                                                                        //
          feedbackLedBar(feedbackLedBar),                                                                            //
          userInterface()
    {
        userInterface.encoderButton.setCallback(
            std::bind(&StateMachine::encoderButtonCallback, this, std::placeholders::_1));

        userInterface.powerButton.setCallback(
            std::bind(&StateMachine::powerButtonCallback, this, std::placeholders::_1));

        // ToDo: set sync button callback
        // userInterface.syncButton.setCallback(
        //     std::bind(&StateMachine::syncButtonCallback, this, std::placeholders::_1));
    }

protected:
    //-------------------------------------------------------------------------------------------------
    void taskMain(void *) override
    {
        auto lastWakeTime = xTaskGetTickCount();
        constexpr auto TaskSamplingInterval = 10.0_ms;

        while (true)
        {
            // evaluate encoder delta and do button sampling
            encoderDelta = userInterface.encoder.getEncoderDelta();
            userInterface.doButtonsSampling(TaskSamplingInterval);
            processEncoderMovement();

            vTaskDelayUntil(&lastWakeTime, toOsTicks(TaskSamplingInterval));
        }
    }
    //-------------------------------------------------------------------------------------------------

private:
    const size_t ControlPanelIndex;

    // maps consecutive index pairs to light drivers: 0,1 -> 1; 2,3 -> 2
    const size_t TargetLightDriverIndex{ControlPanelIndex / 2 + 1};

    // determines if this control panel controls the long or short side leds
    const can_id::LedType TargetLedType{(ControlPanelIndex % 2 == 0) ? can_id::LedType::LongSide
                                                                     : can_id::LedType::ShortSide};

    CanMessageSender &canMessageSender;
    FeedbackLedBar &feedbackLedBar;
    UserInterface userInterface;

    int encoderDelta;

    static constexpr auto MaximumLevel = NumberOfFeedbackLeds;
    static constexpr auto DefaultBrightnessLevel = 9;       // 80% brightness
    static constexpr auto DefaultColorTemperatureLevel = 6; // 4200K color temperature
    static constexpr auto StepPerBrightnessLevel = 8;
    static constexpr auto StepPerColorTemperatureLevel = 300;
    static constexpr auto StartColorTemperature = 2700 - StepPerColorTemperatureLevel; // because of 1-based index

    uint8_t brightnessLevels = DefaultBrightnessLevel;
    uint8_t colorTemperatureLevels = DefaultColorTemperatureLevel;
    bool powerState = false;
    bool isInColorChangeState = false;

    //-------------------------------------------------------------------------------------------------
    void processEncoderMovement()
    {
        if (encoderDelta == 0)
            return;

        powerState = true; // turn on led strip if encoder is moved
        isInColorChangeState ? updateColorTemperature() : updateBrightness();
    }

    //-------------------------------------------------------------------------------------------------
    void updateBrightness()
    {
        brightnessLevels = //
            std::clamp(brightnessLevels + encoderDelta, 0, MaximumLevel);

        publishBrightness(brightnessLevels);
    }

    //-------------------------------------------------------------------------------------------------
    void updateColorTemperature()
    {
        colorTemperatureLevels = //
            std::clamp(colorTemperatureLevels + encoderDelta, 1, MaximumLevel);

        publishColorTemperature(colorTemperatureLevels);
    }

    //-------------------------------------------------------------------------------------------------
    void togglePower()
    {
        powerState = !powerState;
        isInColorChangeState = false; // reset to brightness mode on power toggle
        publishBrightness(powerState ? brightnessLevels : 0);
    }

    //-------------------------------------------------------------------------------------------------
    void resetBrightnessToDefault()
    {
        brightnessLevels = DefaultBrightnessLevel;
        publishBrightness(brightnessLevels);
    }

    //-------------------------------------------------------------------------------------------------
    void resetColorTemperatureToDefault()
    {
        colorTemperatureLevels = DefaultColorTemperatureLevel;
        publishColorTemperature(colorTemperatureLevels);
    }

    //-------------------------------------------------------------------------------------------------
    void publishBrightness(uint8_t brightnessLevel)
    {
        uint8_t percentage = brightnessLevel * StepPerBrightnessLevel;

        if (percentage > 100)
            percentage = 100;

        canMessageSender.sendBrightnessMessage(TargetLightDriverIndex, TargetLedType, percentage);
        feedbackLedBar.showStatusAnimation.showBrightness(brightnessLevel);
    }

    //-------------------------------------------------------------------------------------------------
    void publishColorTemperature(uint8_t colorTemperatureLevel)
    {
        uint16_t colorTemperature = StartColorTemperature + colorTemperatureLevel * StepPerColorTemperatureLevel;

        if (colorTemperature > 6500)
            colorTemperature = 6500;

        canMessageSender.sendColorTemperatureMessage(TargetLightDriverIndex, TargetLedType, colorTemperature);
        feedbackLedBar.showStatusAnimation.showColorTemperature(colorTemperatureLevel);
    }

    //-------------------------------------------------------------------------------------------------
    void encoderButtonCallback(util::Button::Action action)
    {
        switch (action)
        {
        case util::Button::Action::ShortPress:
        {
            // toggle between brightness and color change mode
            isInColorChangeState = !isInColorChangeState;

            isInColorChangeState ? feedbackLedBar.showStatusAnimation.showColorTemperature(colorTemperatureLevels)
                                 : feedbackLedBar.showStatusAnimation.showBrightness(brightnessLevels);
        }
        break;

        case util::Button::Action::LongPress:
        {
            isInColorChangeState ? resetColorTemperatureToDefault() : resetBrightnessToDefault();
        }
        default:
            break;
        }
    }

    //-------------------------------------------------------------------------------------------------
    void powerButtonCallback(util::Button::Action action)
    {
        switch (action)
        {
        case util::Button::Action::ShortPress:
            togglePower();
            break;

        case util::Button::Action::LongPress:
            powerState = true;
            resetColorTemperatureToDefault();
            resetBrightnessToDefault();
            break;

        default:
            break;
        }
    }
};