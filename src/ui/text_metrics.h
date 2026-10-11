#pragma once

// 文字をどの倍率で矩形に収めるかの計算。フォントの実寸は呼び出し側（デバイス層）から渡す。
// ここで持つのは「どの役割をどこまで大きくするか」という表示方針だけ。

#include "display_renderer.h"
#include "geometry.h"

namespace life::ui {

// 倍率1のときの文字寸法。セル（送り幅）と、実際に描かれるインクを区別する。
// 内蔵フォントはセル内の右と下に余白を持つため、180°回転するとその余白が反対側へ移る。
// セル基準で中央寄せすると、回転した領域と通常の領域で余白1つ分ずれてしまう。
struct GlyphMetrics {
    int width;       // セルの送り幅
    int height;      // セルの高さ
    int ink_width;   // セル内で実際に描かれる幅（右の余白を除く）
    int ink_height;  // セル内で実際に描かれる高さ（下の余白を除く）
    int padding;     // 矩形内側に確保する余白
};

// 役割ごとの最大倍率。数値を最も大きく、ラベルは控えめにする。
int max_text_scale(TextRole role);

// 矩形に収まる最大倍率（最低1）。収まらない場合も1を返して描画を落とさない。
int fit_text_scale(int text_length, const Rect& area, const GlyphMetrics& glyph, int max_scale);

// 中央揃えで描いたときに文字の**インク**が占める矩形。セルの余白は含まない。
// 端数の寄せ方は軸ごとに異なる:
//  - 縦: そのプレイヤー自身の上側へ寄せる（180°回転の領域では画面上の下側）。
//        上下の領域は鏡像配置なので、これで画面全体が対称になる。
//  - 横: 向きによらず常に画面座標の左側へ寄せる。同じ列の領域は同一座標にあり、
//        向きで丸めを変えると上下の数値が縦一直線に並ばなくなるため（2026-10-11 実機指摘）。
// 描画側はこの矩形の原点からセルを描く（回転時は画面中心で反射した位置に描く）。
// セルが領域からはみ出す場合は、はみ出さない位置へ寄せる。
Rect text_bounds(int text_length, const Rect& area, const GlyphMetrics& glyph, int scale,
                 Orientation orientation = Orientation::Normal);

}  // namespace life::ui
