#include "candler.hpp"
#include "stm32f0xx_hal_can.h"
#include "FreeRTOS.h"
#include <chrono>
#include <vector>

namespace Candler {
    void callback_on_can_error(CAN_HandleTypeDef* hcan);

    struct Interface {
        std::optional<RtosEventInterface<CanTaskEvent>> event_group;
        std::vector<CAN_HandleTypeDef *> can_handles;
    };

    static Interface candler_interface{};

    void candler_task_init() {
        candler_interface.event_group.emplace();
    }

    void add_can_handle(CAN_HandleTypeDef* hcan) {
        // I swear to god if you free this god dam hcan I will lose my mind
        candler_interface.can_handles.push_back(hcan);

        HAL_CAN_RegisterCallback(hcan, HAL_CAN_ERROR_CB_ID, callback_on_can_error);
    }

    [[noreturn]] void candler_task(void* arguments) {
        for (;;) {
            auto task_events =
                    candler_interface.event_group->wait_for_any();

            if (task_events.IsSet(CanTaskEvent::Error)) {
                // hahahhahahaaahhahhhh there is a fucking error
            }

            if (task_events.IsSet(CanTaskEvent::Rx)) {
                // Deal with a new stupid message
            }
        }
    }

    void callback_on_can_error(CAN_HandleTypeDef* hcan) {
        candler_interface.event_group->isr_set(CanTaskEvent::Error);
    }
}
