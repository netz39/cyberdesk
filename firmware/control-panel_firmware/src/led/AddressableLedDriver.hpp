#pragma once

#include "FreeRTOS.h"
#include "LedDataTypes.hpp"
#include "spi.h"
#include "units/si/time.hpp"
#include "util/led/GammaCorrection.hpp"

// Controls the addressable LEDs over SPI hardware
// see following links for implementation details
// https://cpldcpu.wordpress.com/2014/11/30/understanding-the-apa102-superled/
// https://cpldcpu.wordpress.com/2016/12/13/sk9822-a-clone-of-the-apa102/
class AddressableLedDriver
{
public:
    static constexpr auto Timeout = 2.0_s;

    explicit AddressableLedDriver(SPI_HandleTypeDef *spiPeripherie) : spiPeripherie(spiPeripherie)
    {
        configASSERT(spiPeripherie != nullptr);
        endFrames.fill(0xFF);
    };

    //------------------------------------------------------------------------------------------
    void sendBuffer(LedSegmentArray &ledArray)
    {
        convertToGammaCorrectedColors(ledArray, ledSpiData);

        sendStartFrame();

        HAL_SPI_Transmit_DMA(spiPeripherie, reinterpret_cast<uint8_t *>(ledSpiData.data()),
                             ledSpiData.size() * sizeof(LedSpiData));
        ulTaskNotifyTake(pdTRUE, toOsTicks(Timeout));

        HAL_SPI_Transmit_DMA(spiPeripherie, endFrames.data(), NumberOfEndFrames);
        ulTaskNotifyTake(pdTRUE, toOsTicks(Timeout));
    }

private:
    SPI_HandleTypeDef *spiPeripherie = nullptr;

    static constexpr auto NumberOfEndFrames = (NumberOfFeedbackLeds + 15) / 16;
    std::array<uint8_t, NumberOfEndFrames> endFrames{};

    static constexpr size_t PwmSteps = 256;
    static constexpr auto ResolutionBits = std::bit_width<size_t>(PwmSteps - 1);
    using GammaCorrection_t = util::led::pwm::GammaCorrection<ResolutionBits, 2.0f>;
    static constexpr GammaCorrection_t GammaCorrection{};

    struct LedSpiData
    {
        uint8_t Start = 0xFF; //!< the first byte contains control data like brightness
        BgrColor color;

        void assignGammaCorrectedColor(BgrColor newColor)
        {
            color.blue = GammaCorrection.LookUpTable[newColor.blue];
            color.green = GammaCorrection.LookUpTable[newColor.green];
            color.red = GammaCorrection.LookUpTable[newColor.red];
        }
    };
    using LedSpiDataArray = std::array<LedSpiData, NumberOfFeedbackLeds>;

    LedSpiDataArray ledSpiData;

    //------------------------------------------------------------------------------------------
    void sendStartFrame()
    {
        uint32_t startFrame = 0;

        HAL_SPI_Transmit_DMA(spiPeripherie, reinterpret_cast<uint8_t *>(&startFrame), sizeof(startFrame));

        ulTaskNotifyTake(pdTRUE, toOsTicks(Timeout));
    }

    //------------------------------------------------------------------------------------------
    /// convert LED data to gamma corrected colors and put it to SPI-related array
    void convertToGammaCorrectedColors(LedSegmentArray &source, LedSpiDataArray &destination)
    {
        for (size_t i = 0; i < destination.size(); i++)
            destination[i].assignGammaCorrectedColor(source[i]);
    }
};