# ハードウェア確認事項

ターゲット候補: ESP32-2432S028R系 2.8インチ CYD、320×240横向き、抵抗膜タッチ。**基板バリエーションによりLCDドライバが異なるため、型番末尾・端子構成だけで断定しない。**

参考: 一般的な初期型は ILI9341 + XPT2046。USB-C/二端子版等に ST7789 の派生例あり。実機シルク・販売情報・動作確認を優先する。

Phase 0で記録:
- 基板刻印、基板写真、USBポート数と種類
- LCDコントローラ / タッチコントローラ
- 採用ライブラリ、バージョン、ピン定義、回転方向
- タッチ四隅の補正値、画面座標との対応
- 使用したPlatformIO環境とビルド結果

参考資料（実装前に照合）:
- https://github.com/kthxbyte/platformio-esp32-2432s028
- https://github.com/JamesDavid/cyd-GridBot/blob/main/CYD-ESP32-2432S028R.md

未確認のGPIOやドライバを「実機確認済」と記載しない。

## Phase 0 調査結果（2026-10-11、開発環境のみ）

### 実機情報
- 基板シルク: **ESP32-2432S028**（2026-10-11、人間が現物を確認）
- 液晶モジュールのシルク: **TPM408-2.8**、USB端子は **Type-C**（実機写真 `docs/images/screen-2026-10-11.jpg`）
- ピン定義: ESP32-2432S028 系で共通のため `src/platform/cyd/board_config.h` の値を採用。
  表示 SCLK14 / MOSI13 / MISO12 / CS15 / DC2 / BL21、タッチ SCLK25 / MOSI32 / MISO39 / CS33 / IRQ36。
  **実機で未検証**（表示が出れば検証済みとなる）。
- LCDドライバIC: **ST7789（2026-10-11 実機で確定）**。ST7789設定で画面が正常に表示されたことを根拠とする。
  ILI9341 では初期化が通らず表示されないため、表示できた時点で確定とみなす。
- ID読み出しは不可（下記）。判定は表示結果と `tools/cyd_probe` のテストパターンで行った。

### 向きの基準（必ずこれを明示して判定する）
**画面を見たとき USB コネクタが右側にある向き**を基準とする（2026-10-11 実機確認）。
この向きで `kDisplayRotation = 1` のとき、論理座標の左上が物理の左上に一致する。

基板を180°持ち替えると同じビルドでも「上下が逆さま」に見えるため、向きの報告は必ず
USBコネクタの位置とセットで記録する。実機確認の初期に、この基準が共有されていなかったために
「回転1で上下反転」という報告と「回転1で四隅が正しい」という報告が両方出て切り分けが長引いた。

### 実機で確定した設定（2026-10-11、tools/cyd_probe による測定）
| 項目 | 確定値 | 判定根拠 |
|---|---|---|
| `kDisplayRotation` | **1** | 四隅ラベル TL/TR/BL/BR が正しい隅に出て、非対称文字 F が鏡像でない |
| `kPanelInvert` | **false** | 純色帯テストで補色化を確認（送信赤→黄、緑→赤系、青→水色） |
| `kPanelRgbOrder` | **false** | 同上。純色が純色で出ないのは反転が原因で、色順の問題ではなかった |
| `kTouchOffsetRotation` | **4** | 実効 r=5（x/y入れ替えのみ。アフィンが min/max から昇順に作られるため追加の反転は不要） |
| タッチ生値レンジ | x 420〜3750 / y 350〜3700 | 四隅実測値に余裕を持たせた（旧 300〜3900 では下端が y=211 止まり） |

#### 色の切り分け経過（同じ症状を二度追わないための記録）
`invert=true` は「ST7789系CYDで必要な例が多い」という一般論で入れた初期値だったが、この個体では不要だった。
反転が入ると純色が補色になるため、最初の「リセットボタンの赤が青っぽい」は色順の問題に見えた。
`rgb_order` を反転させると「黄色」に変わり、両方を疑うことで初めて切り分けられた。
判定には**純色に文字ラベルを添えたテストパターン**（`tools/cyd_probe`）が有効だった。

#### タッチ実測値（画面回転1 / touch offset_rotation=**2** → 実効 r=3 のとき）
測定時点の診断スケッチは `kTouchOffsetRotation = 2` でビルドされていた。
確定値（offset 4 / 実効 r=5）での測定値ではない点に注意する。
| タッチした隅 | raw | conv | 期待値 |
|---|---|---|---|
| TL | (478,459) | (306,11) | (≈10,≈10) |
| TR | (445,3579) | (28,9) | (≈310,≈10) |
| BL | (3355,365) | (314,202) | (≈10,≈230) |
| BR | (3490,3655) | (22,211) | (≈310,≈230) |

`raw.x` が画面の縦方向（上端≈445／下端≈3655）、`raw.y` が横方向（左≈365／右≈3655）に対応する。
この測定時の実効回転は **r=3**（診断スケッチのビルド時点で `kTouchOffsetRotation = 2` だった）で、
X のみ反転・Y は一致という結果になる。別途「BR をタッチすると BL 付近に丸が出る」ことも確認した。

#### タッチ実効回転の対応表（Panel_Device::convertRawXY）
`r&1` で x/y 入れ替え、`r&2` で X 反転、`r∈{1,2,4,7}` で上下反転。
アフィンは `raw.x→X昇順` / `raw.y→Y昇順` に作られるため、横向きでは入れ替えのみの **r=5** が正しい。

| 実効 r | X | Y | 実機での症状 |
|---|---|---|---|
| 1 | 正 | 逆 | 未試験 |
| 3 | 逆 | 正 | 診断スケッチでの測定値がこれ |
| **5** | **正** | **正** | **採用。実機で一致** |
| 7 | 逆 | 逆 | P1操作がP4に反映され、+/-も逆（180°ずれ） |

切り分けを長引かせた原因は、測定時の実効回転（r=3）を r=1 と誤認し「X反転を足す」と判断して
r=7 にしてしまったこと。**測定値を記録するときは、そのときの実効 r も必ず併記する。**
- タッチIC: XPT2046（抵抗膜、2432S028系共通）。

### ドライバID読み出しの実測結果（2026-10-11）
```
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
CYD Life Counter v0.1
panel driver (build): ST7789
panel id: RDDID(0x04)=0xD9818181 RDID4(0xD3)=0x00000000
```
**この個体ではID読み出しが機能しない。** `RDID4=0x00000000` は ILI9341 を否定する材料になるが、
`RDDID=0xD9818181` も ST7789 の規定値（0x85 0x85 0x52 系）ではない。
ESP32-2432S028 は液晶側 MISO が結線されていない個体があり、本機もそれに該当すると判断する。
→ **ドライバ判別は表示結果で行う**（下記の対処表）。ID読み出しの行は診断材料として残す。

### LCDドライバの判別手順（写真不要）
起動時にシリアル(115200)へ次の行が出る。
```
panel driver (build): ST7789
panel id: RDDID(0x04)=0x........ RDID4(0xD3)=0x........
```
- `RDID4` に **0x9341** が含まれる → **ILI9341**。`-DCYD_PANEL_ILI9341` を付けて再ビルドする。
- `RDID4` が `0x00000000` / `0xFFFFFFFF` など無意味な値で、`RDDID` に **0x85 0x85 0x52** 系の並びが見える
  → **ST7789**（既定のまま）。
- どちらとも判別できない場合は表示結果で判断する。
  **以下は未知のボードを調べるための一般的な対処表であり、本機の確定値ではない。**
  本機の確定値は「実機で確定した設定」の表だけを正本とする。判断材料:
  | 症状 | 対処 |
  |---|---|
  | 何も映らない（バックライトのみ点灯） | ドライバ違いの可能性 → `-DCYD_PANEL_ILI9341` で再ビルド |
  | 白黒が反転している | `kPanelInvert` を反転 |
  | 赤と青が入れ替わっている | `kPanelRgbOrder` を反転 |
  | 画面が90°ずれる・端が切れる | `kDisplayRotation` を 1↔3 で変更 |
  | 上下だけ反転している（左右は正しい） | `kDisplayRotation` に +4（上下反転フラグ） |
  | 表示は正常だがタッチ位置がずれる | `-DCYD_TOUCH_CALIBRATION` で四隅補正値を取得 |

判別できたら、このファイルに「確認日・シリアル出力の実値・採用したビルドフラグ」を追記する。

### 写真について
ピン配置の説明には不要（型番で確定する）。記録として残すなら `docs/images/` に
①基板表（USB端子が1個か2個か分かる画角）②基板裏のシルク全体 を置くと、
後でボードのリビジョン違いを追う際の材料になる。ドライバICの判別には使えない。

### 開発環境（WSL2 Ubuntu / Windows ホスト）
| 項目 | 実測値 | 用途 |
|---|---|---|
| g++ | 15.2.0 (Ubuntu) | ホスト単体テスト（`make check`） |
| GNU Make | 4.4.1 | ホストビルド |
| arduino-cli | 1.5.1（Windows 側 `C:\Program Files\Arduino CLI\arduino-cli.exe`） | 実機ビルド候補 |
| PlatformIO | 未インストール（pip も無し） | 採用しない方針（下記） |
| 既存コア | `arduino:avr 1.8.8`, `m5stack:esp32 3.3.9` | CYD には不足 |
| 既存ライブラリ | IRremote, IRremoteESP8266, LibSSH-ESP32, M5Cardputer, M5GFX, M5Unified | CYD用表示ライブラリは未導入 |

- 既存の `git-repos/cardputer-arduino.git` が同じ WSL→Windows interop で arduino-cli を叩く構成だったため、
  本プロジェクトも arduino-cli を採用候補とする（README の初期案にあった PlatformIO は未インストールで、
  環境を二重管理する理由がない）。
- CYD（ESP32-2432S028R系）は Espressif の `esp32:esp32` コアが必要で**未インストール**。
  表示ライブラリ（LovyanGFX / TFT_eSPI など）も未導入。導入は Windows 側の共有 Arduino 環境を変えるため人間の承認後に行う。
- Phase 1/2 は上記実機環境に一切依存せず、`g++` + `make` のみで検証できる。

## 実機ビルド環境（2026-10-11 導入、人間の承認済み）
| 項目 | バージョン | 備考 |
|---|---|---|
| arduino-cli | 1.5.1 | Windows 側 `C:\Program Files\Arduino CLI\arduino-cli.exe` |
| esp32:esp32（ボードマネージャ） | 3.3.12 | 追加URL `https://espressif.github.io/arduino-esp32/package_esp32_index.json` |
| LovyanGFX | 1.2.28 | ST7789 / XPT2046 をコード内設定で扱えるため採用（TFT_eSPI の `User_Setup.h` 編集を避ける） |
| FQBN | `esp32:esp32:esp32` | ESP32 Dev Module |

導入コマンド（再現用）:
```powershell
$cli='C:\Program Files\Arduino CLI\arduino-cli.exe'
$url='https://espressif.github.io/arduino-esp32/package_esp32_index.json'
& $cli core update-index --additional-urls $url
& $cli core install esp32:esp32@3.3.12 --additional-urls $url
& $cli lib install LovyanGFX@1.2.28
# 導入後に版を確認する（記録と一致しなければ、この手順かバージョン表のどちらかが古い）
& $cli core list
& $cli lib list LovyanGFX
```
バージョンを固定しない `core install esp32:esp32` では新しい版が入り、記録と異なる成果物になる。

ビルド（WSL のソースを UNC パス経由で参照。ビルド成果物は Windows 側一時ディレクトリへ出す）:
```powershell
& $cli compile --fqbn esp32:esp32:esp32 `
  --build-path 'C:\Users\ishid\AppData\Local\Temp\cyd_build' `
  '\\wsl.localhost\Ubuntu\home\ishida\cyd_life_counter_md'
```

書き込み（COMポートは実機接続時に `arduino-cli board list` で確認）:
```powershell
& $cli upload --fqbn esp32:esp32:esp32 -p COM<N> '\\wsl.localhost\Ubuntu\home\ishida\cyd_life_counter_md'
```

タッチ補正値の取得（四隅タッチ→シリアル出力→`src/platform/cyd/board_config.h` と本ファイルへ記録）:
```powershell
& $cli compile --fqbn esp32:esp32:esp32 `
  --build-property build.extra_flags=-DCYD_TOUCH_CALIBRATION `
  '\\wsl.localhost\Ubuntu\home\ishida\cyd_life_counter_md'
```

### ビルド実績（2026-10-11、`--warnings all` で警告0）
| ビルド | Flash | RAM(グローバル) |
|---|---|---|
| 既定（ST7789） | 339,043 B (25%) | 23,744 B (7%) |
| `-DCYD_PANEL_ILI9341` | 351,227 B (26%) | 23,872 B (7%) |
| `-DCYD_TOUCH_CALIBRATION` | 352,227 B (26%) | 23,872 B (7%) |

いずれも ESP32 の容量に対して余裕がある（Flash 26%以下、RAM 7%）。

四隅補正の使い方: 補正モードで書き込み→四隅をタッチ→シリアル(115200)に出る8値を
`src/platform/cyd/board_config.h` の `kTouchCalibration` へ転記し `kUseTouchCalibration = true` にする→通常モードで再ビルド。

### 未検証の前提（実機で必ず確認する）
- ピン定義一式（表示: SCLK14/MOSI13/MISO12/CS15/DC2/BL21、タッチ: SCLK25/MOSI32/MISO39/CS33/IRQ36）。
- パネルドライバ ST7789 の前提、`invert=true`、`rgb_order=false`、回転1で 320×240 になること。
- タッチ生値レンジ（暫定 300〜3900）。

### 書き込みの注意（2026-10-11 実機で判明）
この個体は**自動リセット回路でダウンロードモードに入らない**。`arduino-cli upload` だけでは
`Wrong boot mode detected (0x13)! The chip needs to be in download mode.` で失敗する。

手順: **BOOTボタンを押したまま** `upload` を実行する。押し続けたままで完走する。
```powershell
& $cli upload --fqbn 'esp32:esp32:esp32:UploadSpeed=230400' `
  --input-dir 'C:\Users\ishid\AppData\Local\Temp\cyd_build' -p COM4 `
  '\\wsl.localhost\Ubuntu\home\ishida\cyd_life_counter_md'
```
- COMポート: COM4（CH340、VID 0x1A86 / PID 0x7523）
- `UploadSpeed` の有効値は 921600 / 512000 / 256000 / 230400 / 115200。230400 で成功（297 kbit/s、9.1秒）
- 書き込み後はチップがダウンロードモードに留まるため、**RSTボタンまたはUSB抜き差しでファームウェアが起動する**
- 起動ログの取得は `tools/capture_serial.ps1`（Windows側で実行）
