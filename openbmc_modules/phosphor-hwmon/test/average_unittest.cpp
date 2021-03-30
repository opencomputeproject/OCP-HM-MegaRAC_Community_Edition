#include "average.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;

TEST(SensorKeyTest, InvalidSensorKey)
{
    Average av;

    av.setAverageValue(std::make_pair("power", "0"), std::make_pair(0L, 0L));
    av.setAverageValue(std::make_pair("power", "1"), std::make_pair(0L, 0L));

    EXPECT_FALSE(av.getAverageValue(std::make_pair("power", "4")));
}

TEST(SensorKeyTest, ValidSensorKey)
{
    Average av;

    av.setAverageValue(std::make_pair("power", "0"), std::make_pair(0L, 0L));
    av.setAverageValue(std::make_pair("power", "1"), std::make_pair(2L, 2L));

    auto value = av.getAverageValue(std::make_pair("power", "1"));
    auto expected = Average::averageValue(2, 2);
    EXPECT_TRUE(value == expected);
}

TEST(AverageTest, ZeroDelta)
{
    Average av;

    EXPECT_FALSE(av.calcAverage(1L, 1L, 2L, 1L));
}

TEST(AverageTest, NegativeDelta)
{
    Average av;

    ASSERT_DEATH(av.calcAverage(1L, 1L, 2L, 0L), "");
}

TEST(AverageTest, RightAverage)
{
    Average av;

    EXPECT_TRUE(38837438L == av.calcAverage(27624108L, 132864155500L, 27626120L,
                                            132887999500L));
}
