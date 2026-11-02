package life.ui

import life.application.LifeAction
import life.application.LifeCommand
import life.domain.LifeState

// リセットの誤操作防止。リセットだけダブルタップで確定させる。
//
// CYD版との差分（android/DECISIONS.md 参照）:
//  - 待機はプレイヤーごとに独立して持つ。4人が同時に使うため、他人の操作で自分の待機が
//    解除されないようにする。解除されるのは、そのプレイヤー自身の他操作・猶予切れのみ。
//  - 時刻は単調増加の Long(ms)。32bit折り返しは考慮しない。

const val DEFAULT_RESET_WINDOW_MS: Long = 1000

class ResetConfirmation(private val windowMs: Long = DEFAULT_RESET_WINDOW_MS) {
    private val pendingSince = LongArray(life.domain.PLAYER_COUNT)
    private val pending = BooleanArray(life.domain.PLAYER_COUNT)

    // 確定したコマンドだけを返す。Reset は同じプレイヤーで 1000ms 未満に2回目が来たときのみ通す。
    fun accept(command: LifeCommand, timestampMs: Long): LifeCommand? {
        tick(timestampMs)
        val id = command.playerId
        if (!LifeState.isValidPlayer(id)) return null

        if (command.action != LifeAction.Reset) {
            pending[id] = false  // 本人の他操作が入ったら待機を解除する
            return command
        }
        if (pending[id]) {
            pending[id] = false  // 2回目。ここで確定する
            return command
        }
        pending[id] = true  // 1回目
        pendingSince[id] = timestampMs
        return null
    }

    // タッチが無いときも呼び、猶予切れを処理する。経過が windowMs 以上で切れ（ちょうどは不成立）。
    fun tick(timestampMs: Long) {
        for (id in pending.indices) {
            if (pending[id] && timestampMs - pendingSince[id] >= windowMs) pending[id] = false
        }
    }

    // 待機中のプレイヤーのうち、最も早く猶予切れになる時刻（絶対時刻 ms）。待機が無ければ null。
    // 表示側が「次にいつ tick するか」を決めるために使う。
    fun nextExpiryMs(): Long? {
        var earliest: Long? = null
        for (id in pending.indices) {
            if (!pending[id]) continue
            val deadline = pendingSince[id] + windowMs
            if (earliest == null || deadline < earliest) earliest = deadline
        }
        return earliest
    }

    fun isPending(playerId: Int): Boolean = LifeState.isValidPlayer(playerId) && pending[playerId]
}
