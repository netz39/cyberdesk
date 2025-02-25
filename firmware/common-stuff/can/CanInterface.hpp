#pragma once

#include "fdcan.h"
#include "sync.hpp"
#include "wrappers/StreamBuffer.hpp"
#include "wrappers/Task.hpp"

#include "helpers/freertos.hpp"

// retrieve CAN messages from FIFO and put them into the rx queue
// send messages from the tx queue to the CAN bus
class CanInterface : public util::wrappers::TaskWithMemberFunctionBase
{
public:
    CanInterface(FDCAN_HandleTypeDef *canPeripherie, util::wrappers::StreamBuffer &rxStream,
                 util::wrappers::StreamBuffer &txStream)
        : TaskWithMemberFunctionBase("canBusTask", 1024, osPriorityAboveNormal4), //
          canPeripherie(canPeripherie),                                           //
          rxStream(rxStream),                                                     //
          txStream(txStream)
    {
        configASSERT(canPeripherie != nullptr);
    }

    void receiveFifoMessageFromIsr()
    {
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        FDCAN_RxHeaderTypeDef rxHeader;

        if (HAL_FDCAN_GetRxMessage(canPeripherie, FDCAN_RX_FIFO0, &rxHeader, rxBuffer) == HAL_OK)
        {
            rxStream.sendFromISR(std::span(reinterpret_cast<uint8_t *>(&rxHeader), sizeof(FDCAN_RxHeaderTypeDef)),
                                 &higherPriorityTaskWoken);
            rxStream.sendFromISR(std::span(rxBuffer, rxHeader.DataLength), &higherPriorityTaskWoken);
        }

        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }

protected:
    void taskMain(void *) override
    {
        syncEventGroup.waitBits(sync_events::CanBusStarted, true, false, portMAX_DELAY);

        while (true)
        {
            dispatchCanMessage();
        }
    }

private:
    FDCAN_HandleTypeDef *canPeripherie = nullptr;
    util::wrappers::StreamBuffer &rxStream;
    util::wrappers::StreamBuffer &txStream;

    static constexpr auto BufferSize = 64;
    uint8_t rxBuffer[BufferSize];
    uint8_t txBuffer[BufferSize];

    void dispatchCanMessage()
    {
        // wait for a message in queue to send
        auto messageLength = txStream.receive(std::span(txBuffer, BufferSize), portMAX_DELAY);

        if (messageLength == 0)
            return;

        FDCAN_TxHeaderTypeDef *txHeader = reinterpret_cast<FDCAN_TxHeaderTypeDef *>(txBuffer);
        if (txHeader->DataLength + sizeof(FDCAN_TxHeaderTypeDef) != messageLength)
            return;

        uint8_t *txData = txBuffer + sizeof(FDCAN_TxHeaderTypeDef);

        configASSERT(HAL_FDCAN_AddMessageToTxFifoQ(canPeripherie, txHeader, txData) == HAL_OK);
    }
};