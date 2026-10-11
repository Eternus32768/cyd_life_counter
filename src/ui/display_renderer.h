#pragma once

// 描画先の抽象。実機LCD・テスト用のフェイクはこれを実装する。
// 具体的なフォント・ピクセル単位のサイズ指定はデバイス層に委ね、ここでは役割だけ渡す。

#include <cstdint>

#include "geometry.h"

namespace life::ui {

struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

enum class TextRole {
    PlayerLabel,  // P1〜P4
    Value,        // 現在値（最大限大きく）
    Symbol,       // + / − / リセット記号
};

class DisplayRenderer {
public:
    virtual ~DisplayRenderer() = default;

    virtual void begin_frame() {}
    virtual void end_frame() {}

    virtual void fill_rect(const Rect& rect, const Color& color) = 0;
    virtual void draw_border(const Rect& rect, const Color& color) = 0;
    // text は矩形の中央に描く。はみ出しの扱いは実装側で決める。
    // orientation が UpsideDown の場合、文字を180°回転して描く（実装側の責務）。
    virtual void draw_text(const Rect& rect, const char* text, TextRole role, const Color& color,
                           Orientation orientation) = 0;
};

}  // namespace life::ui
