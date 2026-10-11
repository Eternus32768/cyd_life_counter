#include "reset_confirmation.h"

namespace life::ui {

ResetConfirmation::ResetConfirmation(const ResetConfirmationConfig& config) : config_(config) {}

void ResetConfirmation::tick(std::uint32_t timestamp_ms) {
    if (pending_player_ == kNoPending) {
        return;
    }
    if (elapsed_ms(pending_since_ms_, timestamp_ms) >= config_.window_ms) {
        pending_player_ = kNoPending;
    }
}

std::optional<app::LifeCommand> ResetConfirmation::accept(const app::LifeCommand& command,
                                                          std::uint32_t timestamp_ms) {
    tick(timestamp_ms);

    if (command.action != app::LifeAction::Reset) {
        pending_player_ = kNoPending;  // 他の操作が入ったら待機を解除する
        return command;
    }
    if (!domain::LifeState::is_valid_player(command.player_id)) {
        return std::nullopt;
    }
    if (pending_player_ == command.player_id) {
        pending_player_ = kNoPending;  // 2回目。ここで確定する
        return command;
    }
    // 1回目、または別プレイヤーのリセット。待機しなおす。
    pending_player_ = command.player_id;
    pending_since_ms_ = timestamp_ms;
    return std::nullopt;
}

}  // namespace life::ui
