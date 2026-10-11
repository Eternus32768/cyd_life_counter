#!/usr/bin/env bash
# docs/ARCHITECTURE.md の依存境界を機械的に検査する。
# 防止したい失敗: domain/application/ui に実機依存が混入し、ホストテスト不能になること。
# 撤去条件: 同等の検査を静的解析ツール（clang-tidy等）で代替できたとき。
set -uo pipefail

status=0

# 行コメント（//）を除いたコードだけを対象に検索する。
scan() {
    local pattern="$1"
    shift
    local file
    while IFS= read -r -d '' file; do
        sed -E 's#//.*##' "$file" | grep -nE "$pattern" | sed "s#^#${file}:#"
    done < <(find "$@" -type f \( -name '*.h' -o -name '*.cpp' \) -print0 | sort -z)
}

expect_no_match() {
    local message="$1"
    local pattern="$2"
    shift 2
    local hits
    hits="$(scan "$pattern" "$@")"
    if [ -n "$hits" ]; then
        echo "NG: $message"
        echo "$hits"
        status=1
    fi
}

expect_no_match "ハード依存の参照がロジック層にある" \
    'Arduino\.h|ESP32|esp_|TFT_eSPI|LovyanGFX|M5[A-Z]|XPT2046|ILI9341|ST7789|digitalWrite|pinMode' \
    src/domain src/application src/ui

expect_no_match "domain が外側の層を include している" \
    '#include .*(application|ui|platform)/' src/domain

expect_no_match "application が ui / platform を include している" \
    '#include .*(ui|platform)/' src/application

expect_no_match "ui が platform を include している" \
    '#include .*platform/' src/ui

# 画面ピクセル寸法はUIへ注入する。ロジック層に直書きしない。
expect_no_match "画面寸法がロジック層に混入している" \
    '\b(320|240)\b' src/domain src/application

if [ "$status" -eq 0 ]; then
    echo "layer check ok"
fi
exit "$status"
