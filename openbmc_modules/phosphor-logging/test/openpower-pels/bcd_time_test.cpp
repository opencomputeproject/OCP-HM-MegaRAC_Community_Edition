/**
 * Copyright © 2019 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "extensions/openpower-pels/bcd_time.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;

TEST(BCDTimeTest, ToBCDTest)
{
    EXPECT_EQ(toBCD(0), 0x00);
    EXPECT_EQ(toBCD(1), 0x01);
    EXPECT_EQ(toBCD(10), 0x10);
    EXPECT_EQ(toBCD(99), 0x99);
    EXPECT_EQ(toBCD(37), 0x37);
    EXPECT_EQ(toBCD(60), 0x60);
    EXPECT_EQ(toBCD(12345678), 0x12345678);
    EXPECT_EQ(toBCD(0xF), 0x15);
}

TEST(BCDTimeTest, FlattenUnflattenTest)
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8};
    Stream stream{data};
    BCDTime bcd;

    // Unflatten
    stream >> bcd;

    EXPECT_EQ(bcd.yearMSB, 1);
    EXPECT_EQ(bcd.yearLSB, 2);
    EXPECT_EQ(bcd.month, 3);
    EXPECT_EQ(bcd.day, 4);
    EXPECT_EQ(bcd.hour, 5);
    EXPECT_EQ(bcd.minutes, 6);
    EXPECT_EQ(bcd.seconds, 7);
    EXPECT_EQ(bcd.hundredths, 8);

    // Flatten
    uint8_t val = 0x20;
    bcd.yearMSB = val++;
    bcd.yearLSB = val++;
    bcd.month = val++;
    bcd.day = val++;
    bcd.hour = val++;
    bcd.minutes = val++;
    bcd.seconds = val++;
    bcd.hundredths = val++;

    stream.offset(0);
    stream << bcd;

    for (size_t i = 0; i < 8; i++)
    {
        EXPECT_EQ(data[i], 0x20 + i);
    }
}

TEST(BCDTimeTest, ConvertTest)
{
    // Convert a time_point into BCDTime
    tm time_tm;
    time_tm.tm_year = 125;
    time_tm.tm_mon = 11;
    time_tm.tm_mday = 31;
    time_tm.tm_hour = 15;
    time_tm.tm_min = 23;
    time_tm.tm_sec = 42;
    time_tm.tm_isdst = 0;

    auto timepoint = std::chrono::system_clock::from_time_t(mktime(&time_tm));
    auto timeInBCD = getBCDTime(timepoint);

    EXPECT_EQ(timeInBCD.yearMSB, 0x20);
    EXPECT_EQ(timeInBCD.yearLSB, 0x25);
    EXPECT_EQ(timeInBCD.month, 0x12);
    EXPECT_EQ(timeInBCD.day, 0x31);
    EXPECT_EQ(timeInBCD.hour, 0x15);
    EXPECT_EQ(timeInBCD.minutes, 0x23);
    EXPECT_EQ(timeInBCD.seconds, 0x42);
    EXPECT_EQ(timeInBCD.hundredths, 0x00);
}

TEST(BCDTimeTest, ConvertFromMSTest)
{
    auto now = std::chrono::system_clock::now();
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch())
                      .count();

    ASSERT_EQ(getBCDTime(now), getBCDTime(ms));
}
