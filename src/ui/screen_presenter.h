#pragma once

// 状態のスナップショットから描画命令を組み立てる。状態を持たず、描画先の実体を知らない。

#include <cstddef>
#include <cstdint>

#include "../domain/life_state.h"
#include "display_renderer.h"
#include "layout.h"

namespace life::ui {

struct Palette {
    Color panel[domain::kPlayerCount];
    Color button;
    Color reset;
    Color reset_armed;  // ダブルタップの1回目を受け付けた状態
    Color text;
    Color border;

    static Palette defaults();
};

class ScreenPresenter {
public:
    ScreenPresenter(const QuadLayout& layout, DisplayRenderer& renderer,
                    const Palette& palette = Palette::defaults());

    void draw_all(const domain::LifeState& state);
    void draw_player(const domain::LifeState& state, int player_id);

    // リセットボタンだけ描き直す。armed はダブルタップの2回目待ち。
    void draw_reset_button(int player_id, bool armed);

    // 値を10進文字列へ。負号を含む。INT32_MIN まで扱えるバッファ長を要求する。
    static constexpr std::size_t kValueBufferSize = 12;
    static void format_value(std::int32_t value, char* out, std::size_t out_size);

private:
    void draw_player_contents(const domain::LifeState& state, int player_id);
    void draw_reset_button_unframed(int player_id, bool armed);

    const QuadLayout& layout_;
    DisplayRenderer& renderer_;
    Palette palette_;
};

}  // namespace life::ui
