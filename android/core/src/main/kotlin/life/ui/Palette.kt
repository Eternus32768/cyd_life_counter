package life.ui

import life.domain.PLAYER_COUNT

// 色は 0xRRGGBB の整数。描画APIに依存しないため core に置く。
// 暗い背景に4色のカード。白文字のコントラスト 3:1 以上（大きな文字の基準）を保つ範囲で明るくしている。
data class Palette(
    val background: Int,
    val panelLight: List<Int>, // カードのグラデーション（プレイヤー視点で上側）
    val panelDark: List<Int>,  // 同（下側）
    val resetArmed: Int,       // ダブルタップの1回目を受け付けた状態。受け付けたことを見せる
    val onResetArmed: Int,     // resetArmed 上のアイコン色
    val text: Int,
) {
    init {
        require(panelLight.size == PLAYER_COUNT && panelDark.size == PLAYER_COUNT)
    }

    companion object {
        val DEFAULT = Palette(
            background = 0x0B0D12,
            panelLight = listOf(0x3F7BFF, 0x1FB57A, 0xF08A24, 0x9466FF), // 青 緑 橙 紫
            panelDark = listOf(0x2250D6, 0x0E8A5C, 0xCC5C0E, 0x6B3FE0),
            resetArmed = 0xFFC83D,
            onResetArmed = 0x1A1A1A,
            text = 0xFFFFFF,
        )
    }
}
