#include <gtest/gtest.h>

// Demonstrate some basic assertions.
TEST(libtest, BasicAssertions) {
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
    EXPECT_NE(7 * 6, 4);
}