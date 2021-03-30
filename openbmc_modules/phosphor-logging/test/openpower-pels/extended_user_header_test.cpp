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
#include "extensions/openpower-pels/extended_user_header.hpp"
#include "mocks.hpp"
#include "pel_utils.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;
using ::testing::Return;

const std::vector<uint8_t> sectionData{
    // section header
    'E', 'H', 0x00, 0x60, // ID and Size
    0x01, 0x00,           // version, subtype
    0x03, 0x04,           // comp ID

    // MTMS
    'T', 'T', 'T', 'T', '-', 'M', 'M', 'M', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C',

    // Server FW version
    'S', 'E', 'R', 'V', 'E', 'R', '_', 'V', 'E', 'R', 'S', 'I', 'O', 'N', '\0',
    '\0',

    // Subsystem FW Version
    'B', 'M', 'C', '_', 'V', 'E', 'R', 'S', 'I', 'O', 'N', '\0', '\0', '\0',
    '\0', '\0',

    // Reserved
    0x00, 0x00, 0x00, 0x00,

    // Reference time
    0x20, 0x25, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60,

    // Reserved
    0x00, 0x00, 0x00,

    // SymptomID length
    20,

    // SymptomID
    'B', 'D', '8', 'D', '4', '2', '0', '0', '_', '1', '2', '3', '4', '5', '6',
    '7', '8', '\0', '\0', '\0'};

// The section size without the symptom ID
const size_t baseSectionSize = 76;

TEST(ExtUserHeaderTest, StreamConstructorTest)
{
    auto data = sectionData;
    Stream stream{data};
    ExtendedUserHeader euh{stream};

    EXPECT_EQ(euh.valid(), true);
    EXPECT_EQ(euh.header().id, 0x4548); // EH
    EXPECT_EQ(euh.header().size, sectionData.size());
    EXPECT_EQ(euh.header().version, 0x01);
    EXPECT_EQ(euh.header().subType, 0x00);
    EXPECT_EQ(euh.header().componentID, 0x0304);

    EXPECT_EQ(euh.flattenedSize(), sectionData.size());
    EXPECT_EQ(euh.machineTypeModel(), "TTTT-MMM");
    EXPECT_EQ(euh.machineSerialNumber(), "123456789ABC");
    EXPECT_EQ(euh.serverFWVersion(), "SERVER_VERSION");
    EXPECT_EQ(euh.subsystemFWVersion(), "BMC_VERSION");
    EXPECT_EQ(euh.symptomID(), "BD8D4200_12345678");

    BCDTime time{0x20, 0x25, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
    EXPECT_EQ(time, euh.refTime());

    // Flatten it and make sure nothing changes
    std::vector<uint8_t> newData;
    Stream newStream{newData};

    euh.flatten(newStream);
    EXPECT_EQ(sectionData, newData);
}

// Same as above, with with symptom ID empty
TEST(ExtUserHeaderTest, StreamConstructorNoIDTest)
{
    auto data = sectionData;
    data.resize(baseSectionSize);
    data[3] = baseSectionSize; // The size in the header
    data.back() = 0;           // Symptom ID length

    Stream stream{data};
    ExtendedUserHeader euh{stream};

    EXPECT_EQ(euh.valid(), true);
    EXPECT_EQ(euh.header().id, 0x4548); // EH
    EXPECT_EQ(euh.header().size, baseSectionSize);
    EXPECT_EQ(euh.header().version, 0x01);
    EXPECT_EQ(euh.header().subType, 0x00);
    EXPECT_EQ(euh.header().componentID, 0x0304);

    EXPECT_EQ(euh.flattenedSize(), baseSectionSize);
    EXPECT_EQ(euh.machineTypeModel(), "TTTT-MMM");
    EXPECT_EQ(euh.machineSerialNumber(), "123456789ABC");
    EXPECT_EQ(euh.serverFWVersion(), "SERVER_VERSION");
    EXPECT_EQ(euh.subsystemFWVersion(), "BMC_VERSION");
    EXPECT_EQ(euh.symptomID(), "");

    BCDTime time{0x20, 0x25, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
    EXPECT_EQ(time, euh.refTime());

    // Flatten it and make sure nothing changes
    std::vector<uint8_t> newData;
    Stream newStream{newData};

    euh.flatten(newStream);
    EXPECT_EQ(data, newData);
}

TEST(ExtUserHeaderTest, ConstructorTest)
{
    auto srcData = pelDataFactory(TestPELType::primarySRCSection);
    Stream srcStream{srcData};
    SRC src{srcStream};

    message::Entry entry; // Empty Symptom ID vector

    {
        MockDataInterface dataIface;

        EXPECT_CALL(dataIface, getMachineTypeModel())
            .WillOnce(Return("AAAA-BBB"));

        EXPECT_CALL(dataIface, getMachineSerialNumber())
            .WillOnce(Return("123456789ABC"));

        EXPECT_CALL(dataIface, getServerFWVersion())
            .WillOnce(Return("SERVER_VERSION"));

        EXPECT_CALL(dataIface, getBMCFWVersion())
            .WillOnce(Return("BMC_VERSION"));

        ExtendedUserHeader euh{dataIface, entry, src};

        EXPECT_EQ(euh.valid(), true);
        EXPECT_EQ(euh.header().id, 0x4548); // EH

        // The symptom ID accounts for the extra 20 bytes
        EXPECT_EQ(euh.header().size, baseSectionSize + 20);
        EXPECT_EQ(euh.header().version, 0x01);
        EXPECT_EQ(euh.header().subType, 0x00);
        EXPECT_EQ(euh.header().componentID, 0x2000);

        EXPECT_EQ(euh.flattenedSize(), baseSectionSize + 20);
        EXPECT_EQ(euh.machineTypeModel(), "AAAA-BBB");
        EXPECT_EQ(euh.machineSerialNumber(), "123456789ABC");
        EXPECT_EQ(euh.serverFWVersion(), "SERVER_VERSION");
        EXPECT_EQ(euh.subsystemFWVersion(), "BMC_VERSION");

        // The default symptom ID is the ascii string + word 3
        EXPECT_EQ(euh.symptomID(), "BD8D5678_03030310");

        BCDTime time;
        EXPECT_EQ(time, euh.refTime());
    }

    {
        MockDataInterface dataIface;

        // These 4 items are too long and will get truncated
        // in the section.
        EXPECT_CALL(dataIface, getMachineTypeModel())
            .WillOnce(Return("AAAA-BBBBBBBBBBB"));

        EXPECT_CALL(dataIface, getMachineSerialNumber())
            .WillOnce(Return("123456789ABC123456789"));

        EXPECT_CALL(dataIface, getServerFWVersion())
            .WillOnce(Return("SERVER_VERSION_WAY_TOO_LONG"));

        EXPECT_CALL(dataIface, getBMCFWVersion())
            .WillOnce(Return("BMC_VERSION_WAY_TOO_LONG"));

        // Use SRC words 3 through 9
        entry.src.symptomID = {3, 4, 5, 6, 7, 8, 9};
        ExtendedUserHeader euh{dataIface, entry, src};

        EXPECT_EQ(euh.valid(), true);
        EXPECT_EQ(euh.header().id, 0x4548); // EH
        EXPECT_EQ(euh.header().size, baseSectionSize + 72);
        EXPECT_EQ(euh.header().version, 0x01);
        EXPECT_EQ(euh.header().subType, 0x00);
        EXPECT_EQ(euh.header().componentID, 0x2000);

        EXPECT_EQ(euh.flattenedSize(), baseSectionSize + 72);
        EXPECT_EQ(euh.machineTypeModel(), "AAAA-BBB");
        EXPECT_EQ(euh.machineSerialNumber(), "123456789ABC");
        EXPECT_EQ(euh.serverFWVersion(), "SERVER_VERSION_");
        EXPECT_EQ(euh.subsystemFWVersion(), "BMC_VERSION_WAY");

        EXPECT_EQ(euh.symptomID(), "BD8D5678_03030310_04040404_05050505_"
                                   "06060606_07070707_08080808_09090909");
        BCDTime time;
        EXPECT_EQ(time, euh.refTime());
    }

    {
        MockDataInterface dataIface;

        // Empty fields
        EXPECT_CALL(dataIface, getMachineTypeModel()).WillOnce(Return(""));

        EXPECT_CALL(dataIface, getMachineSerialNumber()).WillOnce(Return(""));

        EXPECT_CALL(dataIface, getServerFWVersion()).WillOnce(Return(""));

        EXPECT_CALL(dataIface, getBMCFWVersion()).WillOnce(Return(""));

        entry.src.symptomID = {8, 9};
        ExtendedUserHeader euh{dataIface, entry, src};

        EXPECT_EQ(euh.valid(), true);
        EXPECT_EQ(euh.header().id, 0x4548); // EH
        EXPECT_EQ(euh.header().size, baseSectionSize + 28);
        EXPECT_EQ(euh.header().version, 0x01);
        EXPECT_EQ(euh.header().subType, 0x00);
        EXPECT_EQ(euh.header().componentID, 0x2000);

        EXPECT_EQ(euh.flattenedSize(), baseSectionSize + 28);
        EXPECT_EQ(euh.machineTypeModel(), "");
        EXPECT_EQ(euh.machineSerialNumber(), "");
        EXPECT_EQ(euh.serverFWVersion(), "");
        EXPECT_EQ(euh.subsystemFWVersion(), "");

        EXPECT_EQ(euh.symptomID(), "BD8D5678_08080808_09090909");

        BCDTime time;
        EXPECT_EQ(time, euh.refTime());
    }

    {
        MockDataInterface dataIface;

        EXPECT_CALL(dataIface, getMachineTypeModel())
            .WillOnce(Return("AAAA-BBB"));

        EXPECT_CALL(dataIface, getMachineSerialNumber())
            .WillOnce(Return("123456789ABC"));

        EXPECT_CALL(dataIface, getServerFWVersion())
            .WillOnce(Return("SERVER_VERSION"));

        EXPECT_CALL(dataIface, getBMCFWVersion())
            .WillOnce(Return("BMC_VERSION"));

        // Way too long, will be truncated
        entry.src.symptomID = {9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

        ExtendedUserHeader euh{dataIface, entry, src};

        EXPECT_EQ(euh.valid(), true);
        EXPECT_EQ(euh.header().id, 0x4548); // EH
        EXPECT_EQ(euh.header().size, baseSectionSize + 80);
        EXPECT_EQ(euh.header().version, 0x01);
        EXPECT_EQ(euh.header().subType, 0x00);
        EXPECT_EQ(euh.header().componentID, 0x2000);

        EXPECT_EQ(euh.flattenedSize(), baseSectionSize + 80);
        EXPECT_EQ(euh.machineTypeModel(), "AAAA-BBB");
        EXPECT_EQ(euh.machineSerialNumber(), "123456789ABC");
        EXPECT_EQ(euh.serverFWVersion(), "SERVER_VERSION");
        EXPECT_EQ(euh.subsystemFWVersion(), "BMC_VERSION");

        EXPECT_EQ(euh.symptomID(),
                  "BD8D5678_09090909_09090909_09090909_09090909_09090909_"
                  "09090909_09090909_0909090");

        BCDTime time;
        EXPECT_EQ(time, euh.refTime());
    }
}

TEST(ExtUserHeaderTest, BadDataTest)
{
    auto data = sectionData;
    data.resize(20);

    Stream stream{data};
    ExtendedUserHeader euh{stream};

    EXPECT_EQ(euh.valid(), false);
}
