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
    setupCanBus();

    statusLeds.ledRedGreen.setBrightness(25);
    statusLeds.ledRedGreen.setColor(util::led::pwm::DualLedColor::Green);
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
uint8_t Application::determineAddressBits()
{
    uint8_t address = addressBit0.read() ? 1 : 0 | addressBit1.read() ? 2 : 0 | addressBit2.read() ? 4 : 0;
    configASSERT(address != 0); // address bits cannot all be zero for light drivers

    return address;
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
    const auto LightDriverOffset = LightDriverIndex * can_id::LightDriverOffset;
    const auto LongSideOffset = LightDriverOffset + static_cast<uint8_t>(can_id::LedType::LongSide);
    const auto ShortSideOffset = LightDriverOffset + static_cast<uint8_t>(can_id::LedType::ShortSide);

    configureFilter(static_cast<uint8_t>(can_id::IdBase::Brightness) + LongSideOffset, 2);
    configureFilter(static_cast<uint8_t>(can_id::IdBase::ColorTemperature) + LongSideOffset, 3);
    configureFilter(static_cast<uint8_t>(can_id::IdBase::Brightness) + ShortSideOffset, 4);
    configureFilter(static_cast<uint8_t>(can_id::IdBase::ColorTemperature) + ShortSideOffset, 5);

    configASSERT(HAL_FDCAN_Start(CanPeripherie) == HAL_OK);
    configASSERT(HAL_FDCAN_ActivateNotification(CanPeripherie, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) == HAL_OK);

    util::wrappers::Task::syncEventGroup.setBits(sync_events::CanBusStarted);
}

//--------------------------------------------------------------------------------------------------
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    Application::getApplicationInstance().canInterface.receiveFifoMessageFromIsr();
}
