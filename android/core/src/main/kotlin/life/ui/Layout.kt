package life.ui

import life.application.LifeAction
import life.application.LifeCommand
import life.domain.LifeState
import life.domain.PLAYER_COUNT

// 2×2の論理レイアウトと操作対象判定。画面寸法は注入する。
// プレイヤー配置: 0=左上(P1) 1=右上(P2) 2=左下(P3) 3=右下(P4)

private fun clampInt(value: Int, low: Int, high: Int): Int = when {
    high < low -> low
    value < low -> low
    value > high -> high
    else -> value
}

data class LayoutConfig(
    val screenWidth: Int,
    val screenHeight: Int,
    val buttonStripWidth: Int, // 画面外側に沿う ± 帯の幅
    val headerHeight: Int,     // 画面外周側の帯（ラベルとリセットを並べる）の高さ
    val resetWidth: Int,       // 帯の中でリセットが占める幅
    val resetGap: Int,         // 画面中央側に空ける間隔（隣のプレイヤーのリセットとの誤タップ防止）
) {
    companion object {
        // 画面寸法から比率で各寸法を決める。極小画面でも帯が領域を食い潰さないよう制限する。
        fun forScreen(screenWidth: Int, screenHeight: Int): LayoutConfig {
            val panelWidth = screenWidth / 2
            val panelHeight = screenHeight / 2
            val shorterSide = minOf(panelWidth, panelHeight)
            return LayoutConfig(
                screenWidth = screenWidth,
                screenHeight = screenHeight,
                buttonStripWidth = clampInt(panelWidth * 28 / 100, 1, panelWidth - 1),
                headerHeight = clampInt(panelHeight * 26 / 100, 1, panelHeight - 1),
                resetWidth = clampInt(shorterSide * 30 / 100, 1, shorterSide - 1),
                resetGap = clampInt(shorterSide / 30, 0, shorterSide / 4),
            )
        }
    }
}

class QuadLayout(val config: LayoutConfig) {
    private fun isRightColumn(id: Int) = id % 2 == 1
    private fun isBottomRow(id: Int) = id / 2 == 1

    // 平置き前提。上行(P1/P2)は対面に座るプレイヤーが読むため180°回転させる。
    fun orientationOf(playerId: Int): Orientation {
        if (!LifeState.isValidPlayer(playerId)) return Orientation.Normal
        return if (isBottomRow(playerId)) Orientation.Normal else Orientation.UpsideDown
    }

    // 以下はすべて有効ID(0〜3)が前提。無効IDでは幅・高さ0の Rect を返す。
    fun panel(playerId: Int): Rect {
        if (!LifeState.isValidPlayer(playerId)) return Rect.EMPTY
        // 奇数解像度でも隙間が出ないよう、右列・下行が余りを受け持つ。
        val leftWidth = config.screenWidth / 2
        val topHeight = config.screenHeight / 2
        val x = if (isRightColumn(playerId)) leftWidth else 0
        val width = if (isRightColumn(playerId)) config.screenWidth - leftWidth else leftWidth
        val y = if (isBottomRow(playerId)) topHeight else 0
        val height = if (isBottomRow(playerId)) config.screenHeight - topHeight else topHeight
        return Rect(x, y, width, height)
    }

    private fun buttonStrip(playerId: Int): Rect {
        val area = panel(playerId)
        if (area.width == 0) return Rect.EMPTY
        val stripWidth = clampInt(config.buttonStripWidth, 1, area.width)
        // 画面外側（左列は左端、右列は右端）に沿わせる。
        val x = if (isRightColumn(playerId)) area.right - stripWidth else area.x
        return Rect(x, area.y, stripWidth, area.height)
    }

    private fun stripUpperHalf(playerId: Int): Rect {
        val s = buttonStrip(playerId)
        return if (s.width == 0) Rect.EMPTY else Rect(s.x, s.y, s.width, s.height / 2)
    }

    private fun stripLowerHalf(playerId: Int): Rect {
        val s = buttonStrip(playerId)
        if (s.width == 0) return Rect.EMPTY
        val upper = s.height / 2
        return Rect(s.x, s.y + upper, s.width, s.height - upper)
    }

    // 本人の視点で上が + になるように置く。180°回転の領域では画面上の下半分が + になる。
    fun incrementButton(playerId: Int): Rect =
        if (orientationOf(playerId) == Orientation.UpsideDown) stripLowerHalf(playerId) else stripUpperHalf(playerId)

    fun decrementButton(playerId: Int): Rect =
        if (orientationOf(playerId) == Orientation.UpsideDown) stripUpperHalf(playerId) else stripLowerHalf(playerId)

    // ± を除いた表示域
    fun valueArea(playerId: Int): Rect {
        val area = panel(playerId)
        if (area.width == 0) return Rect.EMPTY
        val strip = buttonStrip(playerId)
        val x = if (isRightColumn(playerId)) area.x else area.x + strip.width
        return Rect(x, area.y, area.width - strip.width, area.height)
    }

    // 外周側（上行は上端、下行は下端）の帯。ラベルとリセットを並べる。
    fun headerArea(playerId: Int): Rect {
        val value = valueArea(playerId)
        if (value.width == 0) return Rect.EMPTY
        val height = clampInt(config.headerHeight, 1, value.height - 1)
        val y = if (isBottomRow(playerId)) value.bottom - height else value.y
        return Rect(value.x, y, value.width, height)
    }

    // ± とは反対側（画面中央寄り）に置き、増減操作の指の軌道から離す。
    fun resetButton(playerId: Int): Rect {
        val header = headerArea(playerId)
        if (header.width == 0) return Rect.EMPTY
        val width = clampInt(config.resetWidth, 1, header.width - 1)
        val gap = clampInt(config.resetGap, 0, header.width - width)
        val x = if (isRightColumn(playerId)) header.x + gap else header.right - gap - width
        return Rect(x, header.y, width, header.height)
    }

    // 帯のうちリセットより画面外側の部分。
    fun labelArea(playerId: Int): Rect {
        val header = headerArea(playerId)
        if (header.width == 0) return Rect.EMPTY
        val reset = resetButton(playerId)
        val x = if (isRightColumn(playerId)) reset.right else header.x
        val width = if (isRightColumn(playerId)) header.right - reset.right else reset.x - header.x
        return Rect(x, header.y, maxOf(width, 1), header.height)
    }

    // 現在値。他のどの操作領域とも重ならない。
    fun numberArea(playerId: Int): Rect {
        val value = valueArea(playerId)
        if (value.width == 0) return Rect.EMPTY
        val header = headerArea(playerId)
        val y = if (isBottomRow(playerId)) value.y else value.y + header.height
        return Rect(value.x, y, value.width, value.height - header.height)
    }

    // 画面内のどの操作にも当たらない場合は null。
    fun hitTest(point: Point): LifeCommand? {
        if (point.x < 0 || point.y < 0 || point.x >= config.screenWidth || point.y >= config.screenHeight) {
            return null
        }
        for (id in 0 until PLAYER_COUNT) {
            if (!panel(id).contains(point)) continue
            return when {
                resetButton(id).contains(point) -> LifeCommand(id, LifeAction.Reset)
                incrementButton(id).contains(point) -> LifeCommand(id, LifeAction.Increment)
                decrementButton(id).contains(point) -> LifeCommand(id, LifeAction.Decrement)
                else -> null  // 領域内だが操作対象外（数値表示部・ラベル）
            }
        }
        return null
    }
}
