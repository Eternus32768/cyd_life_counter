#include "lgfx_display.h"

#include <cstdint>

namespace life::platform::cyd {

CydDisplay::CydDisplay() {
    {
        auto cfg = bus_.config();
        cfg.spi_host = SPI2_HOST;  // ESP32のHSPI
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;
        cfg.freq_read = 16000000;
        cfg.spi_3wire = false;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = kTftSclk;
        cfg.pin_mosi = kTftMosi;
        cfg.pin_miso = kTftMiso;
        cfg.pin_dc = kTftDc;
        bus_.config(cfg);
        panel_.setBus(&bus_);
    }
    {
        auto cfg = panel_.config();
        cfg.pin_cs = kTftCs;
        cfg.pin_rst = kTftRst;
        cfg.pin_busy = -1;
        cfg.memory_width = kPanelMemoryWidth;
        cfg.memory_height = kPanelMemoryHeight;
        cfg.panel_width = kPanelMemoryWidth;
        cfg.panel_height = kPanelMemoryHeight;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        cfg.offset_rotation = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits = 1;
        cfg.readable = false;
        cfg.invert = kPanelInvert;
        cfg.rgb_order = kPanelRgbOrder;
        cfg.dlen_16bit = false;
        cfg.bus_shared = false;
        panel_.config(cfg);
    }
    {
        auto cfg = light_.config();
        cfg.pin_bl = kTftBacklight;
        cfg.invert = false;
        cfg.freq = 44100;
        cfg.pwm_channel = 7;
        light_.config(cfg);
        panel_.setLight(&light_);
    }
    {
        auto cfg = touch_.config();
        cfg.x_min = kTouchRawXMin;
        cfg.x_max = kTouchRawXMax;
        cfg.y_min = kTouchRawYMin;
        cfg.y_max = kTouchRawYMax;
        cfg.pin_int = kTouchIrq;
        cfg.bus_shared = false;
        cfg.offset_rotation = kTouchOffsetRotation;
        cfg.spi_host = SPI3_HOST;  // ESP32のVSPIを任意ピンで使う
        cfg.freq = 1000000;
        cfg.pin_sclk = kTouchSclk;
        cfg.pin_mosi = kTouchMosi;
        cfg.pin_miso = kTouchMiso;
        cfg.pin_cs = kTouchCs;
        touch_.config(cfg);
        panel_.setTouch(&touch_);
    }
    setPanel(&panel_);
}

bool CydDisplay::begin_display() {
    if (!init()) {
        return false;
    }
    setRotation(kDisplayRotation);
    setBrightness(255);
    if (kUseTouchCalibration) {
        // setTouchCalibrate は非constポインタを取るため複製して渡す。
        std::uint16_t parameters[8];
        for (int i = 0; i < 8; ++i) {
            parameters[i] = kTouchCalibration[i];
        }
        setTouchCalibrate(parameters);
    }
    return width() == kScreenWidth && height() == kScreenHeight;
}

std::uint32_t CydDisplay::read_panel_id(std::uint8_t command) {
    if (panel() == nullptr) {
        return 0;
    }
    return panel()->readCommand(command, 0, 4);
}

}  // namespace life::platform::cyd
