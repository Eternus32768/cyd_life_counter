#include "layout.h"

namespace life::ui {
namespace {

constexpr Rect kEmptyRect{0, 0, 0, 0};

int clamp_int(int value, int low, int high) {
    if (high < low) {
        return low;
    }
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

}  // namespace

LayoutConfig LayoutConfig::for_screen(int screen_width, int screen_height) {
    const int panel_width = screen_width / 2;
    const int panel_height = screen_height / 2;
    const int shorter_side = panel_width < panel_height ? panel_width : panel_height;

    LayoutConfig config{};
    config.screen_width = screen_width;
    config.screen_height = screen_height;
    // ± 帯は領域幅の35%。数値表示域を必ず残す。
    config.button_strip_width = clamp_int(panel_width * 35 / 100, 1, panel_width - 1);
    config.header_height = clamp_int(panel_height * 26 / 100, 1, panel_height - 1);
    config.reset_width = clamp_int(shorter_side * 30 / 100, 1, shorter_side - 1);
    config.reset_gap = clamp_int(shorter_side / 30, 0, shorter_side / 4);
    return config;
}

QuadLayout::QuadLayout(const LayoutConfig& config) : config_(config) {}

Rect QuadLayout::panel(int player_id) const {
    if (!domain::LifeState::is_valid_player(player_id)) {
        return kEmptyRect;
    }
    // 奇数解像度でも隙間が出ないよう、右列・下行が余りを受け持つ。
    const int left_width = config_.screen_width / 2;
    const int top_height = config_.screen_height / 2;

    Rect rect{};
    if (is_right_column(player_id)) {
        rect.x = left_width;
        rect.width = config_.screen_width - left_width;
    } else {
        rect.x = 0;
        rect.width = left_width;
    }
    if (is_bottom_row(player_id)) {
        rect.y = top_height;
        rect.height = config_.screen_height - top_height;
    } else {
        rect.y = 0;
        rect.height = top_height;
    }
    return rect;
}

Rect QuadLayout::button_strip(int player_id) const {
    const Rect area = panel(player_id);
    if (area.width == 0) {
        return kEmptyRect;
    }
    const int strip_width = clamp_int(config_.button_strip_width, 1, area.width);
    // 画面外側（左列は左端、右列は右端）に沿わせる。
    const int x = is_right_column(player_id) ? area.right() - strip_width : area.x;
    return Rect{x, area.y, strip_width, area.height};
}

Orientation QuadLayout::orientation_of(int player_id) const {
    if (!domain::LifeState::is_valid_player(player_id)) {
        return Orientation::Normal;
    }
    // 上行のプレイヤーは画面の反対側に座るため、その領域だけ180°回転して読む。
    return is_bottom_row(player_id) ? Orientation::Normal : Orientation::UpsideDown;
}

Rect QuadLayout::strip_upper_half(int player_id) const {
    const Rect strip = button_strip(player_id);
    if (strip.width == 0) {
        return kEmptyRect;
    }
    return Rect{strip.x, strip.y, strip.width, strip.height / 2};
}

Rect QuadLayout::strip_lower_half(int player_id) const {
    const Rect strip = button_strip(player_id);
    if (strip.width == 0) {
        return kEmptyRect;
    }
    const int upper_height = strip.height / 2;
    return Rect{strip.x, strip.y + upper_height, strip.width, strip.height - upper_height};
}

Rect QuadLayout::increment_button(int player_id) const {
    // 本人の視点で上が + になるように置く。180°回転の領域では画面上の下半分が + になる。
    return orientation_of(player_id) == Orientation::UpsideDown ? strip_lower_half(player_id)
                                                                : strip_upper_half(player_id);
}

Rect QuadLayout::decrement_button(int player_id) const {
    return orientation_of(player_id) == Orientation::UpsideDown ? strip_upper_half(player_id)
                                                                : strip_lower_half(player_id);
}

Rect QuadLayout::value_area(int player_id) const {
    const Rect area = panel(player_id);
    if (area.width == 0) {
        return kEmptyRect;
    }
    const Rect strip = button_strip(player_id);
    const int width = area.width - strip.width;
    const int x = is_right_column(player_id) ? area.x : area.x + strip.width;
    return Rect{x, area.y, width, area.height};
}

Rect QuadLayout::header_area(int player_id) const {
    const Rect value = value_area(player_id);
    if (value.width == 0) {
        return kEmptyRect;
    }
    // ラベルとリセットは画面外周側（上行は上端、下行は下端）の帯に並べる。
    // 数値はこの帯を除いた残り全部を使うため、他の操作領域と重ならない。
    const int height = clamp_int(config_.header_height, 1, value.height - 1);
    const int y = is_bottom_row(player_id) ? value.bottom() - height : value.y;
    return Rect{value.x, y, value.width, height};
}

Rect QuadLayout::reset_button(int player_id) const {
    const Rect header = header_area(player_id);
    if (header.width == 0) {
        return kEmptyRect;
    }
    // ± とは反対側（画面中央寄り）に置き、増減操作の指の軌道から離す。
    // 隣のプレイヤーのリセットとの誤タップを防ぐため、中央側に間隔を空ける。
    const int width = clamp_int(config_.reset_width, 1, header.width - 1);
    const int gap = clamp_int(config_.reset_gap, 0, header.width - width);
    const int x = is_right_column(player_id) ? header.x + gap : header.right() - gap - width;
    return Rect{x, header.y, width, header.height};
}

Rect QuadLayout::label_area(int player_id) const {
    const Rect header = header_area(player_id);
    if (header.width == 0) {
        return kEmptyRect;
    }
    const Rect reset = reset_button(player_id);
    // 帯のうちリセットより画面外側の部分。
    const int x = is_right_column(player_id) ? reset.right() : header.x;
    const int width = is_right_column(player_id) ? header.right() - reset.right()
                                                 : reset.x - header.x;
    return Rect{x, header.y, width < 1 ? 1 : width, header.height};
}

Rect QuadLayout::number_area(int player_id) const {
    const Rect value = value_area(player_id);
    if (value.width == 0) {
        return kEmptyRect;
    }
    const Rect header = header_area(player_id);
    const int height = value.height - header.height;
    const int y = is_bottom_row(player_id) ? value.y : value.y + header.height;
    return Rect{value.x, y, value.width, height};
}

std::optional<app::LifeCommand> QuadLayout::hit_test(const Point& point) const {
    if (point.x < 0 || point.y < 0 || point.x >= config_.screen_width ||
        point.y >= config_.screen_height) {
        return std::nullopt;
    }
    for (int player_id = 0; player_id < domain::kPlayerCount; ++player_id) {
        if (!panel(player_id).contains(point)) {
            continue;
        }
        if (reset_button(player_id).contains(point)) {
            return app::LifeCommand{player_id, app::LifeAction::Reset};
        }
        if (increment_button(player_id).contains(point)) {
            return app::LifeCommand{player_id, app::LifeAction::Increment};
        }
        if (decrement_button(player_id).contains(point)) {
            return app::LifeCommand{player_id, app::LifeAction::Decrement};
        }
        return std::nullopt;  // 領域内だが操作対象外（数値表示部・ラベル）
    }
    return std::nullopt;
}

}  // namespace life::ui
