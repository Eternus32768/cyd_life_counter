// Codex指摘 R01 の再発防止。描画される文字が操作領域へはみ出さないことを検査する。
// GlyphMetrics は src/platform/cyd/cyd_renderer.cpp の内蔵フォント実寸と同じ値を使う。

#include <limits>
#include <string>

#include "../../src/ui/layout.h"
#include "../../src/ui/screen_presenter.h"
#include "../../src/ui/text_metrics.h"
#include "test_framework.h"

namespace {

using life::domain::kPlayerCount;
using life::ui::GlyphMetrics;
using life::ui::LayoutConfig;
using life::ui::QuadLayout;
using life::ui::Rect;
using life::ui::ScreenPresenter;
using life::ui::TextRole;

constexpr GlyphMetrics kFont0{6, 8, 5, 7, 4};

QuadLayout make_layout(int width = 320, int height = 240) {
    return QuadLayout(LayoutConfig::for_screen(width, height));
}

bool overlaps(const Rect& a, const Rect& b) {
    return a.x < b.right() && b.x < a.right() && a.y < b.bottom() && b.y < a.bottom();
}

bool contains_rect(const Rect& outer, const Rect& inner) {
    return inner.x >= outer.x && inner.right() <= outer.right() && inner.y >= outer.y &&
           inner.bottom() <= outer.bottom();
}

Rect bounds_for(const char* text, const Rect& area, TextRole role,
                life::ui::Orientation orientation = life::ui::Orientation::Normal) {
    const int length = static_cast<int>(std::string(text).size());
    const int scale = life::ui::fit_text_scale(length, area, kFont0, life::ui::max_text_scale(role));
    return life::ui::text_bounds(length, area, kFont0, scale, orientation);
}

TEST(value_text_stays_inside_the_number_area) {
    const QuadLayout layout = make_layout();
    const std::int32_t values[] = {0,
                                   10,
                                   -1,
                                   -10,
                                   999,
                                   -999,
                                   std::numeric_limits<std::int32_t>::max(),
                                   std::numeric_limits<std::int32_t>::min()};
    for (int id = 0; id < kPlayerCount; ++id) {
        const Rect number = layout.number_area(id);
        for (std::int32_t value : values) {
            char text[ScreenPresenter::kValueBufferSize];
            ScreenPresenter::format_value(value, text, sizeof(text));
            const Rect bounds = bounds_for(text, number, TextRole::Value);
            CHECK_TRUE(contains_rect(number, bounds));
            for (int other = 0; other < kPlayerCount; ++other) {
                CHECK_FALSE(overlaps(bounds, layout.reset_button(other)));
                CHECK_FALSE(overlaps(bounds, layout.increment_button(other)));
                CHECK_FALSE(overlaps(bounds, layout.decrement_button(other)));
                CHECK_FALSE(overlaps(bounds, layout.label_area(other)));
            }
        }
    }
}

TEST(label_and_symbol_text_stay_inside_their_rects) {
    const QuadLayout layout = make_layout();
    for (int id = 0; id < kPlayerCount; ++id) {
        const Rect label = layout.label_area(id);
        CHECK_TRUE(contains_rect(label, bounds_for("P1", label, TextRole::PlayerLabel)));
        CHECK_TRUE(contains_rect(layout.increment_button(id),
                                 bounds_for("+", layout.increment_button(id), TextRole::Symbol)));
        CHECK_TRUE(contains_rect(layout.decrement_button(id),
                                 bounds_for("-", layout.decrement_button(id), TextRole::Symbol)));
        CHECK_TRUE(contains_rect(layout.reset_button(id),
                                 bounds_for("R", layout.reset_button(id), TextRole::Symbol)));
    }
}

TEST(value_text_remains_drawable_on_small_screens) {
    // 小さい画面でも倍率1で描画し、文字を消さない（潰れても表示を落とさない方針）。
    const QuadLayout layout = make_layout(64, 48);
    for (int id = 0; id < kPlayerCount; ++id) {
        const Rect number = layout.number_area(id);
        const Rect bounds = bounds_for("-2147483648", number, TextRole::Value);
        CHECK_TRUE(bounds.width > 0 && bounds.height > 0);
    }
}

TEST(longer_text_never_gets_a_larger_scale) {
    const QuadLayout layout = make_layout();
    const Rect number = layout.number_area(0);
    int previous = std::numeric_limits<int>::max();
    for (int length = 1; length <= 11; ++length) {
        const int scale =
            life::ui::fit_text_scale(length, number, kFont0, life::ui::max_text_scale(TextRole::Value));
        CHECK_TRUE(scale >= 1);
        CHECK_TRUE(scale <= previous);
        previous = scale;
    }
}

// 平置き運用では上行と下行が鏡像になる。中央寄せの端数を同じ向きに丸めると1ピクセルずれるため、
// 「各プレイヤー自身の上端からの距離」が鏡像の相手と一致することを検査する（2026-10-11 実機指摘）。
TEST(text_is_mirror_symmetric_between_top_and_bottom_rows) {
    const QuadLayout layout = make_layout();
    const char* values[] = {"0", "-1", "10", "-2147483648"};

    // 同じ列の上下ペア: P1(0)↔P3(2)、P2(1)↔P4(3)
    const int pairs[2][2] = {{0, 2}, {1, 3}};
    for (const auto& pair : pairs) {
        const int top = pair[0];
        const int bottom = pair[1];
        CHECK_TRUE(layout.orientation_of(top) == life::ui::Orientation::UpsideDown);
        CHECK_TRUE(layout.orientation_of(bottom) == life::ui::Orientation::Normal);

        for (const char* value : values) {
            const Rect top_area = layout.number_area(top);
            const Rect bottom_area = layout.number_area(bottom);
            const Rect top_box =
                bounds_for(value, top_area, TextRole::Value, layout.orientation_of(top));
            const Rect bottom_box = bounds_for(value, bottom_area, TextRole::Value,
                                               layout.orientation_of(bottom));

            // 上行は自分の上端が画面下側にあるため、領域下端からの距離を測る。
            const int top_gap = top_area.bottom() - top_box.bottom();
            const int bottom_gap = bottom_box.y - bottom_area.y;
            CHECK_EQ(top_gap, bottom_gap);
            CHECK_EQ(top_box.height, bottom_box.height);
            CHECK_EQ(top_box.width, bottom_box.width);
        }
    }
}

TEST(mirrored_boxes_land_on_mirrored_screen_positions) {
    // 画面中心線で折り返したとき、上下の文字矩形がぴったり重なること。
    const QuadLayout layout = make_layout();
    const int screen_height = layout.config().screen_height;
    const int pairs[2][2] = {{0, 2}, {1, 3}};
    for (const auto& pair : pairs) {
        const Rect top_box = bounds_for("0", layout.number_area(pair[0]), TextRole::Value,
                                        layout.orientation_of(pair[0]));
        const Rect bottom_box = bounds_for("0", layout.number_area(pair[1]), TextRole::Value,
                                           layout.orientation_of(pair[1]));
        CHECK_EQ(screen_height - (top_box.y + top_box.height), bottom_box.y);
        CHECK_EQ(top_box.x, bottom_box.x);
    }
}

// 実機指摘（2026-10-11）: 平置き対応で上段を180°回転した結果、P1 と P3 の数値が
// フォント1ドット分ずれて一直線に並ばなかった。内蔵フォントのセル内余白が回転で反対側へ移るため。
TEST(numbers_in_the_same_column_share_the_same_horizontal_position) {
    const QuadLayout layout = make_layout();
    const char* values[] = {"0", "-1", "123"};
    const int pairs[2][2] = {{0, 2}, {1, 3}};
    for (const auto& pair : pairs) {
        for (const char* value : values) {
            const Rect top = bounds_for(value, layout.number_area(pair[0]), TextRole::Value,
                                        layout.orientation_of(pair[0]));
            const Rect bottom = bounds_for(value, layout.number_area(pair[1]), TextRole::Value,
                                           layout.orientation_of(pair[1]));
            CHECK_EQ(top.x, bottom.x);          // 縦に一直線
            CHECK_EQ(top.width, bottom.width);
        }
    }
}

TEST(glyph_cells_including_blank_margins_stay_inside_the_area) {
    // 描画側はインク矩形の原点からセルを描く。セルの余白分まで領域内に収まること。
    const int sizes[][2] = {{320, 240}, {64, 48}, {33, 25}};
    for (const auto& size : sizes) {
        const QuadLayout layout = make_layout(size[0], size[1]);
        for (int id = 0; id < kPlayerCount; ++id) {
            const Rect area = layout.number_area(id);
            const life::ui::Orientation orientation = layout.orientation_of(id);
            const Rect ink = bounds_for("-10", area, TextRole::Value, orientation);
            const int scale =
                life::ui::fit_text_scale(3, area, kFont0, life::ui::max_text_scale(TextRole::Value));
            const int blank_x = (kFont0.width - kFont0.ink_width) * scale;
            const int blank_y = (kFont0.height - kFont0.ink_height) * scale;
            if (ink.width + blank_x > area.width || ink.height + blank_y > area.height) {
                continue;  // 領域より文字が大きい場合は潰れても描画を落とさない方針
            }
            // 通常の向きは余白が右下、180°回転では左上に出る。
            const Rect cell =
                (orientation == life::ui::Orientation::UpsideDown)
                    ? Rect{ink.x - blank_x, ink.y - blank_y, ink.width + blank_x,
                           ink.height + blank_y}
                    : Rect{ink.x, ink.y, ink.width + blank_x, ink.height + blank_y};
            CHECK_TRUE(contains_rect(area, cell));
        }
    }
}

}  // namespace
