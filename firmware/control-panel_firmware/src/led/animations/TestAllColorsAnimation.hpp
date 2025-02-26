#pragma once

#include "LedAnimationBase.hpp"

class TestAllColorsAnimation : public LedAnimationBase
{
public:
    explicit TestAllColorsAnimation(LedSegmentArray &ledData) : ledData(ledData) {};
    ~TestAllColorsAnimation() override = default;

    void doAnimationStep() override
    {
        for (size_t i = 0; i <= head; i++)
            ledData[i] = targetColor * 0.25;

        if (++head >= NumberOfFeedbackLeds)
        {
            head = 0;
            targetColor = ColorArray[colorIndex];

            if (++colorIndex > ColorArray.size())
            {
                colorIndex = 0;
                animationIsFinished();
            }
        }
        setDelay(50.0_ms);
    }

protected:
    void resetInheritedAnimation() override
    {
        for (size_t i = 0; i < NumberOfFeedbackLeds; i++)
            ledData[i] = ColorOff;

        colorIndex = 0;
        targetColor = ColorArray[colorIndex++];

        head = 0;
    }

private:
    LedSegmentArray &ledData;

    static constexpr std::array ColorArray{Red, Green, Blue, ColorOff};

    size_t colorIndex = 0;
    BgrColor targetColor = ColorArray[colorIndex++];

    size_t head = 0;
};
