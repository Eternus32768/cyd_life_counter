#pragma once

// 論理座標の基本型。ピクセル値は外部から注入され、ここでは単位を持たない整数として扱う。

namespace life::ui {

// 領域ごとの向き。平置きで対面に座るプレイヤー用に180°回転を持つ。
enum class Orientation {
    Normal,
    UpsideDown,
};

struct Point {
    int x;
    int y;
};

struct Rect {
    int x;
    int y;
    int width;
    int height;

    int right() const { return x + width; }
    int bottom() const { return y + height; }

    bool contains(const Point& p) const {
        return p.x >= x && p.x < right() && p.y >= y && p.y < bottom();
    }
};

}  // namespace life::ui
