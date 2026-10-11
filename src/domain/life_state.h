#pragma once

// 4プレイヤーの整数値を保持するドメイン状態。
// 標準C++のみ。Arduino / ESP32 / LCD / タッチ / 画面座標に依存しない。

#include <cstdint>

namespace life::domain {

// プレイヤーは固定4名。内部IDは 0〜3（画面ラベルはP1〜P4）。
constexpr int kPlayerCount = 4;

// 初版の初期値。将来設定値へ差し替える際はここ1箇所を変える。
constexpr std::int32_t kInitialValue = 0;

class LifeState {
public:
    // 既定は全員 kInitialValue。初期値の差し替え（将来の設定機能）と
    // int32端の検証のため、初期値を明示できる形も用意する。
    LifeState();
    explicit LifeState(std::int32_t initial_value);

    static constexpr bool is_valid_player(int player_id) {
        return player_id >= 0 && player_id < kPlayerCount;
    }

    // 無効IDでは kInitialValue を返す（配列外参照はしない）。
    std::int32_t value_of(int player_id) const;

    // 戻り値はいずれも「有効な操作として処理したか」。
    // 無効IDではfalse。increment / decrement は int32 の端で飽和しfalse。
    // reset は有効IDなら常にtrue（既に初期値でも受理とみなす）。
    bool increment(int player_id);
    bool decrement(int player_id);
    bool reset(int player_id);

private:
    std::int32_t values_[kPlayerCount];
};

}  // namespace life::domain
