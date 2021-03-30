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
#include "extensions/openpower-pels/log_id.hpp"
#include "extensions/openpower-pels/paths.hpp"

#include <arpa/inet.h>

#include <chrono>
#include <filesystem>
#include <thread>

#include <gtest/gtest.h>

using namespace openpower::pels;
namespace fs = std::filesystem;

TEST(LogIdTest, TimeBasedIDTest)
{
    uint32_t lastID = 0;
    for (int i = 0; i < 10; i++)
    {
        auto id = detail::getTimeBasedLogID();

        EXPECT_EQ(id & 0xFF000000, 0x50000000);
        EXPECT_NE(id, lastID);
        lastID = id;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TEST(LogIdTest, IDTest)
{
    EXPECT_EQ(generatePELID(), 0x50000001);
    EXPECT_EQ(generatePELID(), 0x50000002);
    EXPECT_EQ(generatePELID(), 0x50000003);
    EXPECT_EQ(generatePELID(), 0x50000004);
    EXPECT_EQ(generatePELID(), 0x50000005);
    EXPECT_EQ(generatePELID(), 0x50000006);

    auto backingFile = getPELIDFile();
    fs::remove(backingFile);
    EXPECT_EQ(generatePELID(), 0x50000001);
    EXPECT_EQ(generatePELID(), 0x50000002);
    EXPECT_EQ(generatePELID(), 0x50000003);

    fs::remove_all(fs::path{backingFile}.parent_path());
}
