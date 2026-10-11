#pragma once

// CYD の LCD + タッチを LovyanGFX で駆動する。実機依存はこの層だけに置く。

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <cstdint>

#include "board_config.h"

namespace life::platform::cyd {

// パネルドライバはビルド時に選ぶ。既定 ST7789、`-DCYD_PANEL_ILI9341` で ILI9341。
#if defined(CYD_PANEL_ILI9341)
using CydPanelDriver = lgfx::Panel_ILI9341;
#else
using CydPanelDriver = lgfx::Panel_ST7789;
#endif

class CydDisplay : public lgfx::LGFX_Device {
public:
    CydDisplay();

    // 回転・バックライトを含む初期化。失敗時 false。
    bool begin_display();

    // ドライバICのID読み出し。実機のドライバ種別を判別するために使う。
    std::uint32_t read_panel_id(std::uint8_t command);

private:
    CydPanelDriver panel_;
    lgfx::Bus_SPI bus_;
    lgfx::Light_PWM light_;
    lgfx::Touch_XPT2046 touch_;
};

}  // namespace life::platform::cyd
