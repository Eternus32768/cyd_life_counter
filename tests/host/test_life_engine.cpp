// SPEC.md「状態と操作」/ tests/ACCEPTANCE.md の自動テスト項目に対応。

#include <limits>

#include "../../src/application/life_engine.h"
#include "test_framework.h"

namespace {

using life::app::LifeAction;
using life::app::LifeCommand;
using life::app::LifeEngine;
using life::domain::kPlayerCount;

void expect_all(const LifeEngine& engine, std::int32_t p0, std::int32_t p1, std::int32_t p2,
                std::int32_t p3) {
    CHECK_EQ(engine.snapshot().value_of(0), p0);
    CHECK_EQ(engine.snapshot().value_of(1), p1);
    CHECK_EQ(engine.snapshot().value_of(2), p2);
    CHECK_EQ(engine.snapshot().value_of(3), p3);
}

TEST(initial_state_is_zero_for_all_players) {
    LifeEngine engine;
    expect_all(engine, 0, 0, 0, 0);
}

TEST(increment_and_decrement_affect_only_target_player) {
    LifeEngine engine;
    CHECK_TRUE(engine.apply(LifeCommand{0, LifeAction::Increment}));
    expect_all(engine, 1, 0, 0, 0);
    CHECK_TRUE(engine.apply(LifeCommand{0, LifeAction::Decrement}));
    expect_all(engine, 0, 0, 0, 0);
    // 0で下げ止めせず負値を許容する。
    CHECK_TRUE(engine.apply(LifeCommand{0, LifeAction::Decrement}));
    expect_all(engine, -1, 0, 0, 0);
}

TEST(each_player_is_independent) {
    LifeEngine engine;
    for (int player_id = 0; player_id < kPlayerCount; ++player_id) {
        for (int i = 0; i <= player_id; ++i) {
            CHECK_TRUE(engine.apply(LifeCommand{player_id, LifeAction::Increment}));
        }
    }
    expect_all(engine, 1, 2, 3, 4);
}

TEST(reset_clears_only_target_player) {
    LifeEngine engine;
    engine.apply(LifeCommand{0, LifeAction::Increment});
    engine.apply(LifeCommand{1, LifeAction::Increment});
    engine.apply(LifeCommand{1, LifeAction::Increment});
    engine.apply(LifeCommand{2, LifeAction::Decrement});
    engine.apply(LifeCommand{3, LifeAction::Increment});
    expect_all(engine, 1, 2, -1, 1);

    CHECK_TRUE(engine.apply(LifeCommand{1, LifeAction::Reset}));
    expect_all(engine, 1, 0, -1, 1);

    CHECK_TRUE(engine.apply(LifeCommand{2, LifeAction::Reset}));
    expect_all(engine, 1, 0, 0, 1);
}

TEST(reset_is_idempotent_and_accepted) {
    LifeEngine engine;
    CHECK_TRUE(engine.apply(LifeCommand{3, LifeAction::Reset}));
    expect_all(engine, 0, 0, 0, 0);
}

TEST(operations_work_after_reset) {
    LifeEngine engine;
    engine.apply(LifeCommand{2, LifeAction::Increment});
    engine.apply(LifeCommand{2, LifeAction::Reset});
    CHECK_TRUE(engine.apply(LifeCommand{2, LifeAction::Increment}));
    CHECK_EQ(engine.snapshot().value_of(2), 1);
    CHECK_TRUE(engine.apply(LifeCommand{2, LifeAction::Decrement}));
    CHECK_TRUE(engine.apply(LifeCommand{2, LifeAction::Decrement}));
    CHECK_EQ(engine.snapshot().value_of(2), -1);
}

TEST(invalid_player_id_does_not_change_state) {
    LifeEngine engine;
    engine.apply(LifeCommand{0, LifeAction::Increment});

    const int invalid_ids[] = {-1, kPlayerCount, 99, -12345, 1 << 20};
    for (int id : invalid_ids) {
        CHECK_FALSE(engine.apply(LifeCommand{id, LifeAction::Increment}));
        CHECK_FALSE(engine.apply(LifeCommand{id, LifeAction::Decrement}));
        CHECK_FALSE(engine.apply(LifeCommand{id, LifeAction::Reset}));
        CHECK_EQ(engine.snapshot().value_of(id), 0);  // 読み出しも配列外へ出ない
    }
    expect_all(engine, 1, 0, 0, 0);
}

TEST(unknown_action_does_not_change_state) {
    LifeEngine engine;
    engine.apply(LifeCommand{0, LifeAction::Increment});
    const LifeAction unknown = static_cast<LifeAction>(99);
    CHECK_FALSE(engine.apply(LifeCommand{0, unknown}));
    CHECK_FALSE(engine.apply(LifeCommand{1, static_cast<LifeAction>(-3)}));
    expect_all(engine, 1, 0, 0, 0);
}

TEST(saturates_at_int32_max) {
    constexpr std::int32_t kMax = std::numeric_limits<std::int32_t>::max();
    LifeEngine engine(kMax);
    CHECK_FALSE(engine.apply(LifeCommand{0, LifeAction::Increment}));
    CHECK_EQ(engine.snapshot().value_of(0), kMax);  // オーバーフローせず変化なし
    CHECK_TRUE(engine.apply(LifeCommand{0, LifeAction::Decrement}));
    CHECK_EQ(engine.snapshot().value_of(0), kMax - 1);
    CHECK_TRUE(engine.apply(LifeCommand{0, LifeAction::Increment}));
    CHECK_EQ(engine.snapshot().value_of(0), kMax);
}

TEST(saturates_at_int32_min) {
    constexpr std::int32_t kMin = std::numeric_limits<std::int32_t>::min();
    LifeEngine engine(kMin);
    CHECK_FALSE(engine.apply(LifeCommand{3, LifeAction::Decrement}));
    CHECK_EQ(engine.snapshot().value_of(3), kMin);  // アンダーフローせず変化なし
    CHECK_TRUE(engine.apply(LifeCommand{3, LifeAction::Increment}));
    CHECK_EQ(engine.snapshot().value_of(3), kMin + 1);
}

TEST(reset_returns_to_initial_value_from_extremes) {
    LifeEngine engine(std::numeric_limits<std::int32_t>::max());
    CHECK_TRUE(engine.apply(LifeCommand{1, LifeAction::Reset}));
    CHECK_EQ(engine.snapshot().value_of(1), life::domain::kInitialValue);
    CHECK_EQ(engine.snapshot().value_of(0), std::numeric_limits<std::int32_t>::max());
}

}  // namespace
