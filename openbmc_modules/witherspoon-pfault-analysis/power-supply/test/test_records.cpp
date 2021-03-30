/**
 * Copyright © 2017 IBM Corporation
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
#include "../record_manager.hpp"
#include "names_values.hpp"

#include <iostream>

#include <gtest/gtest.h>

using namespace witherspoon::power::history;

/**
 * Test the linearToInteger function with different
 * combinations of positive and negative mantissas and
 * exponents.
 *
 * Value = mantissa * 2**exponent
 */
TEST(LinearFormatTest, TestConversions)
{
    // Mantissa > 0, exponent = 0
    EXPECT_EQ(0, RecordManager::linearToInteger(0));
    EXPECT_EQ(1, RecordManager::linearToInteger(1));
    EXPECT_EQ(38, RecordManager::linearToInteger(0x26));
    EXPECT_EQ(1023, RecordManager::linearToInteger(0x3FF));

    // Mantissa < 0, exponent = 0
    EXPECT_EQ(-1, RecordManager::linearToInteger(0x7FF));
    EXPECT_EQ(-20, RecordManager::linearToInteger(0x7EC));
    EXPECT_EQ(-769, RecordManager::linearToInteger(0x4FF));
    EXPECT_EQ(-989, RecordManager::linearToInteger(0x423));
    EXPECT_EQ(-1024, RecordManager::linearToInteger(0x400));

    // Mantissa >= 0, exponent > 0
    // M = 1, E = 2
    EXPECT_EQ(4, RecordManager::linearToInteger(0x1001));

    // M = 1000, E = 10
    EXPECT_EQ(1024000, RecordManager::linearToInteger(0x53E8));

    // M = 10, E = 15
    EXPECT_EQ(327680, RecordManager::linearToInteger(0x780A));

    // Mantissa >= 0, exponent < 0
    // M = 0, E = -1
    EXPECT_EQ(0, RecordManager::linearToInteger(0xF800));

    // M = 100, E = -2
    EXPECT_EQ(25, RecordManager::linearToInteger(0xF064));

    // Mantissa < 0, exponent < 0
    // M = -100, E = -1
    EXPECT_EQ(-50, RecordManager::linearToInteger(0xFF9C));

    // M = -1024, E = -7
    EXPECT_EQ(-8, RecordManager::linearToInteger(0xCC00));
}

/**
 * @brief Helper function to create a record buffer
 *
 * @param sequenceID - the ID to use
 * @param avgPower - the average power, in linear format
 * @param maxPower - the max power, in linear format
 *
 * @return vector<uint8_t> the record buffer
 */
std::vector<uint8_t> makeRawRecord(uint8_t sequenceID, uint16_t avgPower,
                                   uint16_t maxPower)
{
    std::vector<uint8_t> record;

    record.push_back(sequenceID);
    record.push_back(avgPower & 0xFF);
    record.push_back(avgPower >> 8);
    record.push_back(maxPower & 0xFF);
    record.push_back(maxPower >> 8);

    return record;
}

/**
 * Test record queue management
 */
TEST(ManagerTest, TestRecordAdds)
{
    // Hold 5 max records.  IDs roll over at 8.
    RecordManager mgr{5, 8};

    EXPECT_EQ(0, mgr.getNumRecords());

    mgr.add(makeRawRecord(0, 0, 0));
    EXPECT_EQ(1, mgr.getNumRecords());

    mgr.add(makeRawRecord(1, 0, 0));
    EXPECT_EQ(2, mgr.getNumRecords());

    mgr.add(makeRawRecord(2, 0, 0));
    EXPECT_EQ(3, mgr.getNumRecords());

    mgr.add(makeRawRecord(3, 0, 0));
    EXPECT_EQ(4, mgr.getNumRecords());

    mgr.add(makeRawRecord(4, 0, 0));
    EXPECT_EQ(5, mgr.getNumRecords());

    // start pruning
    mgr.add(makeRawRecord(5, 0, 0));
    EXPECT_EQ(5, mgr.getNumRecords());

    mgr.add(makeRawRecord(6, 0, 0));
    EXPECT_EQ(5, mgr.getNumRecords());

    mgr.add(makeRawRecord(7, 0, 0));
    EXPECT_EQ(5, mgr.getNumRecords());

    mgr.add(makeRawRecord(8, 0, 0));
    EXPECT_EQ(5, mgr.getNumRecords());

    // rollover
    mgr.add(makeRawRecord(0, 0, 0));
    EXPECT_EQ(5, mgr.getNumRecords());

    mgr.add(makeRawRecord(1, 0, 0));
    EXPECT_EQ(5, mgr.getNumRecords());

    // nonsequential ID, clear previous
    mgr.add(makeRawRecord(4, 0, 10));
    EXPECT_EQ(1, mgr.getNumRecords());

    // back to normal
    mgr.add(makeRawRecord(5, 1, 11));
    EXPECT_EQ(2, mgr.getNumRecords());

    // One more good record
    mgr.add(makeRawRecord(6, 2, 12));
    EXPECT_EQ(3, mgr.getNumRecords());

    // Add a garbage length record. No size change
    mgr.add(std::vector<uint8_t>(6, 0));
    EXPECT_EQ(3, mgr.getNumRecords());

    // Test the DBus Records
    auto avgRecords = mgr.getAverageRecords();
    EXPECT_EQ(3, avgRecords.size());

    auto maxRecords = mgr.getMaximumRecords();
    EXPECT_EQ(3, maxRecords.size());

    auto avg = 2;
    for (const auto& r : avgRecords)
    {
        EXPECT_EQ(avg, std::get<1>(r));
        avg--;
    }

    auto max = 12;
    for (const auto& r : maxRecords)
    {
        EXPECT_EQ(max, std::get<1>(r));
        max--;
    }

    // Add a zero length record. Records cleared.
    mgr.add(std::vector<uint8_t>{});
    EXPECT_EQ(0, mgr.getNumRecords());
}
