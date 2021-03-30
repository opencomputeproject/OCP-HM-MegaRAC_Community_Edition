// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "file_storage.hpp"

#include <fstream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

/**
 * @class FileStorageTest
 * @brief Persistent file storage tests.
 */
class FileStorageTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        fs::remove_all(logPath);
    }

    void TearDown() override
    {
        fs::remove_all(logPath);
    }

    const fs::path logPath =
        fs::temp_directory_path() / "file_storage_test_out";
};

TEST_F(FileStorageTest, InvalidPath)
{
    ASSERT_THROW(FileStorage("", "", 0), std::invalid_argument);
    ASSERT_THROW(FileStorage("relative/path", "", 0), std::invalid_argument);
    ASSERT_THROW(FileStorage("/noaccess", "", 0), fs::filesystem_error);
}

TEST_F(FileStorageTest, Save)
{
    const char* data = "test message\n";
    LogBuffer buf(0, 0);
    buf.append(data, strlen(data));

    FileStorage fs(logPath, "", 0);
    fs.save(buf);

    const auto itBegin = fs::recursive_directory_iterator(logPath);
    const auto itEnd = fs::recursive_directory_iterator{};
    ASSERT_EQ(std::distance(itBegin, itEnd), 1);

    const fs::path file = *fs::directory_iterator(logPath);
    EXPECT_NE(fs::file_size(file), 0);
}

TEST_F(FileStorageTest, Rotation)
{
    const size_t limit = 5;
    const std::string prefix = "host123";

    const char* data = "test message\n";
    LogBuffer buf(0, 0);
    buf.append(data, strlen(data));

    FileStorage fs(logPath, prefix, limit);
    for (size_t i = 0; i < limit + 3; ++i)
    {
        fs.save(buf);
    }

    // Dir and other files that can not be removed
    const fs::path dir = logPath / (prefix + "_11111111_222222.log.gz");
    const fs::path files[] = {logPath / "short",
                              logPath / (prefix + "_11111111_222222.bad.ext"),
                              logPath / (prefix + "x_11111111_222222.log.gz")};
    fs::create_directory(dir);
    for (const auto& i : files)
    {
        std::ofstream dummy(i);
    }

    const auto itBegin = fs::recursive_directory_iterator(logPath);
    const auto itEnd = fs::recursive_directory_iterator{};
    EXPECT_EQ(std::distance(itBegin, itEnd),
              limit + 1 /*dir*/ + sizeof(files) / sizeof(files[0]));
    EXPECT_TRUE(fs::exists(dir));
    for (const auto& i : files)
    {
        EXPECT_TRUE(fs::exists(i));
    }
}
