#include "cyd_renderer.h"

#include <cstdint>
#include <cstring>

#include "../../ui/text_metrics.h"

namespace life::platform::cyd {
namespace {

// LovyanGFX 内蔵 Font0 の実寸（倍率1）。tests/host/test_text_layout.cpp が同じ値で検査している。
// セルは6×8、実際に描かれるインクは5×7（右と下に1ドットの余白）。
constexpr ui::GlyphMetrics kFont0Metrics{6, 8, 5, 7, 4};

// 採用回転(1)に対して180°違いの回転値。横向きの4通り MV / MV|MX / MV|MY / MV|MX|MY のうち、
// 1=MV|MX が正しく 5=MV が左右反転だったことから、MY を足した 3=MV|MY が180°に当たる。
// 実機で上下反転の文字が鏡像になる場合は 7 に変える。
constexpr std::uint8_t kRotated180 = 3;

}  // namespace

CydRenderer::CydRenderer(CydDisplay& display) : display_(display) {}

void CydRenderer::begin_frame() { display_.startWrite(); }

void CydRenderer::end_frame() { display_.endWrite(); }

void CydRenderer::fill_rect(const ui::Rect& rect, const ui::Color& color) {
    display_.fillRect(rect.x, rect.y, rect.width, rect.height,
                      display_.color888(color.r, color.g, color.b));
}

void CydRenderer::draw_border(const ui::Rect& rect, const ui::Color& color) {
    display_.drawRect(rect.x, rect.y, rect.width, rect.height,
                      display_.color888(color.r, color.g, color.b));
}

void CydRenderer::draw_text(const ui::Rect& rect, const char* text, ui::TextRole role,
                            const ui::Color& color, ui::Orientation orientation) {
    if (text == nullptr) {
        return;
    }
    const int length = static_cast<int>(std::strlen(text));
    const int scale = ui::fit_text_scale(length, rect, kFont0Metrics, ui::max_text_scale(role));

    // 中心座標ではなく占有矩形を基準に描く。中心を点対称にする方式では、文字幅・高さが偶数のとき
    // 反転側だけ1ピクセルずれる（[c-w/2, c+w/2-1] の反射が [c-w/2+1, c+w/2] になるため）。
    // 矩形を反射すれば上下の行で厳密に対称な位置になる。
    const ui::Rect box = ui::text_bounds(length, rect, kFont0Metrics, scale, orientation);

    display_.setFont(&lgfx::fonts::Font0);
    display_.setTextSize(static_cast<float>(scale));
    display_.setTextDatum(lgfx::textdatum::top_left);
    display_.setTextColor(display_.color888(color.r, color.g, color.b));

    if (orientation == ui::Orientation::UpsideDown) {
        // LovyanGFX v1 に文字単体の回転描画がないため、画面回転を180°違いへ一時的に切り替える。
        const int width = display_.width();
        const int height = display_.height();
        display_.setRotation(kRotated180);
        display_.drawString(text, width - box.x - box.width, height - box.y - box.height);
        display_.setRotation(static_cast<std::uint8_t>(kDisplayRotation));
        return;
    }

    display_.drawString(text, box.x, box.y);
}

}  // namespace life::platform::cyd
