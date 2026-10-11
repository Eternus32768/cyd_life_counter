// SPEC.md「タッチ1回の確定操作につき1回だけコマンド発行」/ ACCEPTANCE のタッチ項目に対応。

#include <limits>
#include <optional>

#include "../../src/ui/touch_gate.h"
#include "test_framework.h"

namespace {

using life::app::LifeAction;
using life::app::LifeCommand;
using life::ui::LayoutConfig;
using life::ui::Point;
using life::ui::QuadLayout;
using life::ui::TouchGate;
using life::ui::TouchGateConfig;
using life::ui::TouchSample;

constexpr int kWidth = 320;
constexpr int kHeight = 240;

QuadLayout make_layout() { return QuadLayout(LayoutConfig::for_screen(kWidth, kHeight)); }

Point center_of_increment(const QuadLayout& layout, int player_id) {
    const auto rect = layout.increment_button(player_id);
    return Point{rect.x + rect.width / 2, rect.y + rect.height / 2};
}

int count_commands(TouchGate& gate, const Point& point, bool pressed, std::uint32_t from_ms,
                   std::uint32_t to_ms, std::uint32_t step_ms) {
    int count = 0;
    for (std::uint32_t t = from_ms; t <= to_ms; t += step_ms) {
        if (gate.update(TouchSample{pressed, point, t}).has_value()) {
            ++count;
        }
    }
    return count;
}

TEST(single_tap_emits_exactly_one_command) {
    const QuadLayout layout = make_layout();
    TouchGate gate(layout);
    const Point point = center_of_increment(layout, 0);

    std::optional<LifeCommand> command;
    int emitted = 0;
    for (std::uint32_t t = 0; t <= 200; t += 10) {
        const bool pressed = t < 150;
        const auto result = gate.update(TouchSample{pressed, point, t});
        if (result.has_value()) {
            command = result;
            ++emitted;
        }
    }
    CHECK_EQ(emitted, 1);
    CHECK_TRUE(command.has_value());
    CHECK_EQ(command->player_id, 0);
    CHECK_TRUE(command->action == LifeAction::Increment);
}

TEST(holding_the_finger_does_not_repeat) {
    const QuadLayout layout = make_layout();
    TouchGate gate(layout);
    const Point point = center_of_increment(layout, 1);
    // 5秒間押しっぱなしでも1回だけ。
    CHECK_EQ(count_commands(gate, point, true, 0, 5000, 10), 1);
}

TEST(release_and_tap_again_emits_again) {
    const QuadLayout layout = make_layout();
    TouchGate gate(layout);
    const Point point = center_of_increment(layout, 2);
    const TouchGateConfig config{};

    int emitted = count_commands(gate, point, true, 0, 300, 10);
    CHECK_EQ(emitted, 1);
    // 離脱確定に必要な時間だけ離す。
    emitted = count_commands(gate, point, false, 310, 310 + config.release_debounce_ms + 20, 10);
    CHECK_EQ(emitted, 0);
    emitted = count_commands(gate, point, true, 500, 800, 10);
    CHECK_EQ(emitted, 1);
}

TEST(chattering_shorter_than_debounce_is_ignored) {
    const QuadLayout layout = make_layout();
    TouchGate gate(layout);
    const Point point = center_of_increment(layout, 0);
    const TouchGateConfig config{};

    int emitted = 0;
    std::uint32_t t = 0;
    for (int i = 0; i < 20; ++i) {
        // 押下は確定時間未満、離脱も短い、を繰り返す。
        if (gate.update(TouchSample{true, point, t}).has_value()) ++emitted;
        t += config.press_debounce_ms - 5;
        if (gate.update(TouchSample{false, point, t}).has_value()) ++emitted;
        t += 5;
    }
    CHECK_EQ(emitted, 0);
}

TEST(bouncing_release_does_not_emit_a_second_command) {
    const QuadLayout layout = make_layout();
    TouchGate gate(layout);
    const Point point = center_of_increment(layout, 3);

    CHECK_EQ(count_commands(gate, point, true, 0, 100, 10), 1);
    // 離脱確定前に再接触する揺れを繰り返しても追加発行しない。
    int emitted = 0;
    std::uint32_t t = 110;
    for (int i = 0; i < 10; ++i) {
        if (gate.update(TouchSample{false, point, t}).has_value()) ++emitted;
        t += 10;
        if (gate.update(TouchSample{true, point, t}).has_value()) ++emitted;
        t += 10;
    }
    CHECK_EQ(emitted, 0);
}

TEST(drag_within_one_press_emits_only_the_confirmed_point) {
    const QuadLayout layout = make_layout();
    TouchGate gate(layout);
    const Point start = center_of_increment(layout, 0);
    const Point other = center_of_increment(layout, 3);

    std::optional<LifeCommand> command;
    int emitted = 0;
    std::uint32_t t = 0;
    for (; t <= 200; t += 10) {
        const Point point = (t < 100) ? start : other;  // 押したまま別領域へ指をずらす
        const auto result = gate.update(TouchSample{true, point, t});
        if (result.has_value()) {
            command = result;
            ++emitted;
        }
    }
    CHECK_EQ(emitted, 1);
    CHECK_EQ(command->player_id, 0);  // 確定時点の領域のみ
}

TEST(tap_outside_any_button_emits_nothing_and_is_consumed) {
    const QuadLayout layout = make_layout();
    TouchGate gate(layout);
    const auto number = layout.number_area(0);
    const Point dead{number.x + 2, number.y + 2};  // 数値表示部（外周寄り）
    CHECK_FALSE(layout.hit_test(dead).has_value());

    int emitted = 0;
    std::uint32_t t = 0;
    for (; t <= 100; t += 10) {
        if (gate.update(TouchSample{true, dead, t}).has_value()) ++emitted;
    }
    // 同じ押下のまま有効領域へ移動しても発行しない。
    const Point button = center_of_increment(layout, 0);
    for (; t <= 300; t += 10) {
        if (gate.update(TouchSample{true, button, t}).has_value()) ++emitted;
    }
    CHECK_EQ(emitted, 0);
}

TEST(timestamp_wraparound_is_handled) {
    const QuadLayout layout = make_layout();
    TouchGate gate(layout);
    const Point point = center_of_increment(layout, 1);
    const std::uint32_t near_max = std::numeric_limits<std::uint32_t>::max() - 20;

    int emitted = 0;
    std::uint32_t t = near_max;
    for (int i = 0; i < 20; ++i) {
        if (gate.update(TouchSample{true, point, t}).has_value()) ++emitted;
        t += 10;  // 途中で折り返す
    }
    CHECK_EQ(emitted, 1);
}

TEST(gate_works_with_externally_corrected_coordinates) {
    // 回転・補正はデバイス層の責務。ゲートは渡された論理座標だけで動く。
    const QuadLayout layout = make_layout();
    TouchGate gate(layout);
    const auto target = layout.decrement_button(3);
    const Point raw{100, 50};
    // 呼び出し側が任意の変換を適用しても、ゲートの判定は変換後の座標に従う。
    const Point corrected{target.x + target.width / 2, target.y + target.height / 2};
    (void)raw;

    std::optional<LifeCommand> command;
    for (std::uint32_t t = 0; t <= 100; t += 10) {
        const auto result = gate.update(TouchSample{true, corrected, t});
        if (result.has_value()) command = result;
    }
    CHECK_TRUE(command.has_value());
    CHECK_EQ(command->player_id, 3);
    CHECK_TRUE(command->action == LifeAction::Decrement);
}

}  // namespace
