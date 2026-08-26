#include <gtest/gtest.h>
#include "hyperloglog/hyperloglog.hpp"

TEST(HLLTest, BasicTest) {
    hll::HyperLogLog<std::string> hll(4);
    ASSERT_EQ(hll.estimate(), 0);

    std::vector<std::string> test_strings = {
        "apple", "banana", "cherry", "date", "elderberry",
        "fig", "grape", "honeydew", "kiwi", "lemon",
        "apple", "banana", "cherry"
    };

    for (const auto& str: test_strings) {
        hll.add(str);
    }

    uint64_t estimate = hll.estimate();

    EXPECT_GT(estimate, 0);
}