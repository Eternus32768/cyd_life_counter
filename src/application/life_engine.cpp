#include "life_engine.h"

namespace life::app {

bool LifeEngine::apply(const LifeCommand& command) {
    switch (command.action) {
        case LifeAction::Increment:
            return state_.increment(command.player_id);
        case LifeAction::Decrement:
            return state_.decrement(command.player_id);
        case LifeAction::Reset:
            return state_.reset(command.player_id);
    }
    return false;  // 未知action（範囲外の値）では状態を変更しない
}

}  // namespace life::app
