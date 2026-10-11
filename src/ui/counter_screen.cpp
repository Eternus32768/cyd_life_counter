#include "counter_screen.h"

namespace life::ui {

CounterScreen::CounterScreen(app::LifeEngine& engine, DisplayRenderer& renderer,
                             const LayoutConfig& layout_config,
                             const TouchGateConfig& touch_config, const Palette& palette,
                             const ResetConfirmationConfig& reset_config)
    : engine_(engine),
      layout_(layout_config),
      presenter_(layout_, renderer, palette),
      gate_(layout_, touch_config),
      confirmation_(reset_config) {}

void CounterScreen::draw_initial() { presenter_.draw_all(engine_.snapshot()); }

bool CounterScreen::handle_touch(const TouchSample& sample) {
    const int armed_before = confirmation_.pending_player();
    confirmation_.tick(sample.timestamp_ms);  // タッチが無くても猶予切れを進める

    // 値が変わって領域ごと描き直したプレイヤー。待機表示の二重描画を避けるために覚えておく。
    int redrawn_player = ResetConfirmation::kNoPending;

    const auto command = gate_.update(sample);
    if (command.has_value()) {
        const auto confirmed = confirmation_.accept(*command, sample.timestamp_ms);
        if (confirmed.has_value() && engine_.apply(*confirmed)) {
            presenter_.draw_player(engine_.snapshot(), confirmed->player_id);
            redrawn_player = confirmed->player_id;
        }
    }

    // 待機表示の更新。解除された側と、新しく待機に入った側の両方を描き直す。
    const int armed_after = confirmation_.pending_player();
    if (armed_after != armed_before) {
        if (armed_before != ResetConfirmation::kNoPending && armed_before != redrawn_player) {
            presenter_.draw_reset_button(armed_before, false);
        }
        if (armed_after != ResetConfirmation::kNoPending) {
            presenter_.draw_reset_button(armed_after, true);
        }
    }

    return redrawn_player != ResetConfirmation::kNoPending || armed_after != armed_before;
}

}  // namespace life::ui
