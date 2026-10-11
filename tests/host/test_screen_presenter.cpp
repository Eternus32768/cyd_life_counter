// 表示は状態のスナップショットから生成する（SPEC.md「入力・表示」）。

#include <limits>
#include <string>
#include <vector>

#include "../../src/application/life_engine.h"
#include "../../src/ui/screen_presenter.h"
#include "test_framework.h"

namespace {

using life::app::LifeAction;
using life::app::LifeCommand;
using life::app::LifeEngine;
using life::domain::kPlayerCount;
using life::ui::Color;
using life::ui::DisplayRenderer;
using life::ui::LayoutConfig;
using life::ui::QuadLayout;
using life::ui::Rect;
using life::ui::ScreenPresenter;
using life::ui::TextRole;

struct TextCall {
    Rect rect;
    std::string text;
    TextRole role;
    life::ui::Orientation orientation;
};

class FakeRenderer : public DisplayRenderer {
public:
    void begin_frame() override { ++begin_count; }
    void end_frame() override { ++end_count; }
    void fill_rect(const Rect& rect, const Color&) override { filled.push_back(rect); }
    void draw_border(const Rect& rect, const Color&) override { bordered.push_back(rect); }
    void draw_text(const Rect& rect, const char* text, TextRole role, const Color&,
                   life::ui::Orientation orientation) override {
        texts.push_back(TextCall{rect, text != nullptr ? text : "", role, orientation});
    }

    std::vector<std::string> texts_with_role(TextRole role) const {
        std::vector<std::string> result;
        for (const TextCall& call : texts) {
            if (call.role == role) {
                result.push_back(call.text);
            }
        }
        return result;
    }

    int begin_count = 0;
    int end_count = 0;
    std::vector<Rect> filled;
    std::vector<Rect> bordered;
    std::vector<TextCall> texts;
};

QuadLayout make_layout() { return QuadLayout(LayoutConfig::for_screen(320, 240)); }

TEST(format_value_prints_sign_and_bounds) {
    char buffer[ScreenPresenter::kValueBufferSize];
    ScreenPresenter::format_value(0, buffer, sizeof(buffer));
    CHECK_STREQ(buffer, "0");
    ScreenPresenter::format_value(-1, buffer, sizeof(buffer));
    CHECK_STREQ(buffer, "-1");
    ScreenPresenter::format_value(42, buffer, sizeof(buffer));
    CHECK_STREQ(buffer, "42");
    ScreenPresenter::format_value(std::numeric_limits<std::int32_t>::max(), buffer, sizeof(buffer));
    CHECK_STREQ(buffer, "2147483647");
    ScreenPresenter::format_value(std::numeric_limits<std::int32_t>::min(), buffer, sizeof(buffer));
    CHECK_STREQ(buffer, "-2147483648");
}

TEST(draw_all_renders_four_labels_and_values) {
    const QuadLayout layout = make_layout();
    FakeRenderer renderer;
    ScreenPresenter presenter(layout, renderer);
    LifeEngine engine;
    engine.apply(LifeCommand{1, LifeAction::Increment});
    engine.apply(LifeCommand{2, LifeAction::Decrement});

    presenter.draw_all(engine.snapshot());

    CHECK_EQ(renderer.begin_count, 1);
    CHECK_EQ(renderer.end_count, 1);

    const std::vector<std::string> labels = renderer.texts_with_role(TextRole::PlayerLabel);
    CHECK_EQ(labels.size(), static_cast<std::size_t>(kPlayerCount));
    CHECK_STREQ(labels[0].c_str(), "P1");
    CHECK_STREQ(labels[3].c_str(), "P4");

    const std::vector<std::string> values = renderer.texts_with_role(TextRole::Value);
    CHECK_EQ(values.size(), static_cast<std::size_t>(kPlayerCount));
    CHECK_STREQ(values[0].c_str(), "0");
    CHECK_STREQ(values[1].c_str(), "1");
    CHECK_STREQ(values[2].c_str(), "-1");
    CHECK_STREQ(values[3].c_str(), "0");

    // ± とリセットの記号が各プレイヤー分描かれる。
    CHECK_EQ(renderer.texts_with_role(TextRole::Symbol).size(),
             static_cast<std::size_t>(kPlayerCount * 3));
}

TEST(draw_player_touches_only_that_panel) {
    const QuadLayout layout = make_layout();
    FakeRenderer renderer;
    ScreenPresenter presenter(layout, renderer);
    LifeEngine engine;
    engine.apply(LifeCommand{2, LifeAction::Increment});

    presenter.draw_player(engine.snapshot(), 2);

    const Rect panel = layout.panel(2);
    CHECK_TRUE(!renderer.filled.empty());
    for (const Rect& rect : renderer.filled) {
        CHECK_TRUE(rect.x >= panel.x && rect.right() <= panel.right());
        CHECK_TRUE(rect.y >= panel.y && rect.bottom() <= panel.bottom());
    }
    const std::vector<std::string> values = renderer.texts_with_role(TextRole::Value);
    CHECK_EQ(values.size(), static_cast<std::size_t>(1));
    CHECK_STREQ(values[0].c_str(), "1");
}

TEST(draw_player_ignores_invalid_player_id) {
    const QuadLayout layout = make_layout();
    FakeRenderer renderer;
    ScreenPresenter presenter(layout, renderer);
    LifeEngine engine;

    presenter.draw_player(engine.snapshot(), -1);
    presenter.draw_player(engine.snapshot(), kPlayerCount);

    CHECK_EQ(renderer.begin_count, 0);
    CHECK_TRUE(renderer.filled.empty());
    CHECK_TRUE(renderer.texts.empty());
}

TEST(top_row_is_drawn_upside_down_and_bottom_row_normally) {
    // 平置きで対面に座るプレイヤー向け（SPECの4方向視認の決定、2026-10-11）。
    const QuadLayout layout = make_layout();
    FakeRenderer renderer;
    ScreenPresenter presenter(layout, renderer);
    LifeEngine engine;

    for (int id = 0; id < kPlayerCount; ++id) {
        renderer.texts.clear();
        presenter.draw_player(engine.snapshot(), id);
        CHECK_TRUE(!renderer.texts.empty());
        const life::ui::Orientation expected =
            (id < 2) ? life::ui::Orientation::UpsideDown : life::ui::Orientation::Normal;
        for (const TextCall& call : renderer.texts) {
            CHECK_TRUE(call.orientation == expected);
        }
    }
}

TEST(values_are_drawn_in_the_number_area) {
    const QuadLayout layout = make_layout();
    FakeRenderer renderer;
    ScreenPresenter presenter(layout, renderer);
    LifeEngine engine;

    presenter.draw_all(engine.snapshot());

    int matched = 0;
    for (const TextCall& call : renderer.texts) {
        if (call.role != TextRole::Value) {
            continue;
        }
        for (int id = 0; id < kPlayerCount; ++id) {
            const Rect number = layout.number_area(id);
            if (call.rect.x == number.x && call.rect.y == number.y &&
                call.rect.width == number.width && call.rect.height == number.height) {
                ++matched;
            }
        }
    }
    CHECK_EQ(matched, kPlayerCount);
}

}  // namespace
