#pragma once

// 入力装置に依存しない共通コマンド。タッチ・将来のBLE等はすべてこの形に正規化する。

namespace life::app {

enum class LifeAction {
    Increment,
    Decrement,
    Reset,
};

struct LifeCommand {
    int player_id;  // 0〜3
    LifeAction action;
};

}  // namespace life::app
