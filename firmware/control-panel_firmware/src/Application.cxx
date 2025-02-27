#include "FreeRTOS.h"
#include "main.h"
#include "task.h"

#include "Application.hpp"
#include "can/can_ids.hpp"
#include "wrappers/Task.hpp"

#include <cstring>
#include <memory>

extern "C" void StartDefaultTask(void *) // NOLINT
{
    static auto app = std::make_unique<Application>();
    app->run();

    configASSERT(false); // this line should be never reached
}

//--------------------------------------------------------------------------------------------------
Application::Application()
{
    // Delegated Singleton, see getApplicationInstance() for further explanations
    configASSERT(instance == nullptr);
    instance = this;

    registerCallbacks();
    setupCanBus();

    statusLed.ledRedGreen.setColorBlinking(util::led::binary::DualLedColor::Green, 0.5_Hz);
}

uint8_t txBuffer[64];

//--------------------------------------------------------------------------------------------------
[[noreturn]] void Application::run()
{
    util::wrappers::Task::applicationIsReadyStartAllTasks();
    while (true)
    {
        vTaskDelay(toOsTicks(5000.0_ms));
        FDCAN_TxHeaderTypeDef txHeader;
        txHeader.Identifier =
            static_cast<uint8_t>(can_id::IdBase::Brightness) + controlPanelIndex * can_id::LightDriverOffset;
        txHeader.DataLength = FDCAN_DLC_BYTES_1;
        txHeader.TxFrameType = FDCAN_DATA_FRAME;
        txHeader.IdType = FDCAN_STANDARD_ID;
        txHeader.ErrorStateIndicator = FDCAN_ESI_PASSIVE;
        txHeader.BitRateSwitch = FDCAN_BRS_OFF;
        txHeader.FDFormat = FDCAN_CLASSIC_CAN;
        txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        txHeader.MessageMarker = 0;

        std::memcpy(txBuffer, &txHeader, sizeof(txHeader));
        txBuffer[sizeof(txHeader)] = 0x12;

        canBusTxStream.send(std::span(txBuffer, sizeof(txHeader) + 1), portMAX_DELAY);

        // vTaskDelay(portMAX_DELAY);
    }
}

//--------------------------------------------------------------------------------------------------
Application &Application::getApplicationInstance()
{
    // Not constructing Application in this singleton, to avoid bugs where something tries to
    // access this function, while application constructs which will cause infinite rec2ursion
    return *instance;
}

//--------------------------------------------------------------------------------------------------
void Application::registerCallbacks()
{
    HAL_StatusTypeDef result = HAL_OK;

    // SPI callback for addressable LEDs
    result = HAL_SPI_RegisterCallback(LedSpiPeripherie, HAL_SPI_TX_COMPLETE_CB_ID, [](SPI_HandleTypeDef *)
                                      { getApplicationInstance().feedbackLedBar.notifySpiIsFinished(); });

    configASSERT(result == HAL_OK);
}

//--------------------------------------------------------------------------------------------------
uint8_t Application::determineAddressBits()
{
    controlPanelIndex = addressBit0.read() ? 1 : 0 | addressBit1.read() ? 2 : 0 | addressBit2.read() ? 4 : 0;
    return controlPanelIndex;
}

//--------------------------------------------------------------------------------------------------
void Application::setupCanBus()
{
    FDCAN_FilterTypeDef filter;
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterType = FDCAN_FILTER_MASK;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID2 = 0x7FF; // all 11 bits must match
    filter.FilterIndex = 0;
    configASSERT(HAL_FDCAN_ConfigFilter(CanPeripherie, &filter) == HAL_OK);

    // helper lambda to configure a filter bank
    auto configureFilter = [&filter](uint32_t id, uint32_t index)
    {
        filter.FilterID1 = id;
        filter.FilterIndex = index;
        configASSERT(HAL_FDCAN_ConfigFilter(CanPeripherie, &filter) == HAL_OK);
    };

    // set can id filters
    if (controlPanelIndex == 0)
    {
        // special case for the main control panel
        // only the long led strip from both light drivers
        configureFilter(static_cast<uint8_t>(can_id::IdBase::Status) + can_id::LightDriverOffset, 0);
        configureFilter(static_cast<uint8_t>(can_id::IdBase::Status) + 2 * can_id::LightDriverOffset, 1);
    }
    else
    {
        const auto ControlPanelOffset = controlPanelIndex * can_id::LightDriverOffset;
        configureFilter(static_cast<uint8_t>(can_id::IdBase::Status) + ControlPanelOffset, 0);
        configureFilter(static_cast<uint8_t>(can_id::IdBase::Status) + ControlPanelOffset + can_id::LedStripOffset, 1);
    }

    configASSERT(HAL_FDCAN_Start(CanPeripherie) == HAL_OK);
    configASSERT(HAL_FDCAN_ActivateNotification(CanPeripherie, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) == HAL_OK);

    util::wrappers::Task::syncEventGroup.setBits(sync_events::CanBusStarted);
}

//--------------------------------------------------------------------------------------------------
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan)
{
    Application::getApplicationInstance().canInterface.receiveFifoMessageFromIsr();
}