// SPEC.md「UI」/ ACCEPTANCE「UIヒットテスト」に対応。画面寸法は注入する。

#include <vector>

#include "../../src/ui/layout.h"
#include "test_framework.h"

namespace {

using life::app::LifeAction;
using life::domain::kPlayerCount;
using life::ui::LayoutConfig;
using life::ui::Point;
using life::ui::QuadLayout;
using life::ui::Rect;

constexpr int kWidth = 320;
constexpr int kHeight = 240;

QuadLayout make_layout(int width = kWidth, int height = kHeight) {
    return QuadLayout(LayoutConfig::for_screen(width, height));
}

Point center_of(const Rect& rect) {
    return Point{rect.x + rect.width / 2, rect.y + rect.height / 2};
}

bool overlaps(const Rect& a, const Rect& b) {
    return a.x < b.right() && b.x < a.right() && a.y < b.bottom() && b.y < a.bottom();
}

TEST(panels_tile_the_screen_without_gap_or_overlap) {
    const QuadLayout layout = make_layout();
    int covered = 0;
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            int hits = 0;
            for (int id = 0; id < kPlayerCount; ++id) {
                if (layout.panel(id).contains(Point{x, y})) {
                    ++hits;
                }
            }
            if (hits == 1) {
                ++covered;
            }
        }
    }
    CHECK_EQ(covered, kWidth * kHeight);
}

TEST(panel_positions_follow_p1_to_p4_order) {
    const QuadLayout layout = make_layout();
    CHECK_TRUE(layout.panel(0).contains(Point{10, 10}));             // P1 左上
    CHECK_TRUE(layout.panel(1).contains(Point{kWidth - 10, 10}));    // P2 右上
    CHECK_TRUE(layout.panel(2).contains(Point{10, kHeight - 10}));   // P3 左下
    CHECK_TRUE(layout.panel(3).contains(Point{kWidth - 10, kHeight - 10}));  // P4 右下
}

TEST(plus_minus_are_vertically_stacked_on_the_outer_edge) {
    const QuadLayout layout = make_layout();
    for (int id = 0; id < kPlayerCount; ++id) {
        const Rect panel = layout.panel(id);
        const Rect plus = layout.increment_button(id);
        const Rect minus = layout.decrement_button(id);

        CHECK_EQ(plus.x, minus.x);  // 縦に並ぶ
        CHECK_EQ(plus.width, minus.width);
        CHECK_FALSE(overlaps(plus, minus));
        // 2つで帯全体を覆う
        CHECK_EQ(plus.height + minus.height, panel.height);

        const bool right_column = (id % 2) == 1;
        if (right_column) {
            CHECK_EQ(plus.right(), panel.right());  // 画面外側＝右端
        } else {
            CHECK_EQ(plus.x, panel.x);              // 画面外側＝左端
        }
    }
}

TEST(plus_is_on_the_player_side_of_the_strip) {
    // 平置きで対面に座るプレイヤー向けに、上行は画面上の下半分が + になる（2026-10-11 決定）。
    const QuadLayout layout = make_layout();
    for (int id = 0; id < kPlayerCount; ++id) {
        const Rect plus = layout.increment_button(id);
        const Rect minus = layout.decrement_button(id);
        const Rect panel = layout.panel(id);
        const bool upside_down = layout.orientation_of(id) == life::ui::Orientation::UpsideDown;

        if (upside_down) {
            CHECK_TRUE(minus.y < plus.y);           // 画面上では − が上
            CHECK_EQ(minus.y, panel.y);
            CHECK_EQ(plus.bottom(), panel.bottom());
        } else {
            CHECK_TRUE(plus.y < minus.y);           // 画面上では + が上
            CHECK_EQ(plus.y, panel.y);
            CHECK_EQ(minus.bottom(), panel.bottom());
        }
    }
}

TEST(top_row_panels_are_upside_down) {
    const QuadLayout layout = make_layout();
    CHECK_TRUE(layout.orientation_of(0) == life::ui::Orientation::UpsideDown);
    CHECK_TRUE(layout.orientation_of(1) == life::ui::Orientation::UpsideDown);
    CHECK_TRUE(layout.orientation_of(2) == life::ui::Orientation::Normal);
    CHECK_TRUE(layout.orientation_of(3) == life::ui::Orientation::Normal);
    // 無効IDは既定の向きを返し、場合分けを呼び出し側へ漏らさない。
    CHECK_TRUE(layout.orientation_of(-1) == life::ui::Orientation::Normal);
    CHECK_TRUE(layout.orientation_of(kPlayerCount) == life::ui::Orientation::Normal);
}

TEST(hit_test_follows_the_swapped_plus_minus_on_the_top_row) {
    const QuadLayout layout = make_layout();
    for (int id = 0; id < 2; ++id) {
        // 画面上の下半分をタッチすると Increment になる。
        const auto lower = layout.hit_test(center_of(layout.increment_button(id)));
        CHECK_TRUE(lower.has_value());
        CHECK_TRUE(lower->action == LifeAction::Increment);
        CHECK_TRUE(center_of(layout.increment_button(id)).y >
                   center_of(layout.decrement_button(id)).y);
    }
}

TEST(reset_sits_inside_the_value_area_away_from_plus_minus) {
    const QuadLayout layout = make_layout();
    for (int id = 0; id < kPlayerCount; ++id) {
        const Rect reset = layout.reset_button(id);
        const Rect value = layout.value_area(id);
        CHECK_TRUE(reset.width > 0 && reset.height > 0);
        CHECK_TRUE(reset.x >= value.x && reset.right() <= value.right());
        CHECK_TRUE(reset.y >= value.y && reset.bottom() <= value.bottom());
        CHECK_FALSE(overlaps(reset, layout.increment_button(id)));
        CHECK_FALSE(overlaps(reset, layout.decrement_button(id)));
    }
}

TEST(reset_sits_in_the_header_band_toward_the_screen_center) {
    const QuadLayout layout = make_layout();
    for (int id = 0; id < kPlayerCount; ++id) {
        const Rect header = layout.header_area(id);
        const Rect reset = layout.reset_button(id);
        CHECK_EQ(reset.y, header.y);
        CHECK_EQ(reset.height, header.height);
        CHECK_TRUE(reset.x >= header.x && reset.right() <= header.right());
        // ラベルより画面中央側にある。
        const Rect label = layout.label_area(id);
        const bool right_column = (id % 2) == 1;
        if (right_column) {
            CHECK_TRUE(reset.x < label.x);
        } else {
            CHECK_TRUE(reset.x > label.x);
        }
    }
}

TEST(neighbouring_reset_buttons_keep_a_gap) {
    const QuadLayout layout = make_layout();
    // 中央線を挟んで隣り合うリセット同士が接しないこと（誤タップ防止）。
    CHECK_TRUE(layout.reset_button(0).right() < layout.reset_button(1).x);
    CHECK_TRUE(layout.reset_button(2).right() < layout.reset_button(3).x);
}

TEST(header_splits_into_label_and_reset_and_number_takes_the_rest) {
    const QuadLayout layout = make_layout();
    for (int id = 0; id < kPlayerCount; ++id) {
        const Rect value = layout.value_area(id);
        const Rect header = layout.header_area(id);
        const Rect label = layout.label_area(id);
        const Rect number = layout.number_area(id);

        CHECK_EQ(header.width, value.width);
        CHECK_EQ(header.height + number.height, value.height);
        CHECK_EQ(number.width, value.width);
        CHECK_FALSE(overlaps(header, number));
        CHECK_FALSE(overlaps(label, layout.reset_button(id)));
        CHECK_TRUE(number.height > header.height);  // 数値を最大限大きく
        // 外周側（上行は上端、下行は下端）に帯が寄る。
        const bool bottom_row = (id / 2) == 1;
        if (bottom_row) {
            CHECK_EQ(header.bottom(), value.bottom());
        } else {
            CHECK_EQ(header.y, value.y);
        }
    }
}

TEST(number_area_never_overlaps_any_button) {
    const QuadLayout layout = make_layout();
    for (int id = 0; id < kPlayerCount; ++id) {
        const Rect number = layout.number_area(id);
        for (int other = 0; other < kPlayerCount; ++other) {
            CHECK_FALSE(overlaps(number, layout.increment_button(other)));
            CHECK_FALSE(overlaps(number, layout.decrement_button(other)));
            CHECK_FALSE(overlaps(number, layout.reset_button(other)));
        }
    }
}

TEST(hit_test_maps_each_button_to_its_player_and_action) {
    const QuadLayout layout = make_layout();
    for (int id = 0; id < kPlayerCount; ++id) {
        const auto plus = layout.hit_test(center_of(layout.increment_button(id)));
        CHECK_TRUE(plus.has_value());
        CHECK_EQ(plus->player_id, id);
        CHECK_TRUE(plus->action == LifeAction::Increment);

        const auto minus = layout.hit_test(center_of(layout.decrement_button(id)));
        CHECK_TRUE(minus.has_value());
        CHECK_EQ(minus->player_id, id);
        CHECK_TRUE(minus->action == LifeAction::Decrement);

        const auto reset = layout.hit_test(center_of(layout.reset_button(id)));
        CHECK_TRUE(reset.has_value());
        CHECK_EQ(reset->player_id, id);
        CHECK_TRUE(reset->action == LifeAction::Reset);
    }
}

TEST(hit_test_ignores_points_outside_the_screen) {
    const QuadLayout layout = make_layout();
    const Point outside[] = {{-1, 0}, {0, -1}, {kWidth, 0}, {0, kHeight},
                             {-100, -100}, {kWidth + 50, kHeight + 50}};
    for (const Point& point : outside) {
        CHECK_FALSE(layout.hit_test(point).has_value());
    }
}

TEST(hit_test_ignores_the_number_area) {
    const QuadLayout layout = make_layout();
    for (int id = 0; id < kPlayerCount; ++id) {
        const Rect number = layout.number_area(id);
        // 数値表示域はどこを触っても操作にならない。
        const Point points[] = {center_of(number),
                                Point{number.x, number.y},
                                Point{number.right() - 1, number.y},
                                Point{number.x, number.bottom() - 1},
                                Point{number.right() - 1, number.bottom() - 1}};
        for (const Point& point : points) {
            CHECK_FALSE(layout.hit_test(point).has_value());
        }
    }
}

TEST(hit_test_boundaries_belong_to_exactly_one_rect) {
    const QuadLayout layout = make_layout();
    // 画面全域を走査し、返したコマンドが必ずその座標を含む領域と一致すること。
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            const Point point{x, y};
            const auto hit = layout.hit_test(point);
            if (!hit.has_value()) {
                continue;
            }
            const int id = hit->player_id;
            CHECK_TRUE(layout.panel(id).contains(point));
            switch (hit->action) {
                case LifeAction::Increment:
                    CHECK_TRUE(layout.increment_button(id).contains(point));
                    break;
                case LifeAction::Decrement:
                    CHECK_TRUE(layout.decrement_button(id).contains(point));
                    break;
                case LifeAction::Reset:
                    CHECK_TRUE(layout.reset_button(id).contains(point));
                    break;
            }
        }
    }
}

TEST(buttons_of_different_players_never_overlap) {
    const QuadLayout layout = make_layout();
    std::vector<Rect> rects;
    for (int id = 0; id < kPlayerCount; ++id) {
        rects.push_back(layout.increment_button(id));
        rects.push_back(layout.decrement_button(id));
        rects.push_back(layout.reset_button(id));
    }
    for (std::size_t i = 0; i < rects.size(); ++i) {
        for (std::size_t j = i + 1; j < rects.size(); ++j) {
            CHECK_FALSE(overlaps(rects[i], rects[j]));
        }
    }
}

TEST(layout_stays_valid_on_unusual_screen_sizes) {
    const int sizes[][2] = {{320, 240}, {240, 320}, {480, 320}, {64, 48}, {33, 25}};
    for (const auto& size : sizes) {
        const QuadLayout layout = make_layout(size[0], size[1]);
        for (int id = 0; id < kPlayerCount; ++id) {
            const Rect panel = layout.panel(id);
            CHECK_TRUE(panel.width > 0 && panel.height > 0);
            for (const Rect& rect : {layout.increment_button(id), layout.decrement_button(id),
                                     layout.reset_button(id), layout.number_area(id),
                                     layout.label_area(id)}) {
                CHECK_TRUE(rect.width > 0 && rect.height > 0);
                CHECK_TRUE(rect.x >= panel.x && rect.right() <= panel.right());
                CHECK_TRUE(rect.y >= panel.y && rect.bottom() <= panel.bottom());
            }
        }
    }
}

TEST(invalid_player_id_returns_empty_rects) {
    const QuadLayout layout = make_layout();
    for (int id : {-1, kPlayerCount, 1000}) {
        CHECK_EQ(layout.panel(id).width, 0);
        CHECK_EQ(layout.increment_button(id).width, 0);
        CHECK_EQ(layout.decrement_button(id).width, 0);
        CHECK_EQ(layout.reset_button(id).width, 0);
        CHECK_EQ(layout.number_area(id).width, 0);
        CHECK_FALSE(layout.panel(id).contains(Point{0, 0}));
    }
}

}  // namespace
