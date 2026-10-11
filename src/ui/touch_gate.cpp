#include "touch_gate.h"

namespace life::ui {

TouchGate::TouchGate(const QuadLayout& layout, const TouchGateConfig& config)
    : layout_(layout), config_(config) {}

std::optional<app::LifeCommand> TouchGate::update(const TouchSample& sample) {
    switch (phase_) {
        case Phase::Idle:
            if (sample.pressed) {
                phase_ = Phase::Settling;
                phase_since_ms_ = sample.timestamp_ms;
            }
            return std::nullopt;

        case Phase::Settling:
            if (!sample.pressed) {
                phase_ = Phase::Idle;  // 確定前に離れた＝チャタリングとして捨てる
                return std::nullopt;
            }
            if (elapsed_ms(phase_since_ms_, sample.timestamp_ms) < config_.press_debounce_ms) {
                return std::nullopt;
            }
            // 確定時点の座標で判定する。確定後は離すまで再発行しない。
            phase_ = Phase::Consumed;
            phase_since_ms_ = sample.timestamp_ms;
            return layout_.hit_test(sample.point);

        case Phase::Consumed:
            if (!sample.pressed) {
                phase_ = Phase::Releasing;
                phase_since_ms_ = sample.timestamp_ms;
            }
            return std::nullopt;

        case Phase::Releasing:
            if (sample.pressed) {
                phase_ = Phase::Consumed;  // 離脱確定前の再接触は同一操作の揺れとみなす
                return std::nullopt;
            }
            if (elapsed_ms(phase_since_ms_, sample.timestamp_ms) >= config_.release_debounce_ms) {
                phase_ = Phase::Idle;
            }
            return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace life::ui
