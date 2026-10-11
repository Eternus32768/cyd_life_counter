#include "screen_presenter.h"

#include <cstdio>

namespace life::ui {
namespace {

// 表示記号。実機のフォントに依存しないASCIIのみを使う。
constexpr const char* kPlayerLabels[domain::kPlayerCount] = {"P1", "P2", "P3", "P4"};
constexpr const char* kIncrementSymbol = "+";
constexpr const char* kDecrementSymbol = "-";
constexpr const char* kResetSymbol = "R";

}  // namespace

Palette Palette::defaults() {
    // 2026-10-11 実機確認: 暗色だと4人の領域を識別しづらかったため中間調へ変更。
    // 色相と明度の両方を離し、白文字のコントラストを保つ範囲で明るくしている。
    Palette palette{};
    palette.panel[0] = Color{0x1F, 0x5F, 0xBF};  // 青
    palette.panel[1] = Color{0x1C, 0x8A, 0x4B};  // 緑
    palette.panel[2] = Color{0xC2, 0x6A, 0x12};  // 橙
    palette.panel[3] = Color{0x7A, 0x33, 0xA8};  // 紫
    palette.button = Color{0x50, 0x54, 0x5E};    // ± の帯（背景より明るいグレー）
    palette.reset = Color{0xC0, 0x39, 0x2B};        // リセット（赤）
    palette.reset_armed = Color{0xF2, 0xC4, 0x1D};  // 2回目待ち（黄）。受け付けたことを見せる
    palette.text = Color{0xFF, 0xFF, 0xFF};
    palette.border = Color{0x00, 0x00, 0x00};
    return palette;
}

ScreenPresenter::ScreenPresenter(const QuadLayout& layout, DisplayRenderer& renderer,
                                 const Palette& palette)
    : layout_(layout), renderer_(renderer), palette_(palette) {}

void ScreenPresenter::format_value(std::int32_t value, char* out, std::size_t out_size) {
    if (out == nullptr || out_size == 0) {
        return;
    }
    std::snprintf(out, out_size, "%ld", static_cast<long>(value));
}

void ScreenPresenter::draw_player_contents(const domain::LifeState& state, int player_id) {
    const Orientation orientation = layout_.orientation_of(player_id);
    const Color panel_color = palette_.panel[player_id];
    renderer_.fill_rect(layout_.panel(player_id), panel_color);
    renderer_.draw_border(layout_.panel(player_id), palette_.border);

    const Rect increment = layout_.increment_button(player_id);
    const Rect decrement = layout_.decrement_button(player_id);
    renderer_.fill_rect(increment, palette_.button);
    renderer_.fill_rect(decrement, palette_.button);
    renderer_.draw_border(increment, palette_.border);
    renderer_.draw_border(decrement, palette_.border);
    renderer_.draw_text(increment, kIncrementSymbol, TextRole::Symbol, palette_.text, orientation);
    renderer_.draw_text(decrement, kDecrementSymbol, TextRole::Symbol, palette_.text, orientation);

    draw_reset_button_unframed(player_id, false);

    renderer_.draw_text(layout_.label_area(player_id), kPlayerLabels[player_id],
                        TextRole::PlayerLabel, palette_.text, orientation);

    char text[kValueBufferSize];
    format_value(state.value_of(player_id), text, sizeof(text));
    renderer_.draw_text(layout_.number_area(player_id), text, TextRole::Value, palette_.text,
                        orientation);
}

void ScreenPresenter::draw_reset_button_unframed(int player_id, bool armed) {
    const Rect reset = layout_.reset_button(player_id);
    renderer_.fill_rect(reset, armed ? palette_.reset_armed : palette_.reset);
    renderer_.draw_border(reset, palette_.border);
    renderer_.draw_text(reset, kResetSymbol, TextRole::Symbol, palette_.text,
                        layout_.orientation_of(player_id));
}

void ScreenPresenter::draw_reset_button(int player_id, bool armed) {
    if (!domain::LifeState::is_valid_player(player_id)) {
        return;
    }
    renderer_.begin_frame();
    draw_reset_button_unframed(player_id, armed);
    renderer_.end_frame();
}

void ScreenPresenter::draw_player(const domain::LifeState& state, int player_id) {
    if (!domain::LifeState::is_valid_player(player_id)) {
        return;
    }
    renderer_.begin_frame();
    draw_player_contents(state, player_id);
    renderer_.end_frame();
}

void ScreenPresenter::draw_all(const domain::LifeState& state) {
    renderer_.begin_frame();
    for (int player_id = 0; player_id < domain::kPlayerCount; ++player_id) {
        draw_player_contents(state, player_id);
    }
    renderer_.end_frame();
}

}  // namespace life::ui
