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
#include "extensions/openpower-pels/generic.hpp"
#include "pel_utils.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;

TEST(GenericSectionTest, UnflattenFlattenTest)
{
    // Use the private header data
    auto data = pelDataFactory(TestPELType::privateHeaderSection);

    Stream stream(data);
    Generic section(stream);

    EXPECT_EQ(section.header().id, 0x5048);
    EXPECT_EQ(section.header().size, data.size());
    EXPECT_EQ(section.header().version, 0x01);
    EXPECT_EQ(section.header().subType, 0x02);
    EXPECT_EQ(section.header().componentID, 0x0304);

    const auto& sectionData = section.data();

    // The data itself starts after the header
    EXPECT_EQ(sectionData.size(), data.size() - 8);

    for (size_t i = 0; i < sectionData.size(); i++)
    {
        EXPECT_EQ(sectionData[i], (data)[i + 8]);
    }

    // Now flatten
    std::vector<uint8_t> newData;
    Stream newStream(newData);
    section.flatten(newStream);

    EXPECT_EQ(data, newData);
}

TEST(GenericSectionTest, BadDataTest)
{
    // Use the private header data to start with
    auto data = pelDataFactory(TestPELType::privateHeaderSection);
    data.resize(4);

    Stream stream(data);
    Generic section(stream);
    ASSERT_FALSE(section.valid());
}
