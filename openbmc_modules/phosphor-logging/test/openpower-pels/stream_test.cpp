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
#include "extensions/openpower-pels/stream.hpp"

#include <iostream>

#include <gtest/gtest.h>

using namespace openpower::pels;

TEST(StreamTest, TestExtract)
{
    std::vector<uint8_t> data{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                              0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                              0x08, 'h',  'e',  'l',  'l',  'o'};
    Stream stream{data};

    {
        uint8_t v;
        stream >> v;
        EXPECT_EQ(v, 0x11);
    }
    {
        uint16_t v;
        stream >> v;
        EXPECT_EQ(v, 0x2233);
    }
    {
        uint32_t v;
        stream >> v;
        EXPECT_EQ(v, 0x44556677);
    }
    {
        uint64_t v;
        stream >> v;
        EXPECT_EQ(v, 0x0102030405060708);
    }
    {
        char v[6] = {0};
        stream.read(v, 5);
        EXPECT_EQ(memcmp(v, "hello", 5), 0);
    }

    EXPECT_EQ(stream.remaining(), 0);

    // At the end, so should throw.
    uint8_t v;
    EXPECT_THROW(stream >> v, std::out_of_range);
}

TEST(StreamTest, InputTestNoExpansion)
{
    std::vector<uint8_t> data(256, 0);
    Stream stream(data);
    uint8_t v1 = 0x11;
    uint16_t v2 = 0x2233;
    uint64_t v3 = 0x445566778899AABB;
    uint32_t v4 = 0xCCDDEEFF;

    stream << v3 << v2 << v4 << v1;

    uint8_t e1;
    uint16_t e2;
    uint64_t e3;
    uint32_t e4;

    stream.offset(0);
    stream >> e3 >> e2 >> e4 >> e1;

    EXPECT_EQ(v1, e1);
    EXPECT_EQ(v2, e2);
    EXPECT_EQ(v3, e3);
    EXPECT_EQ(v4, e4);
}

TEST(StreamTest, InputTestExpansion)
{
    // The stream will expand the underlying vector
    std::vector<uint8_t> data;
    Stream stream(data);

    uint32_t v1 = 0xAABBCCDD;
    stream << v1;

    stream.offset(0);
    uint32_t e1;
    stream >> e1;
    EXPECT_EQ(data.size(), 4);
    EXPECT_EQ(v1, e1);

    stream.offset(2);

    uint64_t v2 = 0x0102030405060708;
    stream << v2;

    EXPECT_EQ(data.size(), 10);
    uint64_t e2;
    stream.offset(2);
    stream >> e2;

    EXPECT_EQ(v2, e2);

    auto origSize = data.size();
    uint8_t v3 = 0xCC;
    stream << v3;

    EXPECT_EQ(origSize + 1, data.size());
    stream.offset(stream.offset() - 1);
    uint8_t e3;
    stream >> e3;
    EXPECT_EQ(v3, e3);
}

TEST(StreamTest, ReadWriteTest)
{
    std::vector<uint8_t> data{0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                              0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
    Stream stream{data};
    uint8_t buf[data.size()];

    stream.read(buf, data.size());

    for (size_t i = 0; i < data.size(); i++)
    {
        EXPECT_EQ(buf[i], data[i]);

        // for the next test
        buf[i] = 0x20 + i;
    }

    stream.offset(6);
    stream.write(buf, 6);
    for (size_t i = 0; i < 6; i++)
    {
        EXPECT_EQ(buf[i], data[i + 6]);
    }
}

TEST(StreamTest, TestOffsets)
{
    std::vector<uint8_t> data{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    Stream stream{data, 3};

    {
        uint8_t v;
        stream >> v;
        EXPECT_EQ(v, 0x44);
        EXPECT_EQ(stream.offset(), 4);
    }

    stream.offset(6);

    {
        uint8_t v;
        stream >> v;
        EXPECT_EQ(v, 0x77);
        EXPECT_EQ(stream.offset(), 7);
        EXPECT_EQ(stream.remaining(), 0);
    }

    EXPECT_THROW(stream.offset(100), std::out_of_range);
}

TEST(StreamTest, TestVectorInsertExtract)
{
    std::vector<uint8_t> toInsert{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    std::vector<uint8_t> data;

    // Insert
    Stream stream{data};
    stream << toInsert;
    EXPECT_EQ(data, toInsert);

    // Extract
    std::vector<uint8_t> toExtract;
    toExtract.resize(toInsert.size());
    stream.offset(0);
    stream >> toExtract;

    EXPECT_EQ(data, toExtract);

    // Go off the end
    EXPECT_THROW(stream >> toExtract, std::out_of_range);
}
