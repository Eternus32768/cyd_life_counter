#pragma once

#include <cstdint>

// ESP32-2432S028（2.8インチ、320×240横向き、液晶モジュール TPM408-2.8）のボード定義。
//
// 回転・色順・反転・タッチ変換は 2026-10-11 に実機で確定済み。判定根拠は docs/HARDWARE.md の
// 「実機で確定した設定」の表を参照（推測で変更しないこと）。ピン定義は型番共通の公開資料ベースで、
// 実機で表示とタッチが成立していることをもって妥当と判断している。
// ボード差分はこのファイルと lgfx_display.h に閉じ込める。

namespace life::platform::cyd {

// 表示（ESP32のHSPI = SPI2_HOST。14/13/12/15 はHSPIの既定ピン）
constexpr int kTftSclk = 14;
constexpr int kTftMosi = 13;
constexpr int kTftMiso = 12;
constexpr int kTftCs = 15;
constexpr int kTftDc = 2;
constexpr int kTftRst = -1;  // 基板上でリセットが結線されており制御線を持たない
constexpr int kTftBacklight = 21;

// パネル本体は縦240×320。横向き表示は回転1で得る（回転はハード層の責務）。
constexpr int kPanelMemoryWidth = 240;
constexpr int kPanelMemoryHeight = 320;
// 2026-10-11 実機確認（tools/cyd_probe）: 回転1で四隅ラベル TL/TR/BL/BR が正しい位置に出て、
// 非対称文字 F も正常（鏡像でない）。回転1が正解。
constexpr int kDisplayRotation = 1;
// パネルドライバは実機で確定する。既定は ST7789系。
// ILI9341 だった場合は -DCYD_PANEL_ILI9341 を付けてビルドする（docs/HARDWARE.md の判別手順）。
#if defined(CYD_PANEL_ILI9341)
constexpr const char* kPanelName = "ILI9341";
#else
constexpr const char* kPanelName = "ST7789";
#endif
// 2026-10-11 実機確認（tools/cyd_probe の純色帯テスト）: この個体は反転不要。
// invert=true では純色が補色で表示された（赤→水色、黄など）。
constexpr bool kPanelInvert = false;
// 2026-10-11 実機確認（tools/cyd_probe の純色帯テスト）: この個体は RGB 順。
// true では赤と青が入れ替わった。初回に「赤が青っぽい」と見えたのは invert=true による補色化が原因で、
// rgb_order の問題ではなかった。
constexpr bool kPanelRgbOrder = false;

// ドライバID読み出しコマンド。RDID4 が 0x9341 を返せば ILI9341。
constexpr std::uint8_t kCmdReadDisplayId = 0x04;  // RDDID
constexpr std::uint8_t kCmdReadId4 = 0xD3;        // RDID4（ILI9341のみ応答）

// 論理画面サイズ（回転後）。UIへ注入する値。
constexpr int kScreenWidth = kPanelMemoryHeight;   // 320
constexpr int kScreenHeight = kPanelMemoryWidth;   // 240

// タッチ XPT2046（表示とは別SPI。ESP32のVSPI = SPI3_HOST を任意ピンで使う）
constexpr int kTouchSclk = 25;
constexpr int kTouchMosi = 32;
constexpr int kTouchMiso = 39;
constexpr int kTouchCs = 33;
constexpr int kTouchIrq = 36;

// タッチ座標変換の回転オフセット。
// LovyanGFX は Panel_Device::convertRawXY で r = _internal_rotation（+ このオフセット）を使い、
// r が 1/2/4/7 のとき上下反転する。画面を 1→5 に変えると r も 1→5 になりタッチ側も反転してしまう。
// タッチ変換の実効回転 r = ((画面回転 + これ) & 3) | ((画面回転 & 4) ^ (これ & 4))。
// Panel_Device::convertRawXY は r&1 で x/y入れ替え、r&2 でX反転、r∈{1,2,4,7} で上下反転する。
// アフィンは cfg の min/max から raw.x→X昇順・raw.y→Y昇順に作られるため、
// 横向き（入れ替えが必要）で追加の反転が不要な r=5 が正しい。
// 画面回転1・オフセット4 のとき r = ((1+4)&3) | ((1&4)^(4&4)) = 1 | 4 = 5。
// 2026-10-11 実機確認: r=3 はXのみ反転、r=7 は両方反転（180°ずれ）、r=5 で一致した。
constexpr int kTouchOffsetRotation = 4;

// タッチ生値の範囲。2026-10-11 に tools/cyd_probe で四隅を実測した値に余裕を持たせたもの。
// 実測: raw.x は画面の縦方向に対応（上端445〜下端3655）、raw.y は横方向（左365〜右3655）。
// 旧値 300〜3900 では範囲が広すぎて画面下端が y=211 止まりになり、下行のリセット帯に届かなかった。
// より厳密に合わせる場合は -DCYD_TOUCH_CALIBRATION の四隅補正を使う。
constexpr int kTouchRawXMin = 420;
constexpr int kTouchRawXMax = 3750;
constexpr int kTouchRawYMin = 350;
constexpr int kTouchRawYMax = 3700;

// 四隅補正（-DCYD_TOUCH_CALIBRATION でビルドして取得した8値）。
// 取得したらここへ転記し kUseTouchCalibration を true にする。true のとき上の生値範囲より優先される。
// 手順と記録先: docs/HARDWARE.md
constexpr bool kUseTouchCalibration = false;
constexpr std::uint16_t kTouchCalibration[8] = {0, 0, 0, 0, 0, 0, 0, 0};

}  // namespace life::platform::cyd
