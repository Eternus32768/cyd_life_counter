#pragma once

// 最小のホストテスト基盤。外部依存を増やさないため自作する。

#include <cstring>
#include <sstream>
#include <string>

namespace testing {

using TestFn = void (*)();

bool register_test(const char* name, TestFn fn);
void report_failure(const char* file, int line, const std::string& message);
int run_all();

template <typename A, typename B>
void check_eq(const A& actual, const B& expected, const char* actual_expr,
              const char* expected_expr, const char* file, int line) {
    if (!(actual == expected)) {
        std::ostringstream out;
        out << actual_expr << " == " << expected_expr << " : actual=" << actual
            << " expected=" << expected;
        report_failure(file, line, out.str());
    }
}

inline void check_streq(const char* actual, const char* expected, const char* actual_expr,
                        const char* file, int line) {
    const bool equal = actual != nullptr && expected != nullptr && std::strcmp(actual, expected) == 0;
    if (!equal) {
        std::ostringstream out;
        out << actual_expr << " : actual=\"" << (actual != nullptr ? actual : "(null)")
            << "\" expected=\"" << (expected != nullptr ? expected : "(null)") << "\"";
        report_failure(file, line, out.str());
    }
}

}  // namespace testing

#define TEST(test_name)                                                     \
    static void test_name();                                                \
    static const bool test_name##_registered =                              \
        ::testing::register_test(#test_name, test_name);                    \
    static void test_name()

#define CHECK_TRUE(expr)                                                    \
    do {                                                                    \
        if (!(expr)) {                                                      \
            ::testing::report_failure(__FILE__, __LINE__, "expected true: " #expr); \
        }                                                                   \
    } while (false)

#define CHECK_FALSE(expr)                                                   \
    do {                                                                    \
        if (expr) {                                                         \
            ::testing::report_failure(__FILE__, __LINE__, "expected false: " #expr); \
        }                                                                   \
    } while (false)

#define CHECK_EQ(actual, expected) \
    ::testing::check_eq((actual), (expected), #actual, #expected, __FILE__, __LINE__)

#define CHECK_STREQ(actual, expected) \
    ::testing::check_streq((actual), (expected), #actual, __FILE__, __LINE__)
