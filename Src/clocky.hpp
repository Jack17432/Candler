#pragma once

#include <chrono>
#include "FreeRTOS.h"
#include "portmacro.h"


namespace Candler::Clocky {
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
