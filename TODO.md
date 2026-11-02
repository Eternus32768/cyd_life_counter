# 作業計画・進捗

最終更新: 2026-10-11（Phase 0〜3 完了、Codexレビュー反映、実機確認済み、平置き4人運用に対応）

## Phase 0 — 調査と環境
- [x] 実機基板の刻印を確認 — **ESP32-2432S028**（2026-10-11）。ピン配置とタッチIC(XPT2046)はこれで確定
- [x] LCDドライバIC の確定 — **ST7789**（2026-10-11 実機で確定。ID読み出しは不可だったため表示結果で判定）
- [x] ホスト側ツールチェーンの採用とバージョン固定（g++ 15.2.0 / C++17 / 自作テスト基盤・外部依存なし）
- [ ] 実機側ツールチェーンの確定 — arduino-cli 1.5.1 を採用候補として確認済み。`esp32:esp32` コアと表示ライブラリは未インストールで**導入承認待ち**
- [x] ホストテスト・静的検査コマンドを README へ追記
- [x] 未決定UI項目の実機検証方法を tests/ACCEPTANCE.md の人間試験項目へ対応付け

## Phase 1 — ロジック
- [x] 4プレイヤー初期値0、±1、個別リセット（`src/domain/life_state.*`）
- [x] 共通コマンドと状態スナップショット（`src/application/life_command.h`, `life_engine.*`）
- [x] 整数境界と不正playerIdの安全処理（飽和・無効ID無視）
- [x] ホスト単体テスト（`tests/host/test_life_engine.cpp`）

## Phase 2 — UI
- [x] 2×2レイアウトと縦並び±（`src/ui/layout.*`）
- [x] 座標変換とタッチイベント確定処理（`src/ui/touch_gate.*`、座標補正は呼び出し側）
- [x] プレイヤーごとのリセットUI（中央寄りの角に配置。最終決定は実機確認）
- [x] UI/ヒットテストのテスト（`tests/host/test_layout.cpp`, `test_touch_gate.cpp`）
- [x] 描画抽象と表示生成（`src/ui/display_renderer.h`, `screen_presenter.*`, `counter_screen.*`）

## Phase 3 — 実機
- [x] ボードドライバ、回転、タッチ補正の実装（`src/platform/cyd/`、ST7789系+XPT2046前提。**ピン値は実機未検証**）
- [x] ターゲットビルド成功（`esp32:esp32:esp32`、警告0、Flash 25% / RAM 7%、2026-10-11）
- [x] 実機テスト項目を実施し結果を記録（2026-10-11、tests/ACCEPTANCE.md）。
      未試験: 連打・長押し・指ずらしでの誤増減、負値・多桁表示の可読性
- [x] Codexレビュー（Codex CLI 0.154.0, read-only）、指摘5件の採否記録と修正・再検証 — `docs/REVIEW_LOG.md`

## v0.1 完了後に追加した対応
- [x] 平置き4人運用: P1/P2 の180°回転と `+`/`−` 入れ替え（SPEC.md 更新、2026-10-11 実機確認済み）
- [x] パレットを中間調へ変更（実機で識別性を確認）
- [x] リセットをダブルタップ確定に変更（猶予1000ms、SPEC.md 更新、2026-10-11 実機確認済み）

## 人間に残っている作業
1. 平置き対応以降の変更（向き対応、text_metrics、パレット、ボード設定、ダブルタップ確定）の Codex レビュー。
2. タッチ精度をさらに詰める場合: `-DCYD_TOUCH_CALIBRATION` で四隅補正値を取得し `board_config.h` に記録。

受入条件（tests/ACCEPTANCE.md）はすべて確認済み。多桁表示のみ負号込み3文字までの確認で、
最大11文字の見え方は未確認（自動縮小する実装、実使用上は問題なしと判断）。

## Android版（`android/`、詳細は `android/DECISIONS.md`）
- [x] Phase 0: 環境確認（JDK 21 / Android SDK 36 / Gradle 9.3.1）。タブレット・平置き4人・Kotlin で確定
- [x] Phase 1: `core` モジュール（domain / application / layout / reset確認）と JUnit（現在51件）— `./gradlew :core:test` 成功
- [x] Phase 2: Android UI（`View`+`Canvas` 描画、ポインタ入力→コマンド、横固定・常時点灯）。`:app:assembleDebug` / `lintDebug` 成功（エラー0）。見た目をモダンに再設計（2026-11-02）
- [x] Phase 3: APK ビルド、実機（タブレット）確認、SPEC/ACCEPTANCE の Android 節（2026-11-02、人間の実機確認はすべて問題なし。`tests/ACCEPTANCE.md`）
  - 2026-11-02 実機(iPlay 70 mini Ultra, Android 14, 2560×1600)に無線デバッグでインストール済み。`adb input tap` で ±・リセット2回タップ・領域独立を確認。
    **残り**: 実機タブレット（最小幅600dp以上）の Android 16 での向き固定（Pixel 9 Pro XL の疑似検証は済）、署名付きリリース APK・配布（必要になったら）
- [x] Codex レビュー（`prompts/CODEX_REVIEW.md`）— 2026-11-02、`docs/REVIEW_LOG.md`。AR01/AR02/AN01/AN02 を反映。AJ01（独立待機）・AJ02（離してから2回目）は承認のうえ実装（core 51件成功）。`SPEC.md` に Android版の差分を追記
