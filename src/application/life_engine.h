#pragma once

// コマンドの検証・適用と状態取得。入力経路（タッチ/将来の通信）を知らない。

#include <cstdint>

#include "../domain/life_state.h"
#include "life_command.h"

namespace life::app {

class LifeEngine {
public:
    LifeEngine() = default;
    explicit LifeEngine(std::int32_t initial_value) : state_(initial_value) {}

    // 有効な操作として処理したときtrue。無効ID・未知action・int32端ではfalse。
    bool apply(const LifeCommand& command);

    const domain::LifeState& snapshot() const { return state_; }

private:
    domain::LifeState state_;
};

}  // namespace life::app
