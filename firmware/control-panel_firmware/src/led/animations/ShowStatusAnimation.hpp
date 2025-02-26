#pragma once

#include "FadingAnimation.hpp"
#include "LedAnimationBase.hpp"

#include <algorithm>

/// show various status
class ShowStatusAnimation : public LedAnimationBase
{
public:
    explicit ShowStatusAnimation(LedSegmentArray &ledData) : ledData(ledData) {};

    ~ShowStatusAnimation() override = default;

    // \param lastLedIndex all leds < lastLedIndex will be on
    // 0 -> all off
    // 12 -> all 12 leds on
    void showBrightness(uint8_t lastLedIndex)
    {
        lastLedIndex = std::clamp((int)lastLedIndex, 0, NumberOfFeedbackLeds);
        fadingAnimation.setFadingTime(150.0_ms);

        for (size_t i = 0; i < lastLedIndex; i++)
            fadingAnimation.targetLedData[i] = 0.5 * NeutralWhite;

        for (size_t i = lastLedIndex; i < NumberOfFeedbackLeds; i++)
            fadingAnimation.targetLedData[i] = 0.05 * Blue;

        resetAnimation();
    }

    // \param currentLedPosition at this position the currently selected color temperature will be shown
    // 1 -> most left led focused -> 2700K
    // 12 -> most right led focused -> 6500K
    void showColorTemperature(uint8_t currentLedPosition)
    {
        currentLedPosition = std::clamp((int)currentLedPosition, 1, NumberOfFeedbackLeds);
        fadingAnimation.setFadingTime(150.0_ms);

        constexpr std::array<BgrColor, NumberOfFeedbackLeds> templateColors{
            BgrColor{0, 100, 255},   BgrColor{0, 190, 255},   BgrColor{50, 190, 255},  BgrColor{100, 200, 255},
            BgrColor{100, 255, 255}, BgrColor{255, 255, 255}, BgrColor{255, 250, 230}, BgrColor{255, 150, 150},
            BgrColor{255, 150, 100}, BgrColor{255, 100, 100}, BgrColor{255, 100, 50},  BgrColor{255, 50, 0}};

        for (size_t i = 0; i < NumberOfFeedbackLeds; i++)
            fadingAnimation.targetLedData[i] = 0.2 * templateColors[i];

        fadingAnimation.targetLedData[currentLedPosition - 1] = 0.8 * templateColors[currentLedPosition - 1];

        resetAnimation();
    }

    void turnOff()
    {
        fadingAnimation.setFadingTime(300.0_ms);

        for (size_t i = 0; i < NumberOfFeedbackLeds; i++)
            fadingAnimation.targetLedData[i] = ColorOff;

        triggerFading();
    }

    void doAnimationStep() override
    {
        // timeout after 5 seconds
        if (isTimeoutRunning && ticksToTime(xTaskGetTickCount() - ticksCounter) > 5.0_s)
        {
            turnOff();
            isTimeoutRunning = false;
        }

        if (fadingAnimation.isAnimationFinished())
            setDelay(20.0_ms);

        else
            fadingAnimation.doAnimationStep();
    }

protected:
    void resetInheritedAnimation() override
    {
        ticksCounter = xTaskGetTickCount();
        isTimeoutRunning = true;
        triggerFading();
    }

private:
    LedSegmentArray &ledData;

    FadingAnimation fadingAnimation{ledData};

    size_t ticksCounter = 0;
    bool isTimeoutRunning = false;

    void triggerFading()
    {
        fadingAnimation.resetAnimation();
        setDelay(fadingAnimation.getDelay());
    }
};
