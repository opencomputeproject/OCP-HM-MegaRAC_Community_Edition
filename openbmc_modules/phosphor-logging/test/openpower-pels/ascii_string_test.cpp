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
#include "extensions/openpower-pels/ascii_string.hpp"
#include "extensions/openpower-pels/registry.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;

TEST(AsciiStringTest, AsciiStringTest)
{
    // Build the ASCII string from a message registry entry
    message::Entry entry;
    entry.src.type = 0xBD;
    entry.src.reasonCode = 0xABCD;
    entry.subsystem = 0x37;

    src::AsciiString as{entry};

    auto data = as.get();

    EXPECT_EQ(data, "BD37ABCD                        ");

    // Now flatten it
    std::vector<uint8_t> flattenedData;
    Stream stream{flattenedData};

    as.flatten(stream);

    for (size_t i = 0; i < 32; i++)
    {
        EXPECT_EQ(data[i], flattenedData[i]);
    }
}

// A 0x11 power SRC doesn't have the subsystem in it
TEST(AsciiStringTest, PowerErrorTest)
{
    message::Entry entry;
    entry.src.type = 0x11;
    entry.src.reasonCode = 0xABCD;
    entry.subsystem = 0x37;

    src::AsciiString as{entry};
    auto data = as.get();

    EXPECT_EQ(data, "1100ABCD                        ");
}

TEST(AsciiStringTest, UnflattenTest)
{
    std::vector<uint8_t> rawData{'B', 'D', '5', '6', '1', '2', 'A', 'B'};

    for (int i = 8; i < 32; i++)
    {
        rawData.push_back(' ');
    }

    Stream stream{rawData};
    src::AsciiString as{stream};

    auto data = as.get();

    EXPECT_EQ(data, "BD5612AB                        ");
}

TEST(AsciiStringTest, UnderflowTest)
{
    std::vector<uint8_t> rawData{'B', 'D', '5', '6'};
    Stream stream{rawData};

    EXPECT_THROW(src::AsciiString as{stream}, std::out_of_range);
}
