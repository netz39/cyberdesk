#include "FreeRTOS.h"
#include "main.h"
#include "task.h"

#include "Application.hpp"
#include "can/can_ids.hpp"
#include "sync.hpp"
#include "wrappers/Task.hpp"

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
    determineAddressBits();

    if (lightDriverIndex == 0)
    {
        // no address bits set, this is not allowed on light driver boards
        statusLeds.ledRedGreen.setColorBlinking(util::led::binary::DualLedColor::Red, 2.0_Hz);
        return;
    }

    setupCanBus();
    statusLeds.ledRedGreen.setColorBlinking(util::led::binary::DualLedColor::Green, 0.5_Hz);
}

//--------------------------------------------------------------------------------------------------
[[noreturn]] void Application::run()
{
    util::wrappers::Task::applicationIsReadyStartAllTasks();
    while (true)
    {
        vTaskDelay(portMAX_DELAY);
    }
}

//--------------------------------------------------------------------------------------------------
Application &Application::getApplicationInstance()
{
    // Not constructing Application in this singleton, to avoid bugs where something tries to
    // access this function, while application constructs which will cause infinite recursion
    return *instance;
}

//--------------------------------------------------------------------------------------------------
void Application::registerCallbacks()
{
    HAL_StatusTypeDef result = HAL_OK;

    configASSERT(result == HAL_OK);
}

//--------------------------------------------------------------------------------------------------
void Application::determineAddressBits()
{
    lightDriverIndex = 1;
    // lightDriverIndex = addressBit0.read() ? 1 : 0 | addressBit1.read() ? 2 : 0 | addressBit2.read() ? 4 : 0;
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
    // global control messages
    configureFilter(static_cast<uint8_t>(can_id::IdBase::Brightness), 0);
    configureFilter(static_cast<uint8_t>(can_id::IdBase::ColorTemperature), 1);

    // light driver specific messages
    const auto LightDriverOffset = lightDriverIndex * can_id::LightDriverOffset;
    configureFilter(static_cast<uint8_t>(can_id::IdBase::Brightness) + LightDriverOffset, 2);
    configureFilter(static_cast<uint8_t>(can_id::IdBase::ColorTemperature) + LightDriverOffset, 3);
    configureFilter(static_cast<uint8_t>(can_id::IdBase::Brightness) + LightDriverOffset + can_id::LedStripOffset, 4);
    configureFilter(static_cast<uint8_t>(can_id::IdBase::ColorTemperature) + LightDriverOffset + can_id::LedStripOffset,
                    5);

    configASSERT(HAL_FDCAN_Start(CanPeripherie) == HAL_OK);
    configASSERT(HAL_FDCAN_ActivateNotification(CanPeripherie, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) == HAL_OK);

    util::wrappers::Task::syncEventGroup.setBits(sync_events::CanBusStarted);
}

//--------------------------------------------------------------------------------------------------
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    Application::getApplicationInstance().canInterface.receiveFifoMessageFromIsr();
}
