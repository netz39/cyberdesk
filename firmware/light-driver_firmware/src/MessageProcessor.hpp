#pragma once

#include "helpers/freertos.hpp"
#include "wrappers/StreamBuffer.hpp"
#include "wrappers/Task.hpp"

#include "can/can_ids.hpp"
#include "led/LedStrip.hpp"

class MessageProcessor : public util::wrappers::TaskWithMemberFunctionBase
{

public:
    MessageProcessor(LedStrip &ledStrip0, LedStrip &ledStrip1, uint8_t &lightDriverIndex,
                     util::wrappers::StreamBuffer &canBusRxStream, util::wrappers::StreamBuffer &canBusTxStream)
        : TaskWithMemberFunctionBase("MessageProcessor", 512, osPriorityAboveNormal6), //
          ledStrip0(ledStrip0),                                                        //
          ledStrip1(ledStrip1),                                                        //
          lightDriverIndex(lightDriverIndex),                                          //
          canBusRxStream(canBusRxStream),                                              //
          canBusTxStream(canBusTxStream)
    {
    }

protected:
    void taskMain(void *) override
    {
        constexpr auto HeaderSize = sizeof(FDCAN_RxHeaderTypeDef);
        while (true)
        {
            // wait for new messages from CAN bus
            auto numberOfBytes = canBusRxStream.receive(std::span(messageBuffer, HeaderSize), portMAX_DELAY);

            if (numberOfBytes == 0)
            {
                // failed to receive message, which should not happen
                // reset buffer to prevent further errors
                canBusRxStream.reset();
                continue;
            }

            FDCAN_RxHeaderTypeDef *rxHeader = reinterpret_cast<FDCAN_RxHeaderTypeDef *>(messageBuffer);

            // receive the rest of the message
            numberOfBytes = canBusRxStream.receive(std::span(messageBuffer + HeaderSize, rxHeader->DataLength),
                                                   toOsTicks(100.0_ms));

            if (numberOfBytes == 0 || numberOfBytes != rxHeader->DataLength)
            {
                // failed to receive rest of message, which should not happen
                // reset buffer to prevent further errors
                canBusRxStream.reset();
                continue;
            }

            uint8_t *rxData = messageBuffer + sizeof(FDCAN_RxHeaderTypeDef);

            if (rxHeader->Identifier < static_cast<uint8_t>(can_id::IdBase::Status) + can_id::LightDriverOffset)
            {
                // global control message for all strips
                if (rxHeader->Identifier <= static_cast<uint8_t>(can_id::IdBase::Reserved2))
                {
                    can_id::IdBase command = static_cast<can_id::IdBase>(rxHeader->Identifier);
                    // control both strips
                    processCommand(command, rxData, 0);
                    processCommand(command, rxData, 1);
                }
            }
            else
            {
                uint8_t command = rxHeader->Identifier - lightDriverIndex * can_id::LightDriverOffset;
                uint8_t ledStripIndex = 0;

                if (command > static_cast<uint8_t>(can_id::IdBase::Reserved2))
                {
                    ledStripIndex = 1;
                    command -= can_id::LedStripOffset;
                }

                processCommand(static_cast<can_id::IdBase>(command), rxData, ledStripIndex);
            }
        }
    }

private:
    LedStrip &ledStrip0;
    LedStrip &ledStrip1;
    uint8_t &lightDriverIndex;
    util::wrappers::StreamBuffer &canBusRxStream;
    util::wrappers::StreamBuffer &canBusTxStream;

    static constexpr auto BufferSize = 64;
    uint8_t messageBuffer[BufferSize];

    void processCommand(can_id::IdBase command, uint8_t *rxData, uint8_t ledStripIndex)
    {
        switch (command)
        {
        case can_id::IdBase::Brightness:
            setBrightness(rxData[0], ledStripIndex);
            break;

        case can_id::IdBase::ColorTemperature:
        {
            units::si::Temperature colorTemperature;
            colorTemperature.setMagnitude((rxData[1] << 8) | rxData[0]);
            setColorTemperature(colorTemperature, ledStripIndex);
        }
        break;

        default:
            break;
        }
    }

    void setBrightness(uint8_t brightness, uint8_t ledStripIndex)
    {
        if (ledStripIndex == 0)
            ledStrip0.setGlobalBrightness(brightness);

        else
            ledStrip1.setGlobalBrightness(brightness);
    }

    void setColorTemperature(units::si::Temperature colorTemperature, uint8_t ledStripIndex)
    {
        if (ledStripIndex == 0)
            ledStrip0.setColorTemperature(colorTemperature);

        else
            ledStrip1.setColorTemperature(colorTemperature);
    }
};
