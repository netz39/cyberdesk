#pragma once

#include "LedAnimationBase.hpp"
#include <cstring>

class FadingAnimation : public LedAnimationBase
{
public:
    explicit FadingAnimation(LedSegmentArray &ledData) : ledData(ledData) {};
    ~FadingAnimation() override = default;

    void doAnimationStep() override
    {
        if (isAnimationFinished())
            return;

        bool anyFading = false;

        for (uint32_t i = 0; i < NumberOfFeedbackLeds; i++)
        {
            if (fadeSteps[i] == 0)
                continue;

            anyFading = true;
            fadeSteps[i]--;

            // apply difference multiplied by progress factor to current data
            float factor = static_cast<float>(fadeSteps[i]) / numberOfSteps;
            ledData[i] = targetLedData[i] + factor * diffLedData[i];
        }

        if (!anyFading)
            animationIsFinished();
    }

    void setFadingTime(units::si::Time fadingTime)
    {
        this->fadingTime = fadingTime;
        numberOfSteps = (fadingTime / RefreshTime).getMagnitude();
    }

    // update target color and calculate fading parameters of a specific led
    void updateTargetLedPixel(uint8_t index, const BgrColor &&color)
    {
        if (index >= NumberOfFeedbackLeds)
            return;

        // only update fading parameters if target changes
        if (targetLedData[index] == color)
            return;

        targetLedData[index] = color;
        diffLedData[index] = ledData[index] - targetLedData[index];
        fadeSteps[index] = numberOfSteps;
    }

protected:
    void resetInheritedAnimation() override
    {
        setDelay(RefreshTime);
    }

private:
    static constexpr auto RefreshTime = 2.0_s / configTICK_RATE_HZ;

    LedSegmentArray &ledData;

    std::array<BgrColorDiff, NumberOfFeedbackLeds> diffLedData;
    std::array<BgrColor, NumberOfFeedbackLeds> targetLedData;
    std::array<size_t, NumberOfFeedbackLeds> fadeSteps;

    units::si::Time fadingTime{0.0};
    size_t numberOfSteps = 0;
};