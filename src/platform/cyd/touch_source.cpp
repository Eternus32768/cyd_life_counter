#include "touch_source.h"

#include <Arduino.h>

namespace life::platform::cyd {

ui::TouchSample read_touch(CydDisplay& display) {
    std::int32_t x = 0;
    std::int32_t y = 0;
    const bool pressed = display.getTouch(&x, &y);
    return ui::TouchSample{pressed, ui::Point{static_cast<int>(x), static_cast<int>(y)},
                           static_cast<std::uint32_t>(millis())};
}

}  // namespace life::platform::cyd
