#include "text_metrics.h"

namespace life::ui {

int max_text_scale(TextRole role) {
    switch (role) {
        case TextRole::Value:
            return 12;
        case TextRole::Symbol:
            return 8;
        case TextRole::PlayerLabel:
            return 3;
    }
    return 1;
}

int fit_text_scale(int text_length, const Rect& area, const GlyphMetrics& glyph, int max_scale) {
    if (text_length <= 0 || glyph.width <= 0 || glyph.height <= 0) {
        return 1;
    }
    const int usable_width = area.width - 2 * glyph.padding;
    const int usable_height = area.height - 2 * glyph.padding;
    const int by_width = usable_width / (glyph.width * text_length);
    const int by_height = usable_height / glyph.height;
    int scale = by_width < by_height ? by_width : by_height;
    if (scale > max_scale) {
        scale = max_scale;
    }
    return scale < 1 ? 1 : scale;
}

namespace {

// 1軸分のインク開始位置を求める。
// cell: セル全体の長さ、ink: インクの長さ、area_start/area_size: 対象領域。
// 通常の向きではセルの余白がインクの後ろに、180°回転ではインクの手前に出る。
// flip_rounding: 端数を領域の後ろ側へ寄せるか。
int ink_origin(int area_start, int area_size, int cell, int ink, Orientation orientation,
               bool flip_rounding) {
    const int slack = area_size - ink;
    const int half = slack / 2;
    int origin = area_start + (flip_rounding ? slack - half : half);

    // セル（余白を含む）が領域からはみ出さないように寄せる。
    const int lead = (orientation == Orientation::UpsideDown) ? cell - ink : 0;
    const int min_origin = area_start + lead;
    const int max_origin = area_start + area_size - cell + lead;
    if (max_origin < min_origin) {
        return min_origin;  // 領域がセルより小さい場合は先頭合わせ
    }
    if (origin < min_origin) {
        origin = min_origin;
    }
    if (origin > max_origin) {
        origin = max_origin;
    }
    return origin;
}

}  // namespace

Rect text_bounds(int text_length, const Rect& area, const GlyphMetrics& glyph, int scale,
                 Orientation orientation) {
    const int cell_width = glyph.width * text_length * scale;
    const int cell_height = glyph.height * scale;
    // 末尾の文字だけ右の余白がインクに含まれない。行の高さも同様に下の余白を除く。
    const int ink_width = cell_width - (glyph.width - glyph.ink_width) * scale;
    const int ink_height = cell_height - (glyph.height - glyph.ink_height) * scale;

    const bool upside_down = orientation == Orientation::UpsideDown;
    // 縦: 端数はそのプレイヤー自身の上側へ寄せる（180°回転の領域では画面上の下側）。
    //     上下の領域は鏡像配置なので、これで画面全体が対称になる。
    // 横: 向きによらず常に画面座標の左側へ寄せる。同じ列の領域は同一座標なので、
    //     向きで丸めを変えると上下の数値が縦一直線に並ばなくなる（2026-10-11 実機指摘）。
    const int x = ink_origin(area.x, area.width, cell_width, ink_width, orientation, false);
    const int y = ink_origin(area.y, area.height, cell_height, ink_height, orientation, upside_down);
    return Rect{x, y, ink_width, ink_height};
}

}  // namespace life::ui
