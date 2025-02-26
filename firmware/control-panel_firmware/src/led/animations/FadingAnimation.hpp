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

        // apply difference multiplied by factor to current data
        float factor = static_cast<float>(fadeProgress) / numberOfSteps;
        for (uint32_t i = 0; i < NumberOfFeedbackLeds; i++)
        {
            if (diffLedData[i].red == 0 && diffLedData[i].green == 0 && diffLedData[i].blue == 0)
                continue;

            ledData[i] = targetLedData[i] + factor * diffLedData[i];
        }

        if (fadeProgress == 0)
            animationIsFinished();

        else
            fadeProgress--;
    }

    void setFadingTime(units::si::Time fadingTime)
    {
        this->fadingTime = fadingTime;
    }

    void setTargetLedData(LedSegmentArray newData)
    {
        std::memcpy(targetLedData.data(), newData.data(), NumberOfFeedbackLeds * sizeof(BgrColor));
    }

    std::array<BgrColor, NumberOfFeedbackLeds> targetLedData;

protected:
    void resetInheritedAnimation() override
    {
        setDelay(RefreshTime);

        numberOfSteps = (fadingTime / RefreshTime).getMagnitude();
        fadeProgress = numberOfSteps - 1;

        // calc difference between current and target data
        for (uint32_t i = 0; i < NumberOfFeedbackLeds; i++)
            diffLedData[i] = ledData[i] - targetLedData[i];
    }

private:
    static constexpr auto RefreshTime = 2.0_s / configTICK_RATE_HZ;

    LedSegmentArray &ledData;

    std::array<BgrColorDiff, NumberOfFeedbackLeds> diffLedData;

    size_t numberOfSteps = 0;
    size_t fadeProgress = 0;
    units::si::Time fadingTime{0.0};
};