// リセットのダブルタップ確定（SPEC.md「リセット誤操作防止」の決定、2026-10-11）。

#include "../../src/ui/reset_confirmation.h"
#include "test_framework.h"

namespace {

using life::app::LifeAction;
using life::app::LifeCommand;
using life::ui::ResetConfirmation;
using life::ui::ResetConfirmationConfig;

LifeCommand reset_of(int player_id) { return LifeCommand{player_id, LifeAction::Reset}; }

TEST(default_window_is_one_second) {
    // 2026-10-11 実機確認での決定値。変更時はこのテストと SPEC.md を更新する。
    CHECK_EQ(ResetConfirmationConfig{}.window_ms, static_cast<std::uint32_t>(1000));
}

TEST(increment_and_decrement_pass_through_immediately) {
    ResetConfirmation confirmation;
    const auto increment = confirmation.accept(LifeCommand{0, LifeAction::Increment}, 0);
    CHECK_TRUE(increment.has_value());
    CHECK_TRUE(increment->action == LifeAction::Increment);

    const auto decrement = confirmation.accept(LifeCommand{0, LifeAction::Decrement}, 10);
    CHECK_TRUE(decrement.has_value());
    CHECK_EQ(confirmation.pending_player(), ResetConfirmation::kNoPending);
}

TEST(first_reset_tap_is_held_and_second_confirms) {
    ResetConfirmation confirmation;
    CHECK_FALSE(confirmation.accept(reset_of(2), 1000).has_value());
    CHECK_EQ(confirmation.pending_player(), 2);

    const auto confirmed = confirmation.accept(reset_of(2), 1200);
    CHECK_TRUE(confirmed.has_value());
    CHECK_EQ(confirmed->player_id, 2);
    CHECK_TRUE(confirmed->action == LifeAction::Reset);
    CHECK_EQ(confirmation.pending_player(), ResetConfirmation::kNoPending);
}

TEST(second_tap_after_the_window_starts_over) {
    ResetConfirmation confirmation{ResetConfirmationConfig{500}};
    CHECK_FALSE(confirmation.accept(reset_of(1), 0).has_value());
    CHECK_FALSE(confirmation.accept(reset_of(1), 500).has_value());  // 猶予ちょうどは不成立
    CHECK_EQ(confirmation.pending_player(), 1);                       // 1回目として数え直す
    CHECK_TRUE(confirmation.accept(reset_of(1), 700).has_value());
}

TEST(second_tap_must_be_the_same_player) {
    ResetConfirmation confirmation;
    CHECK_FALSE(confirmation.accept(reset_of(0), 0).has_value());
    CHECK_FALSE(confirmation.accept(reset_of(3), 100).has_value());
    CHECK_EQ(confirmation.pending_player(), 3);  // 待機先が移る
    CHECK_FALSE(confirmation.accept(reset_of(0), 200).has_value());
    CHECK_TRUE(confirmation.accept(reset_of(0), 300).has_value());
}

TEST(other_actions_cancel_the_pending_reset) {
    ResetConfirmation confirmation;
    CHECK_FALSE(confirmation.accept(reset_of(1), 0).has_value());
    CHECK_TRUE(confirmation.accept(LifeCommand{1, LifeAction::Increment}, 100).has_value());
    CHECK_EQ(confirmation.pending_player(), ResetConfirmation::kNoPending);
    CHECK_FALSE(confirmation.accept(reset_of(1), 200).has_value());  // 1回目からやり直し
}

TEST(tick_expires_the_pending_reset) {
    ResetConfirmation confirmation{ResetConfirmationConfig{500}};
    confirmation.accept(reset_of(0), 0);
    confirmation.tick(499);
    CHECK_EQ(confirmation.pending_player(), 0);
    confirmation.tick(500);
    CHECK_EQ(confirmation.pending_player(), ResetConfirmation::kNoPending);
}

TEST(invalid_player_never_becomes_pending) {
    ResetConfirmation confirmation;
    CHECK_FALSE(confirmation.accept(reset_of(-1), 0).has_value());
    CHECK_FALSE(confirmation.accept(reset_of(4), 10).has_value());
    CHECK_EQ(confirmation.pending_player(), ResetConfirmation::kNoPending);
}

TEST(timestamp_wraparound_is_handled) {
    ResetConfirmation confirmation{ResetConfirmationConfig{500}};
    const std::uint32_t near_max = 0xFFFFFFFFu - 100;
    CHECK_FALSE(confirmation.accept(reset_of(2), near_max).has_value());
    CHECK_TRUE(confirmation.accept(reset_of(2), near_max + 200).has_value());  // 折り返しても成立
}

}  // namespace
