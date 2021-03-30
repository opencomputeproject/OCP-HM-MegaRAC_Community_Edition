#include "hwmon.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(HwmonTest, InvalidType)
{

    hwmon::Attributes attrs;
    EXPECT_FALSE(hwmon::getAttributes("invalid", attrs));
}
