#include "libpldm/base.h"
#include "libpldm/bios.h"

#include "libpldmresponder/bios.hpp"
#include "libpldmresponder/bios_table.hpp"

#include <string.h>

#include <array>
#include <cstring>
#include <ctime>
#include <filesystem>

#include <gtest/gtest.h>

using namespace pldm;
using namespace pldm::responder;
using namespace pldm::responder::bios;
using namespace pldm::responder::utils;

TEST(epochToBCDTime, testTime)
{
    struct tm time
    {};
    time.tm_year = 119;
    time.tm_mon = 3;
    time.tm_mday = 13;
    time.tm_hour = 5;
    time.tm_min = 18;
    time.tm_sec = 13;
    time.tm_isdst = -1;

    time_t epochTime = mktime(&time);
    uint8_t seconds = 0;
    uint8_t minutes = 0;
    uint8_t hours = 0;
    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0;

    epochToBCDTime(epochTime, seconds, minutes, hours, day, month, year);

    ASSERT_EQ(0x13, seconds);
    ASSERT_EQ(0x18, minutes);
    ASSERT_EQ(0x5, hours);
    ASSERT_EQ(0x13, day);
    ASSERT_EQ(0x4, month);
    ASSERT_EQ(0x2019, year);
}

TEST(timeToEpoch, testTime0)
{
    std::time_t ret = 1555132693;

    uint8_t sec = 13;
    uint8_t min = 18;
    uint8_t hours = 5;
    uint8_t day = 13;
    uint8_t month = 4;
    uint16_t year = 2019;

    std::time_t timeSec = 0;
    timeSec = timeToEpoch(sec, min, hours, day, month, year);

    EXPECT_EQ(ret, timeSec);
}