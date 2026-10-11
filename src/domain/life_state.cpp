#include "life_state.h"

#include <limits>

namespace life::domain {

LifeState::LifeState() : LifeState(kInitialValue) {}

LifeState::LifeState(std::int32_t initial_value) {
    for (int i = 0; i < kPlayerCount; ++i) {
        values_[i] = initial_value;
    }
}

std::int32_t LifeState::value_of(int player_id) const {
    if (!is_valid_player(player_id)) {
        return kInitialValue;
    }
    return values_[player_id];
}

bool LifeState::increment(int player_id) {
    if (!is_valid_player(player_id)) {
        return false;
    }
    if (values_[player_id] == std::numeric_limits<std::int32_t>::max()) {
        return false;  // 上限で変化なし
    }
    values_[player_id] += 1;
    return true;
}

bool LifeState::decrement(int player_id) {
    if (!is_valid_player(player_id)) {
        return false;
    }
    if (values_[player_id] == std::numeric_limits<std::int32_t>::min()) {
        return false;  // 下限で変化なし
    }
    values_[player_id] -= 1;
    return true;
}

bool LifeState::reset(int player_id) {
    if (!is_valid_player(player_id)) {
        return false;
    }
    values_[player_id] = kInitialValue;  // 当該プレイヤーのみ初期値へ
    return true;
}

}  // namespace life::domain
