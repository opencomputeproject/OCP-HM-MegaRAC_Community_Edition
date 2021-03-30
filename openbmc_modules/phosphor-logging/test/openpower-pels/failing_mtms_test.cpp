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
#include "extensions/openpower-pels/failing_mtms.hpp"
#include "mocks.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;
using ::testing::Return;

TEST(FailingMTMSTest, SizeTest)
{
    EXPECT_EQ(FailingMTMS::flattenedSize(), 28);
}

TEST(FailingMTMSTest, ConstructorTest)
{
    // Note: the TypeModel field is 8B, and the SN field is 12B
    {
        MockDataInterface dataIface;

        EXPECT_CALL(dataIface, getMachineTypeModel())
            .WillOnce(Return("AAAA-BBB"));

        EXPECT_CALL(dataIface, getMachineSerialNumber())
            .WillOnce(Return("123456789ABC"));

        FailingMTMS fm{dataIface};

        // Check the section header
        EXPECT_EQ(fm.header().id, 0x4D54);
        EXPECT_EQ(fm.header().size, FailingMTMS::flattenedSize());
        EXPECT_EQ(fm.header().version, 0x01);
        EXPECT_EQ(fm.header().subType, 0x00);
        EXPECT_EQ(fm.header().componentID, 0x2000);

        EXPECT_EQ(fm.getMachineTypeModel(), "AAAA-BBB");
        EXPECT_EQ(fm.getMachineSerialNumber(), "123456789ABC");
    }

    // longer than the max - will truncate
    {
        MockDataInterface dataIface;

        EXPECT_CALL(dataIface, getMachineTypeModel())
            .WillOnce(Return("AAAA-BBBTOOLONG"));

        EXPECT_CALL(dataIface, getMachineSerialNumber())
            .WillOnce(Return("123456789ABCTOOLONG"));

        FailingMTMS fm{dataIface};

        EXPECT_EQ(fm.getMachineTypeModel(), "AAAA-BBB");
        EXPECT_EQ(fm.getMachineSerialNumber(), "123456789ABC");
    }

    // shorter than the max
    {
        MockDataInterface dataIface;

        EXPECT_CALL(dataIface, getMachineTypeModel()).WillOnce(Return("A"));

        EXPECT_CALL(dataIface, getMachineSerialNumber()).WillOnce(Return("1"));

        FailingMTMS fm{dataIface};

        EXPECT_EQ(fm.getMachineTypeModel(), "A");
        EXPECT_EQ(fm.getMachineSerialNumber(), "1");
    }
}

TEST(FailingMTMSTest, StreamConstructorTest)
{
    std::vector<uint8_t> data{0x4D, 0x54, 0x00, 0x1C, 0x01, 0x00, 0x20,
                              0x00, 'T',  'T',  'T',  'T',  '-',  'M',
                              'M',  'M',  '1',  '2',  '3',  '4',  '5',
                              '6',  '7',  '8',  '9',  'A',  'B',  'C'};
    Stream stream{data};
    FailingMTMS fm{stream};

    EXPECT_EQ(fm.valid(), true);

    EXPECT_EQ(fm.header().id, 0x4D54);
    EXPECT_EQ(fm.header().size, FailingMTMS::flattenedSize());
    EXPECT_EQ(fm.header().version, 0x01);
    EXPECT_EQ(fm.header().subType, 0x00);
    EXPECT_EQ(fm.header().componentID, 0x2000);

    EXPECT_EQ(fm.getMachineTypeModel(), "TTTT-MMM");
    EXPECT_EQ(fm.getMachineSerialNumber(), "123456789ABC");
}

TEST(FailingMTMSTest, BadStreamConstructorTest)
{
    // too short
    std::vector<uint8_t> data{
        0x4D, 0x54, 0x00, 0x1C, 0x01, 0x00, 0x20, 0x00, 'T', 'T',
    };
    Stream stream{data};
    FailingMTMS fm{stream};

    EXPECT_EQ(fm.valid(), false);
}

TEST(FailingMTMSTest, FlattenTest)
{
    std::vector<uint8_t> data{0x4D, 0x54, 0x00, 0x1C, 0x01, 0x00, 0x20,
                              0x00, 'T',  'T',  'T',  'T',  '-',  'M',
                              'M',  'M',  '1',  '2',  '3',  '4',  '5',
                              '6',  '7',  '8',  '9',  'A',  'B',  'C'};
    Stream stream{data};
    FailingMTMS fm{stream};

    // flatten and check results
    std::vector<uint8_t> newData;
    Stream newStream{newData};

    fm.flatten(newStream);
    EXPECT_EQ(data, newData);
}
