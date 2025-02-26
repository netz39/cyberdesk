#pragma once

#include "LedAnimationBase.hpp"

class RainbowAnimation : public LedAnimationBase
{
public:
    explicit RainbowAnimation(LedSegmentArray &ledData) : ledData(ledData) {};
    ~RainbowAnimation() override = default;

    void doAnimationStep() override
    {
        if (rainbowIndex <= 0)
            rainbowIndex = LightDataSize - 1;
        else
            rainbowIndex -= 1;

        for (size_t i = 0; i < NumberOfFeedbackLeds; i++)
        {
            sineLED(ledData[i], (rainbowIndex + i * 22) % LightDataSize);
        }

        setDelay(15.0_ms);
    }

protected:
    void resetInheritedAnimation() override
    {
        rainbowIndex = 0;
    }

private:
    LedSegmentArray &ledData;

    size_t rainbowIndex = 0;

    static constexpr unsigned LightDataSize = 360;
    static constexpr auto BrightnessFactor = 0.35f;
    static constexpr uint8_t SinTable[LightDataSize] = {
        0,   0,   0,   0,   0,   1,   1,   2,   2,   3,   4,   5,   6,   7,   8,   9,   11,  12,  13,  15,  17,  18,
        20,  22,  24,  26,  28,  30,  32,  35,  37,  39,  42,  44,  47,  49,  52,  55,  58,  60,  63,  66,  69,  72,
        75,  78,  81,  85,  88,  91,  94,  97,  101, 104, 107, 111, 114, 117, 121, 124, 127, 131, 134, 137, 141, 144,
        147, 150, 154, 157, 160, 163, 167, 170, 173, 176, 179, 182, 185, 188, 191, 194, 197, 200, 202, 205, 208, 210,
        213, 215, 217, 220, 222, 224, 226, 229, 231, 232, 234, 236, 238, 239, 241, 242, 244, 245, 246, 248, 249, 250,
        251, 251, 252, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 253, 252, 251, 251, 250,
        249, 248, 246, 245, 244, 242, 241, 239, 238, 236, 234, 232, 231, 229, 226, 224, 222, 220, 217, 215, 213, 210,
        208, 205, 202, 200, 197, 194, 191, 188, 185, 182, 179, 176, 173, 170, 167, 163, 160, 157, 154, 150, 147, 144,
        141, 137, 134, 131, 127, 124, 121, 117, 114, 111, 107, 104, 101, 97,  94,  91,  88,  85,  81,  78,  75,  72,
        69,  66,  63,  60,  58,  55,  52,  49,  47,  44,  42,  39,  37,  35,  32,  30,  28,  26,  24,  22,  20,  18,
        17,  15,  13,  12,  11,  9,   8,   7,   6,   5,   4,   3,   2,   2,   1,   1,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0};

    // sine wave rainbow
    void sineLED(BgrColor &seg, int angle)
    {
        seg.red = SinTable[(angle + 120) % LightDataSize];
        seg.green = SinTable[angle];
        seg.blue = SinTable[(angle + 240) % LightDataSize];

        seg *= BrightnessFactor;
    }
};