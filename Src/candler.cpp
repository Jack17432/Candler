#include "candler.hpp"
#include "stm32f0xx_hal_can.h"
#include "FreeRTOS.h"
#include <chrono>
#include <vector>

#include "templaty.hpp"

namespace Candler {
    void callback_on_can_error(CAN_HandleTypeDef* hcan);

    void callback_on_rx_fifo_msg_pending(CAN_HandleTypeDef* hcan);

    void callback_on_rx_fifo_full(CAN_HandleTypeDef* hcan);

    struct Interface {
        std::optional<Templaty::RtosEventInterface<CanTaskEvent>> event_group;
        std::vector<CAN_HandleTypeDef *> can_handles;
    };

    static Interface candler_interface{};

    void candler_task_init() {
        candler_interface.event_group.emplace();
    }

    void add_can_handle(CAN_HandleTypeDef* hcan) {
        // I swear to god if you free this god dam hcan I will lose my mind
        // If you ever have a memory leak from this call me and ill come and knock some sence into you
        candler_interface.can_handles.push_back(hcan);

        HAL_CAN_RegisterCallback(hcan, HAL_CAN_ERROR_CB_ID, callback_on_can_error);
        HAL_CAN_RegisterCallback(hcan, HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID, callback_on_rx_fifo_msg_pending);
        HAL_CAN_RegisterCallback(hcan, HAL_CAN_RX_FIFO1_MSG_PENDING_CB_ID, callback_on_rx_fifo_msg_pending);
        HAL_CAN_RegisterCallback(hcan, HAL_CAN_RX_FIFO0_FULL_CB_ID, callback_on_rx_fifo_full);
        HAL_CAN_RegisterCallback(hcan, HAL_CAN_RX_FIFO1_FULL_CB_ID, callback_on_rx_fifo_full);
    }

    [[noreturn]] void candler_task(void* arguments) {
        for (;;) {
            auto task_events =
                    candler_interface.event_group->wait_for_any();

            if (task_events.IsSet(CanTaskEvent::Error)) {
                // hahahhahahaaahhahhhh there is a fucking error
            }

            if (task_events.IsSet(CanTaskEvent::RxMsgPending)) {
                // Deal with a new stupid message
            }
        }
    }

    void callback_on_error(CAN_HandleTypeDef* hcan) {
        candler_interface.event_group->isr_set(CanTaskEvent::Error);
    }

    void callback_on_rx_fifo_msg_pending(CAN_HandleTypeDef* hcan) {
        candler_interface.event_group->isr_set(CanTaskEvent::RxMsgPending);
    }

    void callback_on_rx_fifo_full(CAN_HandleTypeDef* hcan) {
        candler_interface.event_group->isr_set(CanTaskEvent::FIFOFull);
    }
}
