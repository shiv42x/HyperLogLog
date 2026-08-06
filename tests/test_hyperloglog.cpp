#include <gtest/gtest.h>
#include "hyperloglog/hyperloglog.hpp"

TEST(HyperLogLogTest, AddTwoNumbers) {
    EXPECT_EQ(hll::add(2, 3), 5);
}