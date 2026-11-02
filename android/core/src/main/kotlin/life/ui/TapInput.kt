package life.ui

import life.application.LifeAction
import life.application.LifeCommand
import life.domain.LifeState
import life.domain.PLAYER_COUNT

// 指（ポインタ）の押下・離上を受け、ダブルタップ確定（ResetConfirmation）へ渡すか決める。
//
// Android版の制約（Codex レビュー AJ02、2026-11-02 人間承認）:
//  リセットの1回目を押した指が画面に触れている間は、同じプレイヤーの2回目のリセットを受け付けない。
//  手のひらや二本指の同時接触で、ダブルタップとして確定してしまうのを防ぐ。
//  無視した接触は待機の状態も期限も変えない。他プレイヤーの操作や ± は影響を受けない。
class TapInput(private val confirmation: ResetConfirmation) {
    private companion object {
        const val RELEASED = -1
    }

    // 各プレイヤーのリセット待機を始めた指。離れたら RELEASED。
    private val armedBy = IntArray(PLAYER_COUNT) { RELEASED }

    // 指が押されたとき。確定したコマンドだけを返す（ResetConfirmation.accept と同じ意味）。
    fun pointerDown(pointerId: Int, command: LifeCommand, timestampMs: Long): LifeCommand? {
        confirmation.tick(timestampMs)
        val id = command.playerId
        val isReset = command.action == LifeAction.Reset && LifeState.isValidPlayer(id)

        if (isReset && confirmation.isPending(id) && armedBy[id] != RELEASED) {
            return null  // 1回目の指がまだ触れている。2回目としては数えず、状態も変えない
        }
        val confirmed = confirmation.accept(command, timestampMs)
        if (isReset) {
            armedBy[id] = if (confirmed == null) pointerId else RELEASED  // 1回目なら記録、確定なら消す
        }
        return confirmed
    }

    // 指が離れたとき。ポインタ番号は再利用されるため、離れた時点で記録を消す。
    fun pointerUp(pointerId: Int) {
        for (id in armedBy.indices) if (armedBy[id] == pointerId) armedBy[id] = RELEASED
    }

    // ACTION_CANCEL やウィンドウのフォーカス喪失など、どの指が残っているか分からないとき。
    fun cancelAll() {
        armedBy.fill(RELEASED)
    }
}
