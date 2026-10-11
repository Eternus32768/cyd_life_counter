# ホスト（PC）側のビルド・テスト。実機ビルドは arduino-cli（docs/HARDWARE.md 参照）。

CXX ?= g++
CXXSTD ?= -std=c++17
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror
CXXFLAGS ?= $(CXXSTD) $(WARNINGS) -O2 -g

BUILD_DIR := build/host
LIB_SRCS := $(wildcard src/domain/*.cpp src/application/*.cpp src/ui/*.cpp)
TEST_SRCS := $(wildcard tests/host/*.cpp)
SRCS := $(LIB_SRCS) $(TEST_SRCS)
OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
TEST_BIN := $(BUILD_DIR)/run_tests

.PHONY: all test test-asan check check-layers domain-only clean

all: $(TEST_BIN)

# 既定の検証一式: レイヤ境界チェック → 警告ゼロビルド → テスト → サニタイザ実行
check: check-layers domain-only test test-asan

test: $(TEST_BIN)
	$(TEST_BIN)

$(TEST_BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# アドレス/未定義動作サニタイザ付きで再ビルドして実行（配列外参照・未定義動作の検出）
# -fno-sanitize-recover=undefined: UBSan は既定で検出後も継続し終了コード0になり得るため停止させる
test-asan:
	@mkdir -p $(BUILD_DIR)/asan
	$(CXX) $(CXXSTD) $(WARNINGS) -O1 -g -fsanitize=address,undefined \
		-fno-sanitize-recover=undefined -fno-omit-frame-pointer \
		$(SRCS) -o $(BUILD_DIR)/asan/run_tests
	$(BUILD_DIR)/asan/run_tests

# domain が標準C++のみで単独ビルドできることを確認（Arduino/ESP32/表示ライブラリ不要）
domain-only:
	@mkdir -p $(BUILD_DIR)/domain_only
	$(CXX) $(CXXFLAGS) -c src/domain/life_state.cpp -o $(BUILD_DIR)/domain_only/life_state.o
	@echo "domain-only build ok"

check-layers:
	@tools/check_layers.sh

clean:
	rm -rf build

-include $(DEPS)
