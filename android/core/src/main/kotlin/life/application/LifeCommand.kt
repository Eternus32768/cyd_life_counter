package life.application

// 入力装置に依存しない共通コマンド。タッチ・将来の通信はすべてこの形に正規化する。

enum class LifeAction { Increment, Decrement, Reset }

data class LifeCommand(val playerId: Int, val action: LifeAction)
