#pragma once

// リセットの誤操作防止。リセットだけダブルタップで確定させる。
// 長押しは必要時間が画面から読み取れないため採用しない（2026-10-11 決定）。

#include <cstdint>
#include <optional>

#include "../application/life_command.h"
#include "../domain/life_state.h"

namespace life::ui {

struct ResetConfirmationConfig {
    // 1回目から2回目までの猶予。2026-10-11 実機確認で 500ms は短すぎたため 1000ms。
    // 経過がこの値**以上**になったら猶予切れ（ちょうど window_ms は不成立）。
    // 時刻はチャタリング除去後にタッチが確定した時点のもの（TouchGate が渡すタイムスタンプ）。
    std::uint32_t window_ms = 1000;
};

class ResetConfirmation {
public:
    static constexpr int kNoPending = -1;

    explicit ResetConfirmation(const ResetConfirmationConfig& config = ResetConfirmationConfig{});

    // 確定したコマンドだけを返す。Reset は同じプレイヤーで2回目が来たときのみ通す。
    // Increment / Decrement はそのまま通し、待機中のリセットは取り消す。
    std::optional<app::LifeCommand> accept(const app::LifeCommand& command,
                                           std::uint32_t timestamp_ms);

    // タッチが無いときも呼び、猶予切れを処理する。
    void tick(std::uint32_t timestamp_ms);

    // 2回目を待っているプレイヤー。待機していなければ kNoPending。表示の強調に使う。
    int pending_player() const { return pending_player_; }

private:
    static std::uint32_t elapsed_ms(std::uint32_t from, std::uint32_t to) { return to - from; }

    ResetConfirmationConfig config_;
    int pending_player_ = kNoPending;
    std::uint32_t pending_since_ms_ = 0;
};

}  // namespace life::ui
