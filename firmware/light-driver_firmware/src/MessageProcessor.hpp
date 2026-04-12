#pragma once

#include "helpers/freertos.hpp"
#include "wrappers/StreamBuffer.hpp"
#include "wrappers/Task.hpp"

#include "LedStrip.hpp"
#include "can/can_ids.hpp"

class MessageProcessor : public util::wrappers::TaskWithMemberFunctionBase
{

public:
    MessageProcessor(std::array<LedStrip *, 2> &ledStrips, const uint8_t LightDriverIndex,
                     util::wrappers::StreamBuffer &canBusRxStream, util::wrappers::StreamBuffer &canBusTxStream)
        : TaskWithMemberFunctionBase("MessageProcessor", 512, osPriorityAboveNormal6), //
          ledStrips(ledStrips),                                                        //
          LightDriverIndex(LightDriverIndex),                                          //
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

            // decode CAN ID and determine command and target LED strip
            std::optional<can_id::DecodedId> decodedId = can_id::decodeId(rxHeader->Identifier);

            if (!decodedId.has_value())
                // received message with invalid ID, ignore
                continue;

            if (decodedId->isGlobal)
            {
                // global command, apply to all strips
                processCommand(decodedId->command, rxData, can_id::LedType::LongSide);
                processCommand(decodedId->command, rxData, can_id::LedType::ShortSide);
            }
            else if (decodedId->ledDriverIndex == LightDriverIndex)
            {
                // command for specific strip
                processCommand(decodedId->command, rxData, decodedId->ledType);
            }
        }
    }

private:
    std::array<LedStrip *, 2> &ledStrips;
    const uint8_t LightDriverIndex;
    util::wrappers::StreamBuffer &canBusRxStream;
    util::wrappers::StreamBuffer &canBusTxStream;

    static constexpr auto BufferSize = 64;
    uint8_t messageBuffer[BufferSize];

    void processCommand(can_id::IdBase command, uint8_t *rxData, can_id::LedType ledType)
    {
        switch (command)
        {
        case can_id::IdBase::Brightness:
            setBrightness(rxData[0], ledType);
            break;

        case can_id::IdBase::ColorTemperature:
        {
            units::si::Temperature colorTemperature;
            colorTemperature.setMagnitude((rxData[1] << 8) | rxData[0]);
            setColorTemperature(colorTemperature, ledType);
        }
        break;

        default:
            break;
        }
    }

    void setBrightness(uint8_t brightness, can_id::LedType ledType)
    {
        ledStrips[ledType == can_id::LedType::LongSide ? 0 : 1]->setGlobalBrightness(brightness);
    }

    void setColorTemperature(units::si::Temperature colorTemperature, can_id::LedType ledType)
    {
        ledStrips[ledType == can_id::LedType::LongSide ? 0 : 1]->setColorTemperature(colorTemperature);
    }
};
