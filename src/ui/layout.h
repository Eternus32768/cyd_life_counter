#pragma once

// 2×2の論理レイアウトと操作対象判定。画面寸法は注入する。
// 具体的なLCDライブラリ・タッチICを参照しない。

#include <optional>

#include "../application/life_command.h"
#include "../domain/life_state.h"
#include "geometry.h"

namespace life::ui {

// プレイヤー配置: 0=左上(P1) 1=右上(P2) 2=左下(P3) 3=右下(P4)
struct LayoutConfig {
    int screen_width;
    int screen_height;
    int button_strip_width;  // 画面外側に沿う ± 帯の幅
    int header_height;       // 画面外周側の帯（ラベルとリセットを並べる）の高さ
    int reset_width;         // 帯の中でリセットが占める幅
    int reset_gap;           // 画面中央側に空ける間隔（隣のプレイヤーのリセットとの誤タップ防止）

    // 画面寸法から比率で各寸法を決める。極小画面でも帯が領域を食い潰さないよう制限する。
    static LayoutConfig for_screen(int screen_width, int screen_height);
};

class QuadLayout {
public:
    explicit QuadLayout(const LayoutConfig& config);

    const LayoutConfig& config() const { return config_; }

    // 平置き前提。上行(P1/P2)は対面に座るプレイヤーが読むため180°回転させる。
    Orientation orientation_of(int player_id) const;

    // 以下はすべて有効ID(0〜3)が前提。無効IDでは幅・高さ0のRectを返す。
    Rect panel(int player_id) const;
    Rect increment_button(int player_id) const;
    Rect decrement_button(int player_id) const;
    Rect reset_button(int player_id) const;
    Rect value_area(int player_id) const;  // ± を除いた表示域
    Rect header_area(int player_id) const; // 外周側の帯（ラベル＋リセット）
    Rect label_area(int player_id) const;  // P1〜P4 のラベル
    Rect number_area(int player_id) const; // 現在値。他のどの操作領域とも重ならない

    // 画面内のどの操作にも当たらない場合は無効値を返す。
    std::optional<app::LifeCommand> hit_test(const Point& point) const;

private:
    bool is_right_column(int player_id) const { return (player_id % 2) == 1; }
    bool is_bottom_row(int player_id) const { return (player_id / 2) == 1; }
    Rect button_strip(int player_id) const;
    Rect strip_upper_half(int player_id) const;
    Rect strip_lower_half(int player_id) const;

    LayoutConfig config_;
};

}  // namespace life::ui
