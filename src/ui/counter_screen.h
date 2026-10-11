#pragma once

// タッチ入力 → コマンド → 状態更新 → 再描画 の配線。
// main はこれを組み立てて回すだけにし、状態更新を分散させない。

#include "../application/life_engine.h"
#include "layout.h"
#include "reset_confirmation.h"
#include "screen_presenter.h"
#include "touch_gate.h"

namespace life::ui {

class CounterScreen {
public:
    CounterScreen(app::LifeEngine& engine, DisplayRenderer& renderer,
                  const LayoutConfig& layout_config,
                  const TouchGateConfig& touch_config = TouchGateConfig{},
                  const Palette& palette = Palette::defaults(),
                  const ResetConfirmationConfig& reset_config = ResetConfirmationConfig{});

    const QuadLayout& layout() const { return layout_; }

    // 起動時の全画面描画。
    void draw_initial();

    // タッチ1サンプルを処理する。画面を描き直した場合に true を返す。
    // リセットはダブルタップで確定し、1回目では待機表示だけを更新する。
    bool handle_touch(const TouchSample& sample);

    // 2回目を待っているプレイヤー（待機なしは ResetConfirmation::kNoPending）。
    int pending_reset_player() const { return confirmation_.pending_player(); }

private:
    app::LifeEngine& engine_;
    QuadLayout layout_;
    ScreenPresenter presenter_;
    TouchGate gate_;
    ResetConfirmation confirmation_;
};

}  // namespace life::ui
