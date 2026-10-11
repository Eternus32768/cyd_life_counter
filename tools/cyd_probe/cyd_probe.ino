// CYD 実機診断スケッチ（製品機能ではない。向き・色順・タッチ座標の確定だけに使う）
//
// 目的: 人の目の解釈に頼らず、向き・色・タッチ対応を一意に判定する。
//  - 四隅に TL / TR / BL / BR、中央に非対称な文字 F を描く → 回転と鏡像を一意に判定
//  - 純色の帯に RED / GREEN / BLUE とラベルを描く → チャンネルの対応を一意に判定
//  - タッチのたびに生値と変換後座標をシリアルへ出す → タッチ変換を数値で判定
//
// シリアル(115200)コマンド: 0〜7=setRotation、i=invert切替、c=再描画
// ビルド: arduino-cli compile --fqbn esp32:esp32:esp32 tools/cyd_probe

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <Arduino.h>

#include "../../src/platform/cyd/board_config.h"

namespace cfgs = life::platform::cyd;

class ProbeDisplay : public lgfx::LGFX_Device {
public:
    ProbeDisplay() {
        {
            auto cfg = bus_.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = false;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = cfgs::kTftSclk;
            cfg.pin_mosi = cfgs::kTftMosi;
            cfg.pin_miso = cfgs::kTftMiso;
            cfg.pin_dc = cfgs::kTftDc;
            bus_.config(cfg);
            panel_.setBus(&bus_);
        }
        {
            auto cfg = panel_.config();
            cfg.pin_cs = cfgs::kTftCs;
            cfg.pin_rst = cfgs::kTftRst;
            cfg.pin_busy = -1;
            cfg.memory_width = cfgs::kPanelMemoryWidth;
            cfg.memory_height = cfgs::kPanelMemoryHeight;
            cfg.panel_width = cfgs::kPanelMemoryWidth;
            cfg.panel_height = cfgs::kPanelMemoryHeight;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false;
            cfg.invert = cfgs::kPanelInvert;
            cfg.rgb_order = cfgs::kPanelRgbOrder;
            cfg.bus_shared = false;
            panel_.config(cfg);
        }
        {
            auto cfg = light_.config();
            cfg.pin_bl = cfgs::kTftBacklight;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;
            light_.config(cfg);
            panel_.setLight(&light_);
        }
        {
            auto cfg = touch_.config();
            cfg.x_min = cfgs::kTouchRawXMin;
            cfg.x_max = cfgs::kTouchRawXMax;
            cfg.y_min = cfgs::kTouchRawYMin;
            cfg.y_max = cfgs::kTouchRawYMax;
            cfg.pin_int = cfgs::kTouchIrq;
            cfg.bus_shared = false;
            cfg.offset_rotation = cfgs::kTouchOffsetRotation;
            cfg.spi_host = SPI3_HOST;
            cfg.freq = 1000000;
            cfg.pin_sclk = cfgs::kTouchSclk;
            cfg.pin_mosi = cfgs::kTouchMosi;
            cfg.pin_miso = cfgs::kTouchMiso;
            cfg.pin_cs = cfgs::kTouchCs;
            touch_.config(cfg);
            panel_.setTouch(&touch_);
        }
        setPanel(&panel_);
    }

private:
    lgfx::Panel_ST7789 panel_;
    lgfx::Bus_SPI bus_;
    lgfx::Light_PWM light_;
    lgfx::Touch_XPT2046 touch_;
};

namespace {

ProbeDisplay display;
int g_rotation = 1;
bool g_invert = cfgs::kPanelInvert;

// Panel_Device::convertRawXY が使う実効回転。
// r&1 で x/y 入れ替え、r&2 で X 反転、r∈{1,2,4,7} で上下反転。
int touch_effective_rotation() {
    const int panel = g_rotation;
    const int offset = cfgs::kTouchOffsetRotation;
    return ((panel + offset) & 3) | ((panel & 4) ^ (offset & 4));
}

void draw_pattern() {
    display.setRotation(static_cast<std::uint8_t>(g_rotation));
    display.fillScreen(display.color888(0, 0, 0));
    display.setFont(&lgfx::fonts::Font0);
    display.setTextColor(display.color888(255, 255, 255));

    const int w = display.width();
    const int h = display.height();

    // 四隅のラベル。どの隅に何が出るかで回転と鏡像が一意に決まる。
    display.setTextSize(2);
    display.setTextDatum(lgfx::textdatum::top_left);
    display.drawString("TL", 4, 4);
    display.setTextDatum(lgfx::textdatum::top_right);
    display.drawString("TR", w - 4, 4);
    display.setTextDatum(lgfx::textdatum::bottom_left);
    display.drawString("BL", 4, h - 4);
    display.setTextDatum(lgfx::textdatum::bottom_right);
    display.drawString("BR", w - 4, h - 4);

    // 非対称な文字。鏡像かどうかが分かる。
    display.setTextSize(6);
    display.setTextDatum(lgfx::textdatum::middle_center);
    display.drawString("F", w / 2, h / 2 - 20);

    // 純色の帯＋ラベル。チャンネル対応が一意に決まる。
    const int bar_w = w / 3;
    const int bar_y = h - 60;
    const int bar_h = 28;
    display.fillRect(0, bar_y, bar_w, bar_h, display.color888(255, 0, 0));
    display.fillRect(bar_w, bar_y, bar_w, bar_h, display.color888(0, 255, 0));
    display.fillRect(bar_w * 2, bar_y, w - bar_w * 2, bar_h, display.color888(0, 0, 255));
    display.setTextSize(1);
    display.setTextColor(display.color888(255, 255, 255));
    display.drawString("RED", bar_w / 2, bar_y + bar_h / 2);
    display.drawString("GREEN", bar_w + bar_w / 2, bar_y + bar_h / 2);
    display.drawString("BLUE", bar_w * 2 + bar_w / 2, bar_y + bar_h / 2);

    display.setTextSize(1);
    display.setTextDatum(lgfx::textdatum::bottom_center);
    char line[96];
    std::snprintf(line, sizeof(line), "rot=%d r=%d inv=%d rgb=%d size=%dx%d", g_rotation,
                  touch_effective_rotation(), g_invert ? 1 : 0, cfgs::kPanelRgbOrder ? 1 : 0, w, h);
    display.drawString(line, w / 2, h - 2);

    // 測定条件は必ずログにも残す。画面回転だけでは実効回転を復元できず、
    // 同じ rot でも offset が違うログを取り違える（2026-10-11 の誤認の原因）。
    Serial.printf(
        "pattern: rot=%d touch_offset=%d effective_r=%d inv=%d rgb=%d size=%dx%d "
        "raw_range=x[%d,%d] y[%d,%d]\n",
        g_rotation, cfgs::kTouchOffsetRotation, touch_effective_rotation(), g_invert ? 1 : 0,
        cfgs::kPanelRgbOrder ? 1 : 0, static_cast<int>(display.width()),
        static_cast<int>(display.height()), cfgs::kTouchRawXMin, cfgs::kTouchRawXMax,
        cfgs::kTouchRawYMin, cfgs::kTouchRawYMax);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println();
    Serial.println("CYD probe: keys 0-7=rotation, i=invert, c=redraw");
    display.init();
    display.setBrightness(255);
    draw_pattern();
}

void loop() {
    while (Serial.available() > 0) {
        const int key = Serial.read();
        if (key >= '0' && key <= '7') {
            g_rotation = key - '0';
            draw_pattern();
        } else if (key == 'i') {
            g_invert = !g_invert;
            display.invertDisplay(g_invert);
            Serial.printf("invert=%d\n", g_invert ? 1 : 0);
        } else if (key == 'c') {
            draw_pattern();
        }
    }

    // タッチのたびに生値と変換後座標を出す。押しっぱなしでの連続出力は抑える。
    static bool was_pressed = false;
    lgfx::touch_point_t sample;
    const bool pressed = display.getTouchRaw(&sample, 1) > 0;
    if (pressed && !was_pressed) {
        // raw と conv は同一サンプルから出す。別々に読むと指の移動でずれた値が並ぶ。
        const int raw_x = sample.x;
        const int raw_y = sample.y;
        display.convertRawXY(&sample, 1);
        Serial.printf("touch raw=(%d,%d) conv=(%d,%d) rot=%d touch_offset=%d effective_r=%d\n",
                      raw_x, raw_y, sample.x, sample.y, g_rotation, cfgs::kTouchOffsetRotation,
                      touch_effective_rotation());
        display.fillCircle(sample.x, sample.y, 4, display.color888(255, 255, 0));
    }
    was_pressed = pressed;
    delay(20);
}
