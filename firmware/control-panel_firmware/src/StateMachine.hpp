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

        userInterface.syncButton.setCallback(std::bind(&StateMachine::syncButtonCallback, this, std::placeholders::_1));
    }

    static void timeoutCallback(TimerHandle_t xTimer)
    {
        auto instance = static_cast<StateMachine *>(pvTimerGetTimerID(xTimer));

        // reset brightness and color temperature levels to default after being off for some time
        instance->brightnessLevel = DefaultBrightnessLevel;
        instance->colorTemperatureLevel = DefaultColorTemperatureLevel;
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

    uint8_t brightnessLevel = DefaultBrightnessLevel;
    uint8_t colorTemperatureLevel = DefaultColorTemperatureLevel;
    bool powerState = false;
    bool isInColorChangeState = false;

    TimerHandle_t resetLevelsTimer{
        xTimerCreate("resetLevelsTimer", toOsTicks(15.0_s), pdTRUE, this, &StateMachine::timeoutCallback)};

    //-------------------------------------------------------------------------------------------------
    void processEncoderMovement()
    {
        if (encoderDelta == 0)
            return;

        if (!powerState)
            powerOn();

        isInColorChangeState ? updateColorTemperature() : updateBrightness();
    }

    //-------------------------------------------------------------------------------------------------
    void updateBrightness()
    {
        brightnessLevel = std::clamp(brightnessLevel + encoderDelta, 0, MaximumLevel);

        if (brightnessLevel == 0)
            powerOff();

        publishBrightness(brightnessLevel);
    }

    //-------------------------------------------------------------------------------------------------
    void updateColorTemperature()
    {
        colorTemperatureLevel = std::clamp(colorTemperatureLevel + encoderDelta, 1, MaximumLevel);

        publishColorTemperature(colorTemperatureLevel);
    }

    //-------------------------------------------------------------------------------------------------
    void togglePower()
    {
        !powerState ? powerOn() : powerOff();

        if (powerState)
        {
            publishColorTemperature(colorTemperatureLevel);
            publishBrightness(brightnessLevel);
        }
        else
            publishBrightness(0);
    }

    //-------------------------------------------------------------------------------------------------
    void powerOff()
    {
        powerState = false;
        isInColorChangeState = false;

        // start timer to reset brightness and color temperature levels after being off for some time
        xTimerReset(resetLevelsTimer, 0);
    }

    //-------------------------------------------------------------------------------------------------
    void powerOn()
    {
        powerState = true;

        // stop timer to reset brightness and color temperature levels after being off for some time
        xTimerStop(resetLevelsTimer, 0);
    }

    //-------------------------------------------------------------------------------------------------
    void resetBrightnessToDefault()
    {
        brightnessLevel = DefaultBrightnessLevel;
        publishBrightness(brightnessLevel);
    }

    //-------------------------------------------------------------------------------------------------
    void resetColorTemperatureToDefault()
    {
        colorTemperatureLevel = DefaultColorTemperatureLevel;
        publishColorTemperature(colorTemperatureLevel);
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
        if (!powerState)
            return;

        switch (action)
        {
        case util::Button::Action::ShortPress:
        {
            // toggle between brightness and color change mode
            isInColorChangeState = !isInColorChangeState;

            isInColorChangeState ? feedbackLedBar.showStatusAnimation.showColorTemperature(colorTemperatureLevel)
                                 : feedbackLedBar.showStatusAnimation.showBrightness(brightnessLevel);
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
            powerOn();
            resetColorTemperatureToDefault();
            resetBrightnessToDefault();
            break;

        default:
            break;
        }
    }

    //-------------------------------------------------------------------------------------------------
    void syncButtonCallback(util::Button::Action action)
    {
        // ToDo: short press: sync brightness and color temperature levels of all light drivers to the current
        // levels of this control panel

        // ToDo: long press: turn off all lights except the one related to this control panel
    }
};