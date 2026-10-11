#pragma once

// タッチの確定処理。1回の押下につきコマンドは1回だけ発行する。
// 生座標の補正・回転はデバイス層の責務で、ここには補正済みの論理座標が渡る。

#include <cstdint>
#include <optional>

#include "../application/life_command.h"
#include "geometry.h"
#include "layout.h"

namespace life::ui {

struct TouchSample {
    bool pressed;
    Point point;                 // pressed==false のとき内容は無視される
    std::uint32_t timestamp_ms;  // 単調増加。32bitの折返しを許容する
};

struct TouchGateConfig {
    std::uint32_t press_debounce_ms = 25;    // この時間押下が続いて初めて確定
    std::uint32_t release_debounce_ms = 50;  // この時間離れて初めて次を受け付ける
};

class TouchGate {
public:
    TouchGate(const QuadLayout& layout, const TouchGateConfig& config = TouchGateConfig{});

    // 1サンプルを与え、確定した操作があればコマンドを返す。
    std::optional<app::LifeCommand> update(const TouchSample& sample);

private:
    enum class Phase {
        Idle,      // 離れており、次の押下を受け付けられる
        Settling,  // 押下を検出しチャタリング待ち
        Consumed,  // この押下のコマンドは発行済み。離れるまで何も出さない
        Releasing, // 離れたが確定待ち
    };

    static std::uint32_t elapsed_ms(std::uint32_t from, std::uint32_t to) {
        return to - from;  // 符号なし減算で折返しも正しく扱う
    }

    const QuadLayout& layout_;
    TouchGateConfig config_;
    Phase phase_ = Phase::Idle;
    std::uint32_t phase_since_ms_ = 0;
};

}  // namespace life::ui
