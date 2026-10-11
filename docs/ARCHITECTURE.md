# アーキテクチャ・依存境界

```text
Touch HW → TouchAdapter → UI hit-test → LifeCommand ┐
Future BLE / network adapters ───────────────────────┤
                                                    ▼
                                               LifeEngine
                                                    │ LifeState snapshot
                                                    ▼
                                     Presentation / Layout model
                                                    │
                                         DisplayRenderer interface
                                                    │
                                       CYD LCD adapter (初版)
                                     Future phone / other displays
```

## ルール
- `domain/`：プレイヤー状態、初期値、増減、リセット。標準C++のみ。ESP32・Arduino・LCD・タッチ・画面サイズへの参照禁止。
- `application/`：入力コマンドの検証・適用、状態取得。将来の通信経路を知らない。
- `ui/`：論理的レイアウト、操作対象判定、描画用モデル。画面寸法は注入する。具体的LCDライブラリを直接参照しない。
- `platform/cyd/`：実機LCD・タッチ・GPIO・回転・補正・描画。ボード差分をここに閉じ込める。
- `main`：依存を組み立てるだけ。状態更新を分散させない。
- `LifeCommand { playerId, action }` の `playerId` は内部で0〜3。画面ラベルはP1〜P4。無効ID・未知actionは状態を変更しない。
- ロジックから外部ディスプレイへ直接送信しない。後日、状態の購読/シリアライズを追加できる構造に留める。
- レイヤー間の依存は内側方向。`domain` は `ui` / `platform` をimportしない。

## 過剰設計を避ける
将来のBLE/ESP-NOW/スマホ向けのインターフェースは必要になった時に追加する。現時点ではコマンドと状態の分離で十分。スレッド、イベントバス、DIフレームワークは不要。
