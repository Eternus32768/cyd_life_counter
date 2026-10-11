# CYD Life Counter — 開発入口

初版 v0.1。Claude Code が実装、Codex が独立レビュー、人間が仕様決定・実機確認を行う。

## 文書の優先順位
1. `SPEC.md` — 製品仕様の正本
2. `docs/ARCHITECTURE.md` — 境界と設計制約
3. `AGENTS.md` — 共通の作業・品質規約
4. `CLAUDE.md` / `prompts/CODEX_REVIEW.md` — 役割別手順
5. `TODO.md` — 実装状況。仕様を上書きしない。

矛盾・未決定事項は推測で埋めず `docs/DECISIONS.md` に記録して人間に確認する。`docs/ROADMAP.md` の将来案は初版の実装義務ではない。

## 最初の進め方
1. Claude Code に「CLAUDE.md に従い Phase 0 から開始」と依頼。
2. ボード型番と液晶コントローラを確認し、未確認ならホストテストを先行。
3. 各段階でテスト・ビルド・変更要約を作成。
4. Codex に `prompts/CODEX_REVIEW.md` を読ませて独立レビュー。
5. Claude Code が指摘を分類・修正し、再検証。実機試験は人間が実施。

開発環境の初期案: PlatformIO + Arduino framework + C++。採用理由とバージョンは Phase 0 で固定する。専用 MCP の新設は不要。公式 Codex 連携が利用可能なら使い、なければ手動でレビュー依頼を渡す。

## ソース構成（v0.1）
```
src/domain/       プレイヤー状態・初期値・増減・リセット（標準C++のみ）
src/application/  LifeCommand と LifeEngine（入力経路を知らない）
src/ui/           レイアウト・ヒットテスト・タッチ確定・描画抽象・画面配線
src/platform/cyd/ 実機LCD/タッチ（LovyanGFX）。ボード差分はここだけ
tests/host/       ホスト単体テスト（自作の最小テスト基盤）
tools/            層境界チェック
```

## ホストでの検証コマンド
```bash
make check        # 層境界チェック → domain単独ビルド → テスト → ASan/UBSanテスト
make test         # ビルドして単体テストを実行
make test-asan    # ASan/UBSan付きで実行
make check-layers # domain/application/ui に実機依存が無いことを検査
make clean
```
`make check` は警告をエラー扱い（`-Werror`）でビルドする。ツール要件は g++ 15.2.0 / GNU Make 4.4.1（Phase 0 実測）。

## 実機ビルド
前提: ST7789系 + XPT2046 の CYD（2026-10-11 に人間が指定）。スケッチは `cyd_life_counter_md.ino`、
実機依存コードは `src/platform/cyd/` に閉じ込めている。

ビルド・書き込み・タッチ補正の手順と固定バージョン（arduino-cli 1.5.1 / esp32:esp32 3.3.12 / LovyanGFX 1.2.28）は
`docs/HARDWARE.md` を参照。ピン定義とパネル設定は公開資料ベースの**実機未検証値**で、実機確認後に
`src/platform/cyd/board_config.h` を更新する。
