#pragma once

#include "can/can_ids.hpp"
#include "fdcan.h"
#include "wrappers/StreamBuffer.hpp"

#include <cstring>

class CanMessageSender
{
public:
    CanMessageSender(util::wrappers::StreamBuffer &canBusTxStream) //
        : canBusTxStream(canBusTxStream)
    {
    }

    //-------------------------------------------------------------------------------------------------
    FDCAN_TxHeaderTypeDef createDefaultTxHeader()
    {
        FDCAN_TxHeaderTypeDef txHeader;
        txHeader.Identifier = 0;
        txHeader.IdType = FDCAN_STANDARD_ID;
        txHeader.TxFrameType = FDCAN_DATA_FRAME;
        txHeader.ErrorStateIndicator = FDCAN_ESI_PASSIVE;
        txHeader.BitRateSwitch = FDCAN_BRS_OFF;
        txHeader.FDFormat = FDCAN_CLASSIC_CAN;
        txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        return txHeader;
    }

    //-------------------------------------------------------------------------------------------------
    void sendCanMessage(can_id::IdBase baseCommand, uint8_t lightDriverIndex, uint8_t ledStripIndex, uint16_t payload)
    {
        FDCAN_TxHeaderTypeDef txHeader = createDefaultTxHeader();

        auto finalCommand = static_cast<uint32_t>(baseCommand) + lightDriverIndex * can_id::LightDriverOffset +
                            ledStripIndex * can_id::LedStripOffset;
        txHeader.Identifier = finalCommand;

        if (baseCommand == can_id::IdBase::Brightness)
        {
            txHeader.DataLength = 1;
            uint8_t payload8Bit = payload & 0xFF;
            std::memcpy(txBuffer + sizeof(txHeader), &payload8Bit, txHeader.DataLength);
        }
        else if (baseCommand == can_id::IdBase::ColorTemperature)
        {
            txHeader.DataLength = 2;
            std::memcpy(txBuffer + sizeof(txHeader), reinterpret_cast<uint8_t *>(&payload), txHeader.DataLength);
        }
        else
            txHeader.DataLength = 0;

        std::memcpy(txBuffer, &txHeader, sizeof(txHeader));

        canBusTxStream.send(std::span(txBuffer, sizeof(txHeader) + txHeader.DataLength), portMAX_DELAY);
    }
    //-------------------------------------------------------------------------------------------------

private:
    util::wrappers::StreamBuffer &canBusTxStream;
    static constexpr auto BufferSize = 64;
    uint8_t txBuffer[BufferSize];
};