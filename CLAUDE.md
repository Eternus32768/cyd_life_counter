# Claude Code — 実装担当

`README.md` → `SPEC.md` → `docs/ARCHITECTURE.md` → `AGENTS.md` → `TODO.md` の順に読む。

## 実装ループ
1. Phase 0: リポジトリ・ボード型番・ライブラリ・既存資産を調査。実機型番未確認ならドライバを決め打ちしない。作業計画と確認事項を提示。
2. Phase 1: ハード非依存のモデル・コマンド処理とホストテストを作る。
3. Phase 2: 4分割UIのレイアウト、タッチ座標→コマンド変換、描画アダプタを作る。
4. Phase 3: CYDボードアダプタを作り、ビルドと実機確認手順を整える。
5. 各Phase終了時、lint/静的検査、テスト、ビルドを実施。結果を明記。
6. Codexに `prompts/CODEX_REVIEW.md` に基づく独立レビューを依頼。可能なら公式 `openai/codex-plugin-cc` の `/codex:review` を利用。重大変更時は `/codex:adversarial-review` も検討。プラグイン不在ならレビュー依頼文を出力。
7. 指摘の採否を記録し、必要な修正と再テストを実施。人間の確認が必要な点は止めて質問する。

勝手に大規模な機能追加・設計の一般化をしない。まず v0.1 の完成を優先。進捗は `TODO.md` に反映する。
