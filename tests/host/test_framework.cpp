#include "test_framework.h"

#include <cstdio>
#include <vector>

namespace testing {
namespace {

struct TestCase {
    const char* name;
    TestFn fn;
};

std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

int g_current_failures = 0;

}  // namespace

bool register_test(const char* name, TestFn fn) {
    registry().push_back(TestCase{name, fn});
    return true;
}

void report_failure(const char* file, int line, const std::string& message) {
    ++g_current_failures;
    std::printf("    FAIL %s:%d  %s\n", file, line, message.c_str());
}

int run_all() {
    int failed_tests = 0;
    for (const TestCase& test : registry()) {
        g_current_failures = 0;
        test.fn();
        if (g_current_failures == 0) {
            std::printf("[  ok  ] %s\n", test.name);
        } else {
            std::printf("[ FAIL ] %s (%d checks failed)\n", test.name, g_current_failures);
            ++failed_tests;
        }
    }
    std::printf("\n%zu tests, %d failed\n", registry().size(), failed_tests);
    return failed_tests == 0 ? 0 : 1;
}

}  // namespace testing

int main() { return testing::run_all(); }
