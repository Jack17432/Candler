#pragma once
#include <optional>
#include <chrono>

#include "clocky.hpp"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "event_groups.h"
#include "stm32f0xx_hal_can.h"

namespace Candler {
    enum class CanTaskEvent {
        Error = 0,
        RxMsgPending,
        FIFOFull,
    };

    constexpr osThreadAttr_t candler_task_thread_attr = {
        .name = "Candler",
        .stack_size = 128 * 5,
        .priority = osPriorityAboveNormal,
    };

    void candler_task_init();

    void add_can_handle(CAN_HandleTypeDef* hcan); // I swear to god if you free this god dam hcan I will lose my mind

    [[noreturn]] void candler_task(void* arguments);
}
