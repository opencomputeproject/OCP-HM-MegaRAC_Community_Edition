// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "config.hpp"

#include <gtest/gtest.h>

// Names of environment variables
static const char* SOCKET_ID = "SOCKET_ID";
static const char* BUF_MAXSIZE = "BUF_MAXSIZE";
static const char* BUF_MAXTIME = "BUF_MAXTIME";
static const char* FLUSH_FULL = "FLUSH_FULL";
static const char* HOST_STATE = "HOST_STATE";
static const char* OUT_DIR = "OUT_DIR";
static const char* MAX_FILES = "MAX_FILES";

/**
 * @class ConfigTest
 * @brief Configuration tests.
 */
class ConfigTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        resetEnv();
    }

    void TearDown() override
    {
        resetEnv();
    }

    /** @brief Reset environment variables. */
    void resetEnv() const
    {
        unsetenv(SOCKET_ID);
        unsetenv(BUF_MAXSIZE);
        unsetenv(BUF_MAXTIME);
        unsetenv(FLUSH_FULL);
        unsetenv(HOST_STATE);
        unsetenv(OUT_DIR);
        unsetenv(MAX_FILES);
    }
};

TEST_F(ConfigTest, Defaults)
{
    Config cfg;
    EXPECT_STREQ(cfg.socketId, "");
    EXPECT_EQ(cfg.bufMaxSize, 3000);
    EXPECT_EQ(cfg.bufMaxTime, 0);
    EXPECT_EQ(cfg.bufFlushFull, false);
    EXPECT_STREQ(cfg.hostState, "/xyz/openbmc_project/state/host0");
    EXPECT_STREQ(cfg.outDir, "/var/lib/obmc/hostlogs");
    EXPECT_EQ(cfg.maxFiles, 10);
}

TEST_F(ConfigTest, Load)
{
    setenv(SOCKET_ID, "id123", 1);
    setenv(BUF_MAXSIZE, "1234", 1);
    setenv(BUF_MAXTIME, "4321", 1);
    setenv(FLUSH_FULL, "true", 1);
    setenv(HOST_STATE, "host123", 1);
    setenv(OUT_DIR, "path123", 1);
    setenv(MAX_FILES, "1122", 1);

    Config cfg;
    EXPECT_STREQ(cfg.socketId, "id123");
    EXPECT_EQ(cfg.bufMaxSize, 1234);
    EXPECT_EQ(cfg.bufMaxTime, 4321);
    EXPECT_EQ(cfg.bufFlushFull, true);
    EXPECT_STREQ(cfg.hostState, "host123");
    EXPECT_STREQ(cfg.outDir, "path123");
    EXPECT_EQ(cfg.maxFiles, 1122);
}

TEST_F(ConfigTest, InvalidNumeric)
{
    setenv(BUF_MAXSIZE, "-1234", 1);
    ASSERT_THROW(Config(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidBoolean)
{
    setenv(FLUSH_FULL, "invalid", 1);
    ASSERT_THROW(Config(), std::invalid_argument);
}

TEST_F(ConfigTest, InvalidConfig)
{
    setenv(BUF_MAXSIZE, "0", 1);
    setenv(BUF_MAXTIME, "0", 1);
    setenv(FLUSH_FULL, "true", 1);
    ASSERT_THROW(Config(), std::invalid_argument);
}
