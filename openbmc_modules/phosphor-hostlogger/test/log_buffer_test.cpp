// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "log_buffer.hpp"

#include <gtest/gtest.h>

TEST(LogBufferTest, Append)
{
    const std::string msg = "Test message";

    LogBuffer buf(0, 0);

    buf.append(msg.data(), msg.length());
    ASSERT_EQ(std::distance(buf.begin(), buf.end()), 1);
    EXPECT_EQ(buf.begin()->text, msg);
    EXPECT_NE(buf.begin()->timeStamp, 0);

    // must be merged with previous message
    const std::string append = "Append";
    buf.append(append.data(), append.length());
    ASSERT_EQ(std::distance(buf.begin(), buf.end()), 1);
    EXPECT_EQ(buf.begin()->text, msg + append);

    // end of line, we still have 1 message
    buf.append("\n", 1);
    ASSERT_EQ(std::distance(buf.begin(), buf.end()), 1);

    // second message
    buf.append(append.data(), append.length());
    ASSERT_EQ(std::distance(buf.begin(), buf.end()), 2);
    EXPECT_EQ((++buf.begin())->text, append);
}

TEST(LogBufferTest, AppendEol)
{
    LogBuffer buf(0, 0);

    buf.append("\r\r\r\r", 4);
    EXPECT_EQ(std::distance(buf.begin(), buf.end()), 4);

    buf.clear();
    buf.append("\n\n\n\n", 4);
    EXPECT_EQ(std::distance(buf.begin(), buf.end()), 4);

    buf.clear();
    buf.append("\r\n\r\n", 4);
    EXPECT_EQ(std::distance(buf.begin(), buf.end()), 2);

    buf.clear();
    buf.append("\n\r\n\r", 4);
    EXPECT_EQ(std::distance(buf.begin(), buf.end()), 2);

    buf.clear();
    buf.append("\r\r\r\n\n\n", 6);
    EXPECT_EQ(std::distance(buf.begin(), buf.end()), 5);
}

TEST(LogBufferTest, Clear)
{
    const std::string msg = "Test message";

    LogBuffer buf(0, 0);
    buf.append(msg.data(), msg.length());
    EXPECT_FALSE(buf.empty());
    buf.clear();
    EXPECT_TRUE(buf.empty());
}

TEST(LogBufferTest, SizeLimit)
{
    const size_t limit = 5;
    const std::string msg = "Test message\n";

    LogBuffer buf(limit, 0);
    for (size_t i = 0; i < limit + 3; ++i)
    {
        buf.append(msg.data(), msg.length());
    }
    EXPECT_EQ(std::distance(buf.begin(), buf.end()), limit);
}

TEST(LogBufferTest, FullHandler)
{
    const size_t limit = 5;
    const std::string msg = "Test message\n";

    size_t count = 0;

    LogBuffer buf(limit, 0);
    buf.setFullHandler([&count, &buf]() {
        ++count;
        buf.clear();
    });
    for (size_t i = 0; i < limit + 3; ++i)
    {
        buf.append(msg.data(), msg.length());
    }
    EXPECT_EQ(count, 1);
    EXPECT_EQ(std::distance(buf.begin(), buf.end()), 2);
}
