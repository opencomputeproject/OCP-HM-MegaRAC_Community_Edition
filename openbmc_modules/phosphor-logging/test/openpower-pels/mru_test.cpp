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
#include "extensions/openpower-pels/mru.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;
using namespace openpower::pels::src;

TEST(MRUTest, TestConstructor)
{
    std::vector<uint8_t> data{
        'M',  'R',  0x28, 0x04, // ID, size, flags
        0x00, 0x00, 0x00, 0x00, // Reserved
        0x00, 0x00, 0x00, 'H',  // priority for MRU ID 0
        0x01, 0x01, 0x01, 0x01, // MRU ID 0
        0x00, 0x00, 0x00, 'M',  // priority for MRU ID 1
        0x02, 0x02, 0x02, 0x02, // MRU ID 1
        0x00, 0x00, 0x00, 'L',  // priority for MRU ID 2
        0x03, 0x03, 0x03, 0x03, // MRU ID 2
        0x00, 0x00, 0x00, 'H',  // priority for MRU ID 3
        0x04, 0x04, 0x04, 0x04, // MRU ID 3
    };

    Stream stream{data};

    MRU mru{stream};

    EXPECT_EQ(mru.flattenedSize(), data.size());
    EXPECT_EQ(mru.mrus().size(), 4);

    EXPECT_EQ(mru.mrus().at(0).priority, 'H');
    EXPECT_EQ(mru.mrus().at(0).id, 0x01010101);
    EXPECT_EQ(mru.mrus().at(1).priority, 'M');
    EXPECT_EQ(mru.mrus().at(1).id, 0x02020202);
    EXPECT_EQ(mru.mrus().at(2).priority, 'L');
    EXPECT_EQ(mru.mrus().at(2).id, 0x03030303);
    EXPECT_EQ(mru.mrus().at(3).priority, 'H');
    EXPECT_EQ(mru.mrus().at(3).id, 0x04040404);

    // Now flatten
    std::vector<uint8_t> newData;
    Stream newStream{newData};

    mru.flatten(newStream);
    EXPECT_EQ(data, newData);
}

TEST(MRUTest, TestBadData)
{
    // 4 MRUs expected, but only 1
    std::vector<uint8_t> data{
        'M',  'R',  0x28, 0x04, // ID, size, flags
        0x00, 0x00, 0x00, 0x00, // Reserved
        0x00, 0x00, 0x00, 'H',  // priority 0
        0x01, 0x01, 0x01, 0x01, // MRU ID 0
    };

    Stream stream{data};
    EXPECT_THROW(MRU mru{stream}, std::out_of_range);
}
