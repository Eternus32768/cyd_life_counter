// 入力→状態→表示の一連の流れ。実機なしで操作シナリオを再現する。

#include <string>
#include <vector>

#include "../../src/ui/counter_screen.h"
#include "test_framework.h"

namespace {

using life::app::LifeEngine;
using life::domain::kPlayerCount;
using life::ui::Color;
using life::ui::CounterScreen;
using life::ui::DisplayRenderer;
using life::ui::LayoutConfig;
using life::ui::Point;
using life::ui::Rect;
using life::ui::TextRole;
using life::ui::TouchGateConfig;
using life::ui::TouchSample;

struct FillCall {
    Rect rect;
    Color color;
};

class RecordingRenderer : public DisplayRenderer {
public:
    void fill_rect(const Rect& rect, const Color& color) override {
        fills.push_back(FillCall{rect, color});
    }
    void draw_border(const Rect&, const Color&) override {}
    void draw_text(const Rect&, const char* text, TextRole role, const Color&,
                   life::ui::Orientation) override {
        if (role == TextRole::Value) {
            values.push_back(text != nullptr ? text : "");
        }
    }

    // 指定矩形を最後に塗った色。描かれていなければ false。
    bool last_fill_color(const Rect& rect, Color* out) const {
        for (auto it = fills.rbegin(); it != fills.rend(); ++it) {
            if (it->rect.x == rect.x && it->rect.y == rect.y && it->rect.width == rect.width &&
                it->rect.height == rect.height) {
                *out = it->color;
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> values;
    std::vector<FillCall> fills;
};

bool same_color(const Color& a, const Color& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

struct Fixture {
    LifeEngine engine;
    RecordingRenderer renderer;
    // 既定値に依存しないよう猶予を明示する。
    CounterScreen screen{engine,
                         renderer,
                         LayoutConfig::for_screen(320, 240),
                         life::ui::TouchGateConfig{},
                         life::ui::Palette::defaults(),
                         life::ui::ResetConfirmationConfig{1000}};
    std::uint32_t now_ms = 0;

    // 指を置いて離す1タップ。デバウンス時間を十分に跨ぐ。
    // 1タップ約300msで、リセットのダブルタップ猶予(500ms)内に2回続けられる長さにしている。
    void tap(const Point& point) {
        const TouchGateConfig config{};
        for (int i = 0; i < 3; ++i) {
            screen.handle_touch(TouchSample{true, point, now_ms});
            now_ms += config.press_debounce_ms;
        }
        for (int i = 0; i < 3; ++i) {
            screen.handle_touch(TouchSample{false, point, now_ms});
            now_ms += config.release_debounce_ms;
        }
    }

    // 指を触れずに時間だけ進める（猶予切れの確認用）。
    void wait(std::uint32_t duration_ms) {
        const std::uint32_t until = now_ms + duration_ms;
        while (now_ms < until) {
            screen.handle_touch(TouchSample{false, Point{0, 0}, now_ms});
            now_ms += 20;
        }
    }

    Point center(const Rect& rect) const {
        return Point{rect.x + rect.width / 2, rect.y + rect.height / 2};
    }
};

TEST(initial_draw_shows_zero_for_everyone) {
    Fixture fixture;
    fixture.screen.draw_initial();
    CHECK_EQ(fixture.renderer.values.size(), static_cast<std::size_t>(kPlayerCount));
    for (const std::string& value : fixture.renderer.values) {
        CHECK_STREQ(value.c_str(), "0");
    }
}

TEST(tap_sequence_updates_only_the_tapped_player) {
    Fixture fixture;
    const auto& layout = fixture.screen.layout();

    fixture.tap(fixture.center(layout.increment_button(1)));
    fixture.tap(fixture.center(layout.increment_button(1)));
    fixture.tap(fixture.center(layout.decrement_button(2)));

    CHECK_EQ(fixture.engine.snapshot().value_of(0), 0);
    CHECK_EQ(fixture.engine.snapshot().value_of(1), 2);
    CHECK_EQ(fixture.engine.snapshot().value_of(2), -1);
    CHECK_EQ(fixture.engine.snapshot().value_of(3), 0);

    // 操作ごとに1回だけ該当プレイヤーを再描画する。
    CHECK_EQ(fixture.renderer.values.size(), static_cast<std::size_t>(3));
    CHECK_STREQ(fixture.renderer.values[0].c_str(), "1");
    CHECK_STREQ(fixture.renderer.values[1].c_str(), "2");
    CHECK_STREQ(fixture.renderer.values[2].c_str(), "-1");
}

TEST(single_reset_tap_does_not_change_the_value) {
    // 誤操作防止。1回目は待機するだけで値は動かさない（2026-10-11 決定）。
    Fixture fixture;
    const auto& layout = fixture.screen.layout();
    fixture.tap(fixture.center(layout.increment_button(0)));
    fixture.tap(fixture.center(layout.reset_button(0)));

    CHECK_EQ(fixture.engine.snapshot().value_of(0), 1);
    CHECK_EQ(fixture.screen.pending_reset_player(), 0);  // 待機中は表示で示す
}

TEST(double_reset_tap_affects_only_that_player) {
    Fixture fixture;
    const auto& layout = fixture.screen.layout();
    fixture.tap(fixture.center(layout.increment_button(0)));
    fixture.tap(fixture.center(layout.increment_button(3)));

    fixture.tap(fixture.center(layout.reset_button(0)));
    fixture.tap(fixture.center(layout.reset_button(0)));

    CHECK_EQ(fixture.engine.snapshot().value_of(0), 0);
    CHECK_EQ(fixture.engine.snapshot().value_of(3), 1);
    CHECK_EQ(fixture.screen.pending_reset_player(), life::ui::ResetConfirmation::kNoPending);
}

TEST(reset_needs_the_second_tap_within_the_window) {
    Fixture fixture;
    const auto& layout = fixture.screen.layout();
    fixture.tap(fixture.center(layout.increment_button(1)));
    fixture.tap(fixture.center(layout.reset_button(1)));
    fixture.wait(1100);  // 猶予切れ
    CHECK_EQ(fixture.screen.pending_reset_player(), life::ui::ResetConfirmation::kNoPending);

    fixture.tap(fixture.center(layout.reset_button(1)));  // これは再び1回目になる
    CHECK_EQ(fixture.engine.snapshot().value_of(1), 1);
    CHECK_EQ(fixture.screen.pending_reset_player(), 1);
}

TEST(other_touches_cancel_a_pending_reset) {
    Fixture fixture;
    const auto& layout = fixture.screen.layout();
    fixture.tap(fixture.center(layout.increment_button(2)));
    fixture.tap(fixture.center(layout.reset_button(2)));
    fixture.tap(fixture.center(layout.increment_button(2)));  // 別操作で待機解除
    CHECK_EQ(fixture.screen.pending_reset_player(), life::ui::ResetConfirmation::kNoPending);

    fixture.tap(fixture.center(layout.reset_button(2)));
    CHECK_EQ(fixture.engine.snapshot().value_of(2), 2);  // リセットされていない
}

TEST(holding_a_button_updates_once) {
    Fixture fixture;
    const auto& layout = fixture.screen.layout();
    const Point point = fixture.center(layout.increment_button(0));
    int redraws = 0;
    for (std::uint32_t t = 0; t < 3000; t += 10) {
        if (fixture.screen.handle_touch(TouchSample{true, point, t})) {
            ++redraws;
        }
    }
    CHECK_EQ(redraws, 1);
    CHECK_EQ(fixture.engine.snapshot().value_of(0), 1);
}

TEST(touch_on_the_number_area_changes_nothing) {
    Fixture fixture;
    const auto& layout = fixture.screen.layout();
    const Rect number = layout.number_area(1);
    fixture.tap(Point{number.right() - 2, number.y + 2});

    for (int id = 0; id < kPlayerCount; ++id) {
        CHECK_EQ(fixture.engine.snapshot().value_of(id), 0);
    }
    CHECK_TRUE(fixture.renderer.values.empty());
}

// Codex指摘 R09 の対応。待機表示が欠落してもテストが通る状態だった。
TEST(reset_button_shows_and_clears_the_armed_color) {
    Fixture fixture;
    const auto& layout = fixture.screen.layout();
    const life::ui::Palette palette = life::ui::Palette::defaults();
    const Rect reset = layout.reset_button(0);
    Color color{};

    fixture.tap(fixture.center(reset));  // 1回目: 待機色になる
    CHECK_TRUE(fixture.renderer.last_fill_color(reset, &color));
    CHECK_TRUE(same_color(color, palette.reset_armed));

    fixture.wait(1100);  // 猶予切れで通常色に戻る
    CHECK_TRUE(fixture.renderer.last_fill_color(reset, &color));
    CHECK_TRUE(same_color(color, palette.reset));
}

TEST(confirming_the_reset_leaves_the_button_in_the_normal_color) {
    Fixture fixture;
    const auto& layout = fixture.screen.layout();
    const life::ui::Palette palette = life::ui::Palette::defaults();
    const Rect reset = layout.reset_button(2);
    Color color{};

    fixture.tap(fixture.center(layout.increment_button(2)));
    fixture.tap(fixture.center(reset));
    fixture.tap(fixture.center(reset));  // 2回目で確定

    CHECK_EQ(fixture.engine.snapshot().value_of(2), 0);
    CHECK_TRUE(fixture.renderer.last_fill_color(reset, &color));
    CHECK_TRUE(same_color(color, palette.reset));
}

TEST(arming_another_player_clears_the_previous_armed_color) {
    Fixture fixture;
    const auto& layout = fixture.screen.layout();
    const life::ui::Palette palette = life::ui::Palette::defaults();
    const Rect first = layout.reset_button(1);
    const Rect second = layout.reset_button(3);
    Color color{};

    fixture.tap(fixture.center(first));
    fixture.tap(fixture.center(second));

    CHECK_TRUE(fixture.renderer.last_fill_color(first, &color));
    CHECK_TRUE(same_color(color, palette.reset));  // 前の待機は解除される
    CHECK_TRUE(fixture.renderer.last_fill_color(second, &color));
    CHECK_TRUE(same_color(color, palette.reset_armed));
    CHECK_EQ(fixture.screen.pending_reset_player(), 3);
}

TEST(other_action_clears_the_armed_color) {
    Fixture fixture;
    const auto& layout = fixture.screen.layout();
    const life::ui::Palette palette = life::ui::Palette::defaults();
    const Rect reset = layout.reset_button(0);
    Color color{};

    fixture.tap(fixture.center(reset));
    fixture.tap(fixture.center(layout.increment_button(1)));  // 他プレイヤーの操作で解除

    CHECK_TRUE(fixture.renderer.last_fill_color(reset, &color));
    CHECK_TRUE(same_color(color, palette.reset));
    CHECK_EQ(fixture.screen.pending_reset_player(), life::ui::ResetConfirmation::kNoPending);
}

}  // namespace
