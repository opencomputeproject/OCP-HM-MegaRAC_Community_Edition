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
#include "extensions/openpower-pels/user_data.hpp"
#include "pel_utils.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;

std::vector<uint8_t> udSectionData{0x55, 0x44, // ID 'UD'
                                   0x00, 0x10, // Size
                                   0x01, 0x02, // version, subtype
                                   0x03, 0x04, // comp ID

                                   // Data
                                   0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                   0x18};

TEST(UserDataTest, UnflattenFlattenTest)
{
    Stream stream(udSectionData);
    UserData ud(stream);

    EXPECT_TRUE(ud.valid());
    EXPECT_EQ(ud.header().id, 0x5544);
    EXPECT_EQ(ud.header().size, udSectionData.size());
    EXPECT_EQ(ud.header().version, 0x01);
    EXPECT_EQ(ud.header().subType, 0x02);
    EXPECT_EQ(ud.header().componentID, 0x0304);

    const auto& data = ud.data();

    // The data itself starts after the header
    EXPECT_EQ(data.size(), udSectionData.size() - 8);

    for (size_t i = 0; i < data.size(); i++)
    {
        EXPECT_EQ(data[i], udSectionData[i + 8]);
    }

    // Now flatten
    std::vector<uint8_t> newData;
    Stream newStream(newData);
    ud.flatten(newStream);

    EXPECT_EQ(udSectionData, newData);
}

TEST(UserDataTest, BadDataTest)
{
    auto data = udSectionData;
    data.resize(4);

    Stream stream(data);
    UserData ud(stream);
    EXPECT_FALSE(ud.valid());
}

TEST(UserDataTest, BadSizeFieldTest)
{
    auto data = udSectionData;

    {
        data[3] = 0xFF; // Set the size field too large
        Stream stream(data);
        UserData ud(stream);
        EXPECT_FALSE(ud.valid());
    }
    {
        data[3] = 0x7; // Set the size field too small
        Stream stream(data);
        UserData ud(stream);
        EXPECT_FALSE(ud.valid());
    }
}

TEST(UserDataTest, ConstructorTest)
{
    std::vector<uint8_t> data{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

    UserData ud(0x1112, 0x42, 0x01, data);
    EXPECT_TRUE(ud.valid());

    EXPECT_EQ(ud.header().id, 0x5544);
    EXPECT_EQ(ud.header().size, 16);
    EXPECT_EQ(ud.header().version, 0x01);
    EXPECT_EQ(ud.header().subType, 0x42);
    EXPECT_EQ(ud.header().componentID, 0x1112);
    EXPECT_EQ(ud.flattenedSize(), 16);

    const auto& d = ud.data();

    EXPECT_EQ(d, data);
}

TEST(UserDataTest, ShrinkTest)
{
    std::vector<uint8_t> data(100, 0xFF);

    UserData ud(0x1112, 0x42, 0x01, data);
    EXPECT_TRUE(ud.valid());

    // 4B aligned
    EXPECT_TRUE(ud.shrink(88));
    EXPECT_EQ(ud.flattenedSize(), 88);
    EXPECT_EQ(ud.header().size, 88);

    // rounded off
    EXPECT_TRUE(ud.shrink(87));
    EXPECT_EQ(ud.flattenedSize(), 84);
    EXPECT_EQ(ud.header().size, 84);

    // too big
    EXPECT_FALSE(ud.shrink(200));
    EXPECT_EQ(ud.flattenedSize(), 84);
    EXPECT_EQ(ud.header().size, 84);

    // way too small
    EXPECT_FALSE(ud.shrink(3));
    EXPECT_EQ(ud.flattenedSize(), 84);
    EXPECT_EQ(ud.header().size, 84);

    // the smallest it can go
    EXPECT_TRUE(ud.shrink(12));
    EXPECT_EQ(ud.flattenedSize(), 12);
    EXPECT_EQ(ud.header().size, 12);

    // one too small
    EXPECT_FALSE(ud.shrink(11));
    EXPECT_EQ(ud.flattenedSize(), 12);
    EXPECT_EQ(ud.header().size, 12);
}
