package life.application

import life.domain.LifeState

// コマンドの検証・適用と状態取得。入力経路を知らない。
class LifeEngine(initialValue: Int = life.domain.INITIAL_VALUE) {
    private val state = LifeState(initialValue)

    // 有効な操作として処理したとき true。無効IDや Int の端では false で状態は不変。
    fun apply(command: LifeCommand): Boolean = when (command.action) {
        LifeAction.Increment -> state.increment(command.playerId)
        LifeAction.Decrement -> state.decrement(command.playerId)
        LifeAction.Reset -> state.reset(command.playerId)
    }

    fun valueOf(playerId: Int): Int = state.valueOf(playerId)

    fun snapshot(): List<Int> = state.snapshot()
}
