// CYD Life Counter v0.1 — 実機エントリポイント。
//
// このファイルは依存を組み立てるだけ。状態更新・判定ロジックは src/ 配下に置く。
// スケッチ名は Arduino の規約でフォルダ名と一致させる必要がある（フォルダを改名したら本ファイルも改名する）。
//
// ビルド例（Windows側 arduino-cli、docs/HARDWARE.md 参照）:
//   arduino-cli compile --fqbn esp32:esp32:esp32 <このフォルダ>
// LCDドライバが ILI9341 だった場合:
//   arduino-cli compile --fqbn esp32:esp32:esp32 --build-property build.extra_flags=-DCYD_PANEL_ILI9341 <このフォルダ>
// タッチ補正値を取り直す場合:
//   arduino-cli compile --fqbn esp32:esp32:esp32 --build-property build.extra_flags=-DCYD_TOUCH_CALIBRATION <このフォルダ>

#include <Arduino.h>

#include "src/application/life_engine.h"
#include "src/platform/cyd/board_config.h"
#include "src/platform/cyd/cyd_renderer.h"
#include "src/platform/cyd/lgfx_display.h"
#include "src/platform/cyd/touch_source.h"
#include "src/ui/counter_screen.h"

namespace {

life::platform::cyd::CydDisplay g_display;
life::platform::cyd::CydRenderer g_renderer(g_display);
life::app::LifeEngine g_engine;
life::ui::CounterScreen g_screen(
    g_engine, g_renderer,
    life::ui::LayoutConfig::for_screen(life::platform::cyd::kScreenWidth,
                                       life::platform::cyd::kScreenHeight));

constexpr std::uint32_t kPollIntervalMs = 5;  // タッチ確定時間より十分短く取る

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println();
    Serial.println("CYD Life Counter v0.1");

    // 実機のドライバ種別の判別材料。RDID4 の下位に 0x9341 が出れば ILI9341。
    // 結果は docs/HARDWARE.md へ記録し、必要なら -DCYD_PANEL_ILI9341 を付けて再ビルドする。
    Serial.printf("panel driver (build): %s\n", life::platform::cyd::kPanelName);

    if (!g_display.begin_display()) {
        // 初期化自体は進むが、回転後のサイズが想定と違う場合はここで気付ける。
        Serial.printf("display init warning: size=%dx%d expected=%dx%d\n",
                      static_cast<int>(g_display.width()), static_cast<int>(g_display.height()),
                      life::platform::cyd::kScreenWidth, life::platform::cyd::kScreenHeight);
    }

    Serial.printf("panel id: RDDID(0x04)=0x%08lX RDID4(0xD3)=0x%08lX\n",
                  static_cast<unsigned long>(
                      g_display.read_panel_id(life::platform::cyd::kCmdReadDisplayId)),
                  static_cast<unsigned long>(
                      g_display.read_panel_id(life::platform::cyd::kCmdReadId4)));

#ifdef CYD_TOUCH_CALIBRATION
    // 四隅をタッチして補正値を取得し、シリアルへ出力する。
    // 得られた値を src/platform/cyd/board_config.h と docs/HARDWARE.md に記録する。
    g_display.fillScreen(0x0000);
    g_display.setTextDatum(lgfx::textdatum::middle_center);
    g_display.setFont(&lgfx::fonts::Font0);
    g_display.setTextSize(2);
    g_display.setTextColor(0xFFFF);
    g_display.drawString("TOUCH CORNERS", g_display.width() / 2, g_display.height() / 2);
    std::uint16_t calibration[8] = {0};
    g_display.calibrateTouch(calibration, 0xFFFF, 0x0000, 20);
    Serial.print("touch calibration:");
    for (std::uint16_t value : calibration) {
        Serial.printf(" %u", static_cast<unsigned>(value));
    }
    Serial.println();
#endif

    g_display.fillScreen(0x0000);
    g_screen.draw_initial();
}

void loop() {
    g_screen.handle_touch(life::platform::cyd::read_touch(g_display));
    delay(kPollIntervalMs);
}
