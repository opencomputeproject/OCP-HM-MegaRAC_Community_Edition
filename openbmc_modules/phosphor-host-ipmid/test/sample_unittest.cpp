#include "sample.h"

#include <gtest/gtest.h>

TEST(FactorialTest, Zero)
{
    EXPECT_EQ(1, Factorial(0));
}
