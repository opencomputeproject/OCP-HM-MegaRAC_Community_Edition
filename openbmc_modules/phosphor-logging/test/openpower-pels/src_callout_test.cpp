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
#include "extensions/openpower-pels/callout.hpp"
#include "pel_utils.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;
using namespace openpower::pels::src;

// Unflatten the callout section with all three substructures
TEST(CalloutTest, TestUnflattenAllSubstructures)
{
    // The base data.
    std::vector<uint8_t> data{
        0xFF, 0x2F, 'H', 8, // size, flags, priority, LC length
        'U',  '1',  '2', '-', 'P', '1', 0x00, 0x00 // LC
    };

    auto fruIdentity = srcDataFactory(TestSRCType::fruIdentityStructure);
    auto pceIdentity = srcDataFactory(TestSRCType::pceIdentityStructure);
    auto mrus = srcDataFactory(TestSRCType::mruStructure);

    // Add all 3 substructures
    data.insert(data.end(), fruIdentity.begin(), fruIdentity.end());
    data.insert(data.end(), pceIdentity.begin(), pceIdentity.end());
    data.insert(data.end(), mrus.begin(), mrus.end());

    // The final size
    data[0] = data.size();

    Stream stream{data};
    Callout callout{stream};

    EXPECT_EQ(callout.flattenedSize(), data.size());
    EXPECT_EQ(callout.priority(), 'H');
    EXPECT_EQ(callout.locationCode(), "U12-P1");

    // Spot check the 3 substructures
    EXPECT_TRUE(callout.fruIdentity());
    EXPECT_EQ(callout.fruIdentity()->getSN(), "123456789ABC");

    EXPECT_TRUE(callout.pceIdentity());
    EXPECT_EQ(callout.pceIdentity()->enclosureName(), "PCENAME12");

    EXPECT_TRUE(callout.mru());
    EXPECT_EQ(callout.mru()->mrus().size(), 4);
    EXPECT_EQ(callout.mru()->mrus().at(3).id, 0x04040404);

    // Now flatten
    std::vector<uint8_t> newData;
    Stream newStream{newData};

    callout.flatten(newStream);
    EXPECT_EQ(data, newData);
}

TEST(CalloutTest, TestUnflattenOneSubstructure)
{
    std::vector<uint8_t> data{
        0xFF, 0x28, 'H', 0x08, // size, flags, priority, LC length
        'U',  '1',  '2', '-',  'P', '1', 0x00, 0x00 // LC
    };

    auto fruIdentity = srcDataFactory(TestSRCType::fruIdentityStructure);

    data.insert(data.end(), fruIdentity.begin(), fruIdentity.end());

    // The final size
    data[0] = data.size();

    Stream stream{data};
    Callout callout{stream};

    EXPECT_EQ(callout.flattenedSize(), data.size());

    // Spot check the substructure
    EXPECT_TRUE(callout.fruIdentity());
    EXPECT_EQ(callout.fruIdentity()->getSN(), "123456789ABC");

    // Not present
    EXPECT_FALSE(callout.pceIdentity());
    EXPECT_FALSE(callout.mru());

    // Now flatten
    std::vector<uint8_t> newData;
    Stream newStream{newData};

    callout.flatten(newStream);
    EXPECT_EQ(data, newData);
}

TEST(CalloutTest, TestUnflattenTwoSubstructures)
{
    std::vector<uint8_t> data{
        0xFF, 0x2B, 'H', 0x08, // size, flags, priority, LC length
        'U',  '1',  '2', '-',  'P', '1', 0x00, 0x00 // LC
    };

    auto fruIdentity = srcDataFactory(TestSRCType::fruIdentityStructure);
    auto pceIdentity = srcDataFactory(TestSRCType::pceIdentityStructure);

    data.insert(data.end(), fruIdentity.begin(), fruIdentity.end());
    data.insert(data.end(), pceIdentity.begin(), pceIdentity.end());

    // The final size
    data[0] = data.size();

    Stream stream{data};
    Callout callout{stream};

    EXPECT_EQ(callout.flattenedSize(), data.size());

    // Spot check the 2 substructures
    EXPECT_TRUE(callout.fruIdentity());
    EXPECT_EQ(callout.fruIdentity()->getSN(), "123456789ABC");

    EXPECT_TRUE(callout.pceIdentity());
    EXPECT_EQ(callout.pceIdentity()->enclosureName(), "PCENAME12");

    // Not present
    EXPECT_FALSE(callout.mru());

    // Now flatten
    std::vector<uint8_t> newData;
    Stream newStream{newData};

    callout.flatten(newStream);
    EXPECT_EQ(data, newData);
}

TEST(CalloutTest, TestNoLocationCode)
{
    std::vector<uint8_t> data{
        0xFF, 0x2B, 'H', 0x00 // size, flags, priority, LC length
    };

    auto fruIdentity = srcDataFactory(TestSRCType::fruIdentityStructure);
    data.insert(data.end(), fruIdentity.begin(), fruIdentity.end());

    // The final size
    data[0] = data.size();

    Stream stream{data};
    Callout callout{stream};

    EXPECT_TRUE(callout.locationCode().empty());
}

// Create a callout object by passing in the hardware fields to add
TEST(CalloutTest, TestHardwareCallout)
{
    constexpr size_t fruIdentitySize = 28;

    {
        Callout callout{CalloutPriority::high, "U99-42.5-P1-C2-E1", "1234567",
                        "ABCD", "123456789ABC"};

        // size/flags/pri/locsize fields +
        // rounded up location code length +
        // FRUIdentity size
        size_t size = 4 + 20 + fruIdentitySize;

        EXPECT_EQ(callout.flags(),
                  Callout::calloutType | Callout::fruIdentIncluded);

        EXPECT_EQ(callout.flattenedSize(), size);
        EXPECT_EQ(callout.priority(), 'H');
        EXPECT_EQ(callout.locationCode(), "U99-42.5-P1-C2-E1");
        EXPECT_EQ(callout.locationCodeSize(), 20);

        auto& fru = callout.fruIdentity();
        EXPECT_EQ(fru->getPN().value(), "1234567");
        EXPECT_EQ(fru->getCCIN().value(), "ABCD");
        EXPECT_EQ(fru->getSN().value(), "123456789ABC");
    }

    {
        // A 3B location code, plus null = 4
        Callout callout{CalloutPriority::high, "123", "1234567", "ABCD",
                        "123456789ABC"};

        size_t size = 4 + 4 + fruIdentitySize;
        EXPECT_EQ(callout.locationCodeSize(), 4);
        EXPECT_EQ(callout.flattenedSize(), size);
        EXPECT_EQ(callout.locationCode(), "123");
    }

    {
        // A 4B location code, plus null = 5, then pad to 8
        Callout callout{CalloutPriority::high, "1234", "1234567", "ABCD",
                        "123456789ABC"};

        size_t size = 4 + 8 + fruIdentitySize;
        EXPECT_EQ(callout.locationCodeSize(), 8);
        EXPECT_EQ(callout.flattenedSize(), size);
        EXPECT_EQ(callout.locationCode(), "1234");
    }

    {
        // A truncated location code (80 is max size, including null)
        std::string locCode(81, 'L');
        Callout callout{CalloutPriority::high, locCode, "1234567", "ABCD",
                        "123456789ABC"};

        size_t size = 4 + 80 + fruIdentitySize;
        EXPECT_EQ(callout.locationCodeSize(), 80);
        EXPECT_EQ(callout.flattenedSize(), size);

        // take off 1 to get to 80, and another for the null
        locCode = locCode.substr(0, locCode.size() - 2);
        EXPECT_EQ(callout.locationCode(), locCode);
    }

    {
        // A truncated location code by 1 because of the null
        std::string locCode(80, 'L');
        Callout callout{CalloutPriority::high, locCode, "1234567", "ABCD",
                        "123456789ABC"};

        size_t size = 4 + 80 + fruIdentitySize;
        EXPECT_EQ(callout.locationCodeSize(), 80);
        EXPECT_EQ(callout.flattenedSize(), size);

        locCode.pop_back();
        EXPECT_EQ(callout.locationCode(), locCode);
    }

    {
        // Max size location code
        std::string locCode(79, 'L');
        Callout callout{CalloutPriority::low, locCode, "1234567", "ABCD",
                        "123456789ABC"};

        size_t size = 4 + 80 + fruIdentitySize;
        EXPECT_EQ(callout.locationCodeSize(), 80);
        EXPECT_EQ(callout.flattenedSize(), size);

        EXPECT_EQ(callout.locationCode(), locCode);

        // How about we flatten/unflatten this last one
        std::vector<uint8_t> data;
        Stream stream{data};

        callout.flatten(stream);

        {
            Stream newStream{data};
            Callout newCallout{newStream};

            EXPECT_EQ(newCallout.flags(),
                      Callout::calloutType | Callout::fruIdentIncluded);

            EXPECT_EQ(newCallout.flattenedSize(), callout.flattenedSize());
            EXPECT_EQ(newCallout.priority(), callout.priority());
            EXPECT_EQ(newCallout.locationCode(), callout.locationCode());
            EXPECT_EQ(newCallout.locationCodeSize(),
                      callout.locationCodeSize());

            auto& fru = newCallout.fruIdentity();
            EXPECT_EQ(fru->getPN().value(), "1234567");
            EXPECT_EQ(fru->getCCIN().value(), "ABCD");
            EXPECT_EQ(fru->getSN().value(), "123456789ABC");
        }
    }
}

// Create a callout object by passing in the maintenance procedure to add.
TEST(CalloutTest, TestProcedureCallout)
{
    Callout callout{CalloutPriority::medium, "no_vpd_for_fru"};

    // size/flags/pri/locsize fields + FRUIdentity size
    // No location code.
    size_t size = 4 + 12;

    EXPECT_EQ(callout.flags(),
              Callout::calloutType | Callout::fruIdentIncluded);

    EXPECT_EQ(callout.flattenedSize(), size);
    EXPECT_EQ(callout.priority(), 'M');
    EXPECT_EQ(callout.locationCode(), "");
    EXPECT_EQ(callout.locationCodeSize(), 0);

    auto& fru = callout.fruIdentity();
    EXPECT_EQ(fru->getMaintProc().value(), "BMCSP01");

    // flatten/unflatten
    std::vector<uint8_t> data;
    Stream stream{data};

    callout.flatten(stream);

    Stream newStream{data};
    Callout newCallout{newStream};

    EXPECT_EQ(newCallout.flags(),
              Callout::calloutType | Callout::fruIdentIncluded);

    EXPECT_EQ(newCallout.flattenedSize(), callout.flattenedSize());
    EXPECT_EQ(newCallout.priority(), callout.priority());
    EXPECT_EQ(newCallout.locationCode(), callout.locationCode());
    EXPECT_EQ(newCallout.locationCodeSize(), callout.locationCodeSize());

    auto& newFRU = newCallout.fruIdentity();
    EXPECT_EQ(newFRU->getMaintProc().value(), fru->getMaintProc().value());
}

// Create a callout object by passing in the symbolic FRU to add.
TEST(CalloutTest, TestSymbolicFRUCallout)
{
    // symbolic FRU with a location code
    {
        Callout callout{CalloutPriority::high, "service_docs", "P1-C3", false};

        // size/flags/pri/locsize fields + plus loc + FRUIdentity size
        size_t size = 4 + 8 + 12;

        EXPECT_EQ(callout.flags(),
                  Callout::calloutType | Callout::fruIdentIncluded);

        EXPECT_EQ(callout.flattenedSize(), size);
        EXPECT_EQ(callout.priority(), 'H');
        EXPECT_EQ(callout.locationCode(), "P1-C3");
        EXPECT_EQ(callout.locationCodeSize(), 8);

        auto& fru = callout.fruIdentity();

        EXPECT_EQ(fru->failingComponentType(), FRUIdentity::symbolicFRU);
        EXPECT_EQ(fru->getPN().value(), "SVCDOCS");
    }

    // symbolic FRU without a location code
    {
        Callout callout{CalloutPriority::high, "service_docs", "", false};

        // size/flags/pri/locsize fields + plus loc + FRUIdentity size
        size_t size = 4 + 0 + 12;

        EXPECT_EQ(callout.flags(),
                  Callout::calloutType | Callout::fruIdentIncluded);

        EXPECT_EQ(callout.flattenedSize(), size);
        EXPECT_EQ(callout.priority(), 'H');
        EXPECT_EQ(callout.locationCode(), "");
        EXPECT_EQ(callout.locationCodeSize(), 0);

        auto& fru = callout.fruIdentity();

        EXPECT_EQ(fru->failingComponentType(), FRUIdentity::symbolicFRU);
        EXPECT_EQ(fru->getPN().value(), "SVCDOCS");
    }

    // symbolic FRU with a trusted location code
    {
        Callout callout{CalloutPriority::high, "service_docs", "P1-C3", true};

        // size/flags/pri/locsize fields + plus loc + FRUIdentity size
        size_t size = 4 + 8 + 12;

        EXPECT_EQ(callout.flags(),
                  Callout::calloutType | Callout::fruIdentIncluded);

        EXPECT_EQ(callout.flattenedSize(), size);
        EXPECT_EQ(callout.priority(), 'H');
        EXPECT_EQ(callout.locationCode(), "P1-C3");
        EXPECT_EQ(callout.locationCodeSize(), 8);

        auto& fru = callout.fruIdentity();
        EXPECT_EQ(fru->failingComponentType(),
                  FRUIdentity::symbolicFRUTrustedLocCode);
        EXPECT_EQ(fru->getPN().value(), "SVCDOCS");
    }
}
