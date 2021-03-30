// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "zlib_exception.hpp"
#include "zlib_file.hpp"

#include <gtest/gtest.h>

TEST(ZlibFileTest, Exception)
{
    ASSERT_THROW(ZlibFile("invalid/path"), ZlibException);
}

TEST(ZlibFileTest, Write)
{
    const std::string msg = "Test message";
    time_t currTime;
    time(&currTime);
    tm localTime;
    localtime_r(&currTime, &localTime);

    const std::string path = "/tmp/zlib_file_test.out";
    ZlibFile file(path);
    file.write(localTime, msg);
    file.close();

    char expect[64];
    const int len = snprintf(expect, sizeof(expect), "[ %02i:%02i:%02i ] %s\n",
                             localTime.tm_hour, localTime.tm_min,
                             localTime.tm_sec, msg.c_str());

    gzFile fd = gzopen(path.c_str(), "r");
    ASSERT_TRUE(fd);
    char buf[64];
    memset(buf, 0, sizeof(buf));
    EXPECT_EQ(gzread(fd, buf, sizeof(buf)), len);
    EXPECT_STREQ(buf, expect);
    EXPECT_EQ(gzclose(fd), 0);

    unlink(path.c_str());
}
