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
#include "extensions/openpower-pels/fru_identity.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;
using namespace openpower::pels::src;

// Unflatten a FRUIdentity that is a HW FRU callout
TEST(FRUIdentityTest, TestHardwareFRU)
{
    // Has PN, SN, CCIN
    std::vector<uint8_t> data{'I', 'D', 0x1C, 0x1D, // type, size, flags
                              '1', '2', '3',  '4',  // PN
                              '5', '6', '7',  0x00, 'A', 'A', 'A', 'A', // CCIN
                              '1', '2', '3',  '4',  '5', '6', '7', '8', // SN
                              '9', 'A', 'B',  'C'};

    Stream stream{data};

    FRUIdentity fru{stream};

    EXPECT_EQ(fru.failingComponentType(), FRUIdentity::hardwareFRU);
    EXPECT_EQ(fru.flattenedSize(), data.size());
    EXPECT_EQ(fru.type(), 0x4944);

    EXPECT_EQ(fru.getPN().value(), "1234567");
    EXPECT_EQ(fru.getCCIN().value(), "AAAA");
    EXPECT_EQ(fru.getSN().value(), "123456789ABC");
    EXPECT_FALSE(fru.getMaintProc());

    // Flatten
    std::vector<uint8_t> newData;
    Stream newStream{newData};
    fru.flatten(newStream);
    EXPECT_EQ(data, newData);
}

// Unflatten a FRUIdentity that is a Maintenance Procedure callout
TEST(FRUIdentityTest, TestMaintProcedure)
{
    // Only contains the maintenance procedure
    std::vector<uint8_t> data{
        0x49, 0x44, 0x0C, 0x42,                     // type, size, flags
        '1',  '2',  '3',  '4',  '5', '6', '7', 0x00 // Procedure
    };

    Stream stream{data};

    FRUIdentity fru{stream};

    EXPECT_EQ(fru.failingComponentType(), FRUIdentity::maintenanceProc);
    EXPECT_EQ(fru.flattenedSize(), data.size());

    EXPECT_EQ(fru.getMaintProc().value(), "1234567");
    EXPECT_FALSE(fru.getPN());
    EXPECT_FALSE(fru.getCCIN());
    EXPECT_FALSE(fru.getSN());

    // Flatten
    std::vector<uint8_t> newData;
    Stream newStream{newData};
    fru.flatten(newStream);
    EXPECT_EQ(data, newData);
}

// Try to unflatten garbage data
TEST(FRUIdentityTest, BadDataTest)
{
    std::vector<uint8_t> data{0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                              0xFF, 0xFF, 0xFF, 0xFF};

    Stream stream{data};

    EXPECT_THROW(FRUIdentity fru{stream}, std::out_of_range);
}

void testHWCallout(const std::string& pn, const std::string ccin,
                   const std::string& sn, const std::string& expectedPN,
                   const std::string& expectedCCIN,
                   const std::string& expectedSN)
{
    FRUIdentity fru{pn, ccin, sn};

    EXPECT_EQ(fru.flattenedSize(), 28);
    EXPECT_EQ(fru.type(), 0x4944);
    EXPECT_EQ(fru.failingComponentType(), FRUIdentity::hardwareFRU);
    EXPECT_EQ(fru.getPN().value(), expectedPN);
    EXPECT_EQ(fru.getCCIN().value(), expectedCCIN);
    EXPECT_EQ(fru.getSN().value(), expectedSN);
    EXPECT_FALSE(fru.getMaintProc());

    // Flatten and unflatten, then compare again
    std::vector<uint8_t> data;
    Stream stream{data};
    fru.flatten(stream);

    EXPECT_EQ(data.size(), fru.flattenedSize());

    stream.offset(0);
    FRUIdentity newFRU{stream};

    EXPECT_EQ(newFRU.flattenedSize(), fru.flattenedSize());
    EXPECT_EQ(newFRU.type(), fru.type());
    EXPECT_EQ(newFRU.failingComponentType(), fru.failingComponentType());
    EXPECT_EQ(newFRU.getPN().value(), fru.getPN().value());
    EXPECT_EQ(newFRU.getCCIN().value(), fru.getCCIN().value());
    EXPECT_EQ(newFRU.getSN().value(), fru.getSN().value());
    EXPECT_FALSE(newFRU.getMaintProc());
}

// Test the constructor that takes in a PN/SN/CCIN
TEST(FRUIdentityTest, CreateHardwareCalloutTest)
{
    // The right sizes
    testHWCallout("1234567", "1234", "123456789ABC",
                  // expected
                  "1234567", "1234", "123456789ABC");

    // Too long
    testHWCallout("1234567long", "1234long", "123456789ABClong",
                  // expected
                  "1234567", "1234", "123456789ABC");
    // Too short
    testHWCallout("11", "22", "333",
                  // expected
                  "11", "22", "333");

    // empty
    testHWCallout("", "", "",
                  // expected
                  "", "", "");

    // Leading spaces in the part number will be stripped
    testHWCallout("    567", "1234", "123456789ABC",
                  // expected
                  "567", "1234", "123456789ABC");

    // All spaces in the part number
    testHWCallout("       ", "1234", "123456789ABC",
                  // expected
                  "", "1234", "123456789ABC");
}

// Test the constructor that takes in a maint procedure
TEST(FRUIdentityTest, CreateProcedureCalloutTest)
{
    {
        FRUIdentity fru{"no_vpd_for_fru"};

        EXPECT_EQ(fru.flattenedSize(), 12);
        EXPECT_EQ(fru.type(), 0x4944);
        EXPECT_EQ(fru.failingComponentType(), FRUIdentity::maintenanceProc);
        EXPECT_EQ(fru.getMaintProc().value(), "BMCSP01");
        EXPECT_FALSE(fru.getPN());
        EXPECT_FALSE(fru.getCCIN());
        EXPECT_FALSE(fru.getSN());

        // Flatten and unflatten, then compare again
        std::vector<uint8_t> data;
        Stream stream{data};
        fru.flatten(stream);

        EXPECT_EQ(data.size(), fru.flattenedSize());

        stream.offset(0);
        FRUIdentity newFRU{stream};

        EXPECT_EQ(newFRU.flattenedSize(), 12);
        EXPECT_EQ(newFRU.type(), 0x4944);
        EXPECT_EQ(newFRU.failingComponentType(), FRUIdentity::maintenanceProc);
        EXPECT_EQ(newFRU.getMaintProc().value(), "BMCSP01");
        EXPECT_FALSE(newFRU.getPN());
        EXPECT_FALSE(newFRU.getCCIN());
        EXPECT_FALSE(newFRU.getSN());
    }

    {
        // Invalid maintenance procedure
        FRUIdentity fru{"invalid"};

        EXPECT_EQ(fru.flattenedSize(), 12);
        EXPECT_EQ(fru.type(), 0x4944);
        EXPECT_EQ(fru.failingComponentType(), FRUIdentity::maintenanceProc);
        EXPECT_EQ(fru.getMaintProc().value(), "INVALID");
        EXPECT_FALSE(fru.getPN());
        EXPECT_FALSE(fru.getCCIN());
        EXPECT_FALSE(fru.getSN());
    }
}

// Test the constructor that takes in a symbolic FRU.
TEST(FRUIdentityTest, CreateSymbolicFRUCalloutTest)
{
    // Symbolic FRU (not trusted)
    {
        FRUIdentity fru{"service_docs", false};

        EXPECT_EQ(fru.flattenedSize(), 12);
        EXPECT_EQ(fru.type(), 0x4944);
        EXPECT_EQ(fru.failingComponentType(), FRUIdentity::symbolicFRU);
        EXPECT_EQ(fru.getPN().value(), "SVCDOCS");
        EXPECT_FALSE(fru.getMaintProc());
        EXPECT_FALSE(fru.getCCIN());
        EXPECT_FALSE(fru.getSN());

        // Flatten and unflatten, then compare again
        std::vector<uint8_t> data;
        Stream stream{data};
        fru.flatten(stream);

        EXPECT_EQ(data.size(), fru.flattenedSize());

        stream.offset(0);
        FRUIdentity newFRU{stream};

        EXPECT_EQ(newFRU.flattenedSize(), 12);
        EXPECT_EQ(newFRU.type(), 0x4944);
        EXPECT_EQ(newFRU.failingComponentType(), FRUIdentity::symbolicFRU);
        EXPECT_EQ(newFRU.getPN().value(), "SVCDOCS");
        EXPECT_FALSE(newFRU.getMaintProc());
        EXPECT_FALSE(newFRU.getCCIN());
        EXPECT_FALSE(newFRU.getSN());
    }

    // Trusted symbolic FRU
    {
        FRUIdentity fru{"service_docs", true};

        EXPECT_EQ(fru.flattenedSize(), 12);
        EXPECT_EQ(fru.type(), 0x4944);
        EXPECT_EQ(fru.failingComponentType(),
                  FRUIdentity::symbolicFRUTrustedLocCode);
        EXPECT_EQ(fru.getPN().value(), "SVCDOCS");
        EXPECT_FALSE(fru.getMaintProc());
        EXPECT_FALSE(fru.getCCIN());
        EXPECT_FALSE(fru.getSN());
    }

    // Invalid symbolic FRU
    {
        FRUIdentity fru{"garbage", false};

        EXPECT_EQ(fru.flattenedSize(), 12);
        EXPECT_EQ(fru.type(), 0x4944);
        EXPECT_EQ(fru.failingComponentType(), FRUIdentity::symbolicFRU);
        EXPECT_EQ(fru.getPN().value(), "INVALID");
        EXPECT_FALSE(fru.getMaintProc());
        EXPECT_FALSE(fru.getCCIN());
        EXPECT_FALSE(fru.getSN());
    }
}
