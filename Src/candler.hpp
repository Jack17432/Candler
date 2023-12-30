#pragma once
#include <optional>
#include <chrono>

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "event_groups.h"
#include "stm32f0xx_hal_can.h"

#define EVENT_ALL_BITS (0x00FFFFFF)

namespace Candler {
    namespace Clocky {
        using Microseconds = std::chrono::duration<int64_t, std::micro>;
        using Milliseconds = std::chrono::duration<int64_t, std::milli>;
        using Seconds = std::chrono::duration<int64_t>;
        using Minutes = std::chrono::duration<int64_t, std::ratio<60>>;
        using Hours = std::chrono::duration<int64_t, std::ratio<3600>>;


        static constexpr Milliseconds NoWait = Milliseconds::zero();
        static constexpr Milliseconds WaitForever = Milliseconds::max();

        static constexpr TickType_t duration_to_ticks(const Milliseconds ms) {
            return ms == WaitForever ? portMAX_DELAY : pdMS_TO_TICKS(ms.count());
        }
    }

    enum class CanTaskEvent {
        Error = 0,
        Rx,
    };

    enum class WaitBehavior {
        ALL,
        ANY
    };

    enum class ClearBehavior {
        CLEAR,
        LEAVE
    };

    template<typename T>
    concept ScopedEnum =
            std::integral_constant<bool, std::is_enum_v<T> && !std::is_convertible_v<T, int>>::value;

    template<typename E>
    constexpr auto to_integral(E e) -> std::underlying_type<E> {
        return static_cast<std::underlying_type<E>>(e);
    };

    class NonCopyable {
    public:
        constexpr NonCopyable() = default;

        ~NonCopyable() = default;

        NonCopyable(const NonCopyable&) = delete;

        NonCopyable& operator=(const NonCopyable&) = delete;
    };

    template<ScopedEnum E>
    class EventSet {
    private:
        uint32_t v_ = 0;

    public:
        constexpr EventSet() = default;

        constexpr explicit EventSet(E v) : v_(1u << to_integral(v)) {
        };

        constexpr EventSet(const std::initializer_list<E>& events)
            : v_([&events]() {
                uint32_t result = 0;
                for (const auto event: events) {
                    result |= 1u << to_integral(event);
                }
                return result;
            }()) {
        };

        constexpr explicit EventSet(uint32_t v) : v_(v) {
        };

        constexpr EventSet<E> operator|(E e) const {
            return *this | EventSet(e);
        };

        constexpr EventSet<E> operator|(EventSet const& lhs) const {
            EventSet<E> res;
            res.v_ = v_ | lhs.v_;
            return res;
        };

        constexpr bool IsSet(E e) const {
            return (v_ & (1u << to_integral(e))) != 0u;
        };

        constexpr void Clear(E e) {
            v_ = (v_ & ~(1u << to_integral(e)));
        };

        constexpr void Set(E e) {
            v_ = (v_ | (1u << to_integral(e)));
        };

        [[nodiscard]] constexpr bool AllOf(EventSet<E> const& other) const {
            return (v_ & other.v_) == other.v_;
        };

        [[nodiscard]] constexpr bool Empty() const {
            return v_ == 0u;
        };

        [[nodiscard]] constexpr uint32_t Value() const {
            return v_;
        };

        bool operator==(EventSet const& other) const {
            return other.v_ == v_;
        };
    };

    template<typename E, typename ES = EventSet<E>>
    class RtosEventInterface : private NonCopyable {
    private:
        EventGroupHandle_t handle_;
        StaticEventGroup_t buffer_;

    public:
        RtosEventInterface() : handle_(xEventGroupCreateStatic(&buffer_)) {
        };

        ~RtosEventInterface() noexcept {
            vEventGroupDelete(handle_);
        };

        ES get() {
            return ES(xEventGroupGetBits(handle_));
        };

        ES isr_get() {
            return ES(xEventGroupGetBitsFromISR(handle_));
        };

        void set(ES e) {
            xEventGroupSetBits(handle_, e.Value());
        };

        void isr_set(ES e) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            const BaseType_t bit_set_successfully =
                    xEventGroupSetBitsFromISR(handle_,
                                              e.Value(),
                                              &xHigherPriorityTaskWoken);

            if (bit_set_successfully == pdPASS) {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        };

        void set(E e) {
            xEventGroupSetBits(handle_, ES(e).Value());
        };

        void isr_set(E e) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            const BaseType_t bit_set_successfully =
                    xEventGroupSetBitsFromISR(handle_,
                                              ES(e).Value(),
                                              &xHigherPriorityTaskWoken);

            if (bit_set_successfully == pdPASS) {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        };

        void clear(ES e) {
            xEventGroupClearBits(handle_, e.Value());
        };

        void clear(E e) {
            xEventGroupClearBits(handle_, ES(e).Value());
        };

        void isr_clear(ES e) {
            xEventGroupClearBitsFromISR(handle_, e.Value());
        };

        void isr_clear(E e) {
            xEventGroupClearBitsFromISR(handle_, ES(e).Value());
        };

        ES wait_for_any(ClearBehavior cb = ClearBehavior::CLEAR,
                        Clocky::Milliseconds timeout = Clocky::WaitForever) {
            return ES(xEventGroupWaitBits(handle_,
                                          EVENT_ALL_BITS,
                                          cb == ClearBehavior::CLEAR,
                                          false,
                                          Clocky::duration_to_ticks(timeout)
                )
            );
        };

        ES wait_for(E e, ClearBehavior cb = ClearBehavior::CLEAR,
                    Clocky::Milliseconds timeout = Clocky::WaitForever) {
            return ES(xEventGroupWaitBits(handle_,
                                          ES(e).Value(),
                                          cb == ClearBehavior::CLEAR,
                                          true,
                                          Clocky::duration_to_ticks(timeout)
                )
            ).IsSet(e);
        };

        ES wait_for(ES e, ClearBehavior cb = ClearBehavior::CLEAR,
                    WaitBehavior wb = WaitBehavior::ANY,
                    Clocky::Milliseconds timeout = Clocky::WaitForever) {
            return ES(xEventGroupWaitBits(handle_,
                                          e.Value(),
                                          cb == ClearBehavior::CLEAR,
                                          wb == WaitBehavior::ALL,
                                          Clocky::duration_to_ticks(timeout)
                )
            );
        };
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
