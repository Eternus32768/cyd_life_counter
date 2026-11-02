package life.domain

// 4プレイヤーの整数値を保持するドメイン状態。Android / 画面座標に依存しない。

// プレイヤーは固定4名。内部IDは 0〜3（画面ラベルはP1〜P4）。
const val PLAYER_COUNT = 4

// 初版の初期値。将来設定値へ差し替える際はここ1箇所を変える。
const val INITIAL_VALUE: Int = 0

class LifeState(initialValue: Int = INITIAL_VALUE) {
    private val values = IntArray(PLAYER_COUNT) { initialValue }

    // 無効IDでは INITIAL_VALUE を返す。
    fun valueOf(playerId: Int): Int = if (isValidPlayer(playerId)) values[playerId] else INITIAL_VALUE

    // 戻り値は「有効な操作として処理したか」。
    // 無効IDでは false。increment / decrement は Int の端で飽和し false。
    // reset は有効IDなら常に true（既に初期値でも受理とみなす）。
    fun increment(playerId: Int): Boolean {
        if (!isValidPlayer(playerId) || values[playerId] == Int.MAX_VALUE) return false
        values[playerId] += 1
        return true
    }

    fun decrement(playerId: Int): Boolean {
        if (!isValidPlayer(playerId) || values[playerId] == Int.MIN_VALUE) return false
        values[playerId] -= 1
        return true
    }

    fun reset(playerId: Int): Boolean {
        if (!isValidPlayer(playerId)) return false
        values[playerId] = INITIAL_VALUE  // 当該プレイヤーのみ初期値へ
        return true
    }

    // 描画側に渡す不変スナップショット。以後の操作は反映されない。
    fun snapshot(): List<Int> = values.toList()

    companion object {
        fun isValidPlayer(playerId: Int): Boolean = playerId in 0 until PLAYER_COUNT
    }
}
