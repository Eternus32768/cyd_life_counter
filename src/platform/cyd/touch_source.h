#pragma once

// タッチ生値の読み出しと論理座標への変換。回転・補正はこの層（LovyanGFX設定）で完結させる。

#include "../../ui/touch_gate.h"
#include "lgfx_display.h"

namespace life::platform::cyd {

// 1回分のサンプルを取得する。座標は回転・補正適用後の論理座標。
ui::TouchSample read_touch(CydDisplay& display);

}  // namespace life::platform::cyd
