package life.ui

// 論理座標の基本型。単位はピクセルだが、描画APIには依存しない。

// 領域ごとの向き。平置きで対面に座るプレイヤー用に180°回転を持つ。
enum class Orientation { Normal, UpsideDown }

data class Point(val x: Int, val y: Int)

data class Rect(val x: Int, val y: Int, val width: Int, val height: Int) {
    val right: Int get() = x + width
    val bottom: Int get() = y + height

    fun contains(p: Point): Boolean = p.x >= x && p.x < right && p.y >= y && p.y < bottom

    fun overlaps(o: Rect): Boolean = x < o.right && o.x < right && y < o.bottom && o.y < bottom

    companion object {
        val EMPTY = Rect(0, 0, 0, 0)
    }
}
