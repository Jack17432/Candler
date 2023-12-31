#pragma once
#include "cmsis_os2.h"
#include <chrono>
#include <optional>

#ifdef STM32F042x6
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"
#endif

namespace Candler {
    enum class CanTaskEvent {
        Error = 0,
        RxMsgPending,
        FIFOFull,
        Start,
        Restart,
        Shutdown,
    };

    struct ReportData {
        std::optional<uint32_t> ErrorCode;
        std::chrono::milliseconds TimeStamp;
    };

    struct Callbacks {
        void (*Report)(ReportData the_god_dam_report_msg);

        void (*RxData)(CAN_RxHeaderTypeDef header, uint8_t data[8], CAN_HandleTypeDef* hcan);
    };

    constexpr osThreadAttr_t candler_task_thread_attr = {
        .name = "Candler",
        .stack_size = 128 * 5,
        .priority = osPriorityAboveNormal,
    };

    void candler_task_init();

    void add_can_handle(CAN_HandleTypeDef* hcan); // I swear to god if you free this god dam hcan I will lose my mind

    void set_reporting_callback(void (*pCallback)(ReportData report_msg));

    void set_rx_callback(u_int16_t id,
                         void (*pRxData)(CAN_RxHeaderTypeDef header, uint8_t data[8], CAN_HandleTypeDef* hcan));

    [[noreturn]] void candler_task(void* arguments);
}
