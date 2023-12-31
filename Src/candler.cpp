#include "candler.hpp"

#include <optional>
#include <vector>

#include "templaty.hpp"

// add your chip here if not supported
#ifdef STM32F042x6
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"
#endif


namespace Candler {
    void callback_on_can_error(CAN_HandleTypeDef* hcan);

    void callback_on_rx_fifo_msg_pending(CAN_HandleTypeDef* hcan);

    void callback_on_rx_fifo_full(CAN_HandleTypeDef* hcan);

    struct Interface {
        std::optional<Templaty::RtosEventInterface<CanTaskEvent>> event_group;
        std::vector<CAN_HandleTypeDef *> can_handles;
    };

    static Interface candler_interface{};
    static Callbacks candler_callbacks{};

    void candler_task_init() {
        candler_interface.event_group.emplace();
    }

    void start_it_up_boi(CAN_HandleTypeDef* hcan) {
        HAL_CAN_RegisterCallback(hcan, HAL_CAN_ERROR_CB_ID, callback_on_can_error);
        HAL_CAN_RegisterCallback(hcan, HAL_CAN_RX_FIFO0_MSG_PENDING_CB_ID, callback_on_rx_fifo_msg_pending);
        HAL_CAN_RegisterCallback(hcan, HAL_CAN_RX_FIFO0_FULL_CB_ID, callback_on_rx_fifo_full);

        HAL_CAN_ActivateNotification(hcan, CAN_IT_ERROR | CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO0_FULL);
        HAL_CAN_Start(hcan);
    }

    void add_can_handle(CAN_HandleTypeDef* hcan) {
        // I swear to god if you free this god dam hcan I will lose my mind
        // If you ever have a memory leak from thisCallMeBackSometimePlzList call me and ill come and knock some sence into you
        candler_interface.can_handles.push_back(hcan);
    }

    [[noreturn]] void candler_task(void* arguments) {
        // if this fails call `candler_task_init` before starting this task.
        assert_param(candler_interface.event_group.has_value());

        for (const auto handle_of_the_can: candler_interface.can_handles) {
            start_it_up_boi(handle_of_the_can);
        }

        for (;;) {
            auto task_events = candler_interface.event_group->wait_for_any();

            if (task_events.IsSet(CanTaskEvent::Error)) {
                // hahahhahahaaahhahhhh there is a fucking error
                for (const auto hcan: candler_interface.can_handles) {
                    const auto error_codes = HAL_CAN_GetError(hcan);
                    if (error_codes == 0) {
                        continue;
                    }

                    ReportData report_msg{};
                    report_msg.ErrorCode.emplace(error_codes);
                    report_msg.TimeStamp = std::chrono::milliseconds(HAL_GetTick());

                    switch (error_codes) {
                        case HAL_CAN_ERROR_NOT_INITIALIZED: {
                            candler_interface.event_group->set(CanTaskEvent::Restart);
                            break;
                        }

                        default: break;
                    }

                    candler_callbacks.Report(report_msg);
                    // Tommorows fucking issue
                }
            }

            if (task_events.IsSet(CanTaskEvent::RxMsgPending)) {
                CAN_RxHeaderTypeDef can_rx_header;
                uint8_t can_rx_data[8] = {0};

                for (const auto hcan: candler_interface.can_handles) {
                    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &can_rx_header, can_rx_data) != HAL_OK) {
                        continue;
                    }
                }
            }
        }
    }

    void set_reporting_callback(void (*pCallback)(ReportData report_msg)) {
        candler_callbacks.Report = pCallback;
    }

    void set_rx_callback(u_int16_t id,
                         void (*pRxData)(CAN_RxHeaderTypeDef header, uint8_t data[], CAN_HandleTypeDef* hcan)) {
        candler_callbacks.RxData = pRxData;
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
