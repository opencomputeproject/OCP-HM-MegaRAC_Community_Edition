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
#include "extensions/openpower-pels/src.hpp"
#include "mocks.hpp"
#include "pel_utils.hpp"

#include <fstream>

#include <gtest/gtest.h>

using namespace openpower::pels;
using ::testing::_;
using ::testing::InvokeWithoutArgs;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgReferee;
namespace fs = std::filesystem;

const auto testRegistry = R"(
{
"PELs":
[
    {
        "Name": "xyz.openbmc_project.Error.Test",
        "Subsystem": "bmc_firmware",
        "SRC":
        {
            "ReasonCode": "0xABCD",
            "Words6To9":
            {
                "6":
                {
                    "Description": "Component ID",
                    "AdditionalDataPropSource": "COMPID"
                },
                "7":
                {
                    "Description": "Failure count",
                    "AdditionalDataPropSource": "FREQUENCY"
                },
                "8":
                {
                    "Description": "Time period",
                    "AdditionalDataPropSource": "DURATION"
                },
                "9":
                {
                    "Description": "Error code",
                    "AdditionalDataPropSource": "ERRORCODE"
                }
            }
        },
        "Documentation":
        {
            "Description": "A Component Fault",
            "Message": "Comp %1 failed %2 times over %3 secs with ErrorCode %4",
            "MessageArgSources":
            [
                "SRCWord6", "SRCWord7", "SRCWord8", "SRCWord9"
            ]
        }
    }
]
}
)";

class SRCTest : public ::testing::Test
{
  protected:
    static void SetUpTestCase()
    {
        char path[] = "/tmp/srctestXXXXXX";
        regDir = mkdtemp(path);
    }

    static void TearDownTestCase()
    {
        fs::remove_all(regDir);
    }

    static std::string writeData(const char* data)
    {
        fs::path path = regDir / "registry.json";
        std::ofstream stream{path};
        stream << data;
        return path;
    }

    static fs::path regDir;
};

fs::path SRCTest::regDir{};

TEST_F(SRCTest, UnflattenFlattenTestNoCallouts)
{
    auto data = pelDataFactory(TestPELType::primarySRCSection);

    Stream stream{data};
    SRC src{stream};

    EXPECT_TRUE(src.valid());

    EXPECT_EQ(src.header().id, 0x5053);
    EXPECT_EQ(src.header().size, 0x50);
    EXPECT_EQ(src.header().version, 0x01);
    EXPECT_EQ(src.header().subType, 0x01);
    EXPECT_EQ(src.header().componentID, 0x0202);

    EXPECT_EQ(src.version(), 0x02);
    EXPECT_EQ(src.flags(), 0x00);
    EXPECT_EQ(src.hexWordCount(), 9);
    EXPECT_EQ(src.size(), 0x48);

    const auto& hexwords = src.hexwordData();
    EXPECT_EQ(0x02020255, hexwords[0]);
    EXPECT_EQ(0x03030310, hexwords[1]);
    EXPECT_EQ(0x04040404, hexwords[2]);
    EXPECT_EQ(0x05050505, hexwords[3]);
    EXPECT_EQ(0x06060606, hexwords[4]);
    EXPECT_EQ(0x07070707, hexwords[5]);
    EXPECT_EQ(0x08080808, hexwords[6]);
    EXPECT_EQ(0x09090909, hexwords[7]);

    EXPECT_EQ(src.asciiString(), "BD8D5678                        ");
    EXPECT_FALSE(src.callouts());

    // Flatten
    std::vector<uint8_t> newData;
    Stream newStream{newData};

    src.flatten(newStream);
    EXPECT_EQ(data, newData);
}

TEST_F(SRCTest, UnflattenFlattenTest2Callouts)
{
    auto data = pelDataFactory(TestPELType::primarySRCSection2Callouts);

    Stream stream{data};
    SRC src{stream};

    EXPECT_TRUE(src.valid());
    EXPECT_EQ(src.flags(), 0x01); // Additional sections within the SRC.

    // Spot check the SRC fields, but they're the same as above
    EXPECT_EQ(src.asciiString(), "BD8D5678                        ");

    // There should be 2 callouts
    const auto& calloutsSection = src.callouts();
    ASSERT_TRUE(calloutsSection);
    const auto& callouts = calloutsSection->callouts();
    EXPECT_EQ(callouts.size(), 2);

    // spot check that each callout has the right substructures
    EXPECT_TRUE(callouts.front()->fruIdentity());
    EXPECT_FALSE(callouts.front()->pceIdentity());
    EXPECT_FALSE(callouts.front()->mru());

    EXPECT_TRUE(callouts.back()->fruIdentity());
    EXPECT_TRUE(callouts.back()->pceIdentity());
    EXPECT_TRUE(callouts.back()->mru());

    // Flatten
    std::vector<uint8_t> newData;
    Stream newStream{newData};

    src.flatten(newStream);
    EXPECT_EQ(data, newData);
}

// Create an SRC from the message registry
TEST_F(SRCTest, CreateTestNoCallouts)
{
    message::Entry entry;
    entry.src.type = 0xBD;
    entry.src.reasonCode = 0xABCD;
    entry.subsystem = 0x42;
    entry.src.powerFault = true;
    entry.src.hexwordADFields = {{5, "TEST1"}, // Not a user defined word
                                 {6, "TEST1"},
                                 {7, "TEST2"},
                                 {8, "TEST3"},
                                 {9, "TEST4"}};

    // Values for the SRC words pointed to above
    std::vector<std::string> adData{"TEST1=0x12345678", "TEST2=12345678",
                                    "TEST3=0XDEF", "TEST4=Z"};
    AdditionalData ad{adData};
    NiceMock<MockDataInterface> dataIface;

    EXPECT_CALL(dataIface, getMotherboardCCIN).WillOnce(Return("ABCD"));

    SRC src{entry, ad, dataIface};

    EXPECT_TRUE(src.valid());
    EXPECT_TRUE(src.isPowerFaultEvent());
    EXPECT_EQ(src.size(), baseSRCSize);

    const auto& hexwords = src.hexwordData();

    // The spec always refers to SRC words 2 - 9, and as the hexwordData()
    // array index starts at 0 use the math in the [] below to make it easier
    // to tell what is being accessed.
    EXPECT_EQ(hexwords[2 - 2] & 0xF0000000, 0);    // Partition dump status
    EXPECT_EQ(hexwords[2 - 2] & 0x00F00000, 0);    // Partition boot type
    EXPECT_EQ(hexwords[2 - 2] & 0x000000FF, 0x55); // SRC format
    EXPECT_EQ(hexwords[3 - 2] & 0x000000FF, 0x10); // BMC position
    EXPECT_EQ(hexwords[3 - 2] & 0xFFFF0000, 0xABCD0000); // Motherboard CCIN

    // Validate more fields here as the code starts filling them in.

    // Ensure hex word 5 wasn't allowed to be set to TEST1's contents
    EXPECT_EQ(hexwords[5 - 2], 0);

    // The user defined hex word fields specifed in the additional data.
    EXPECT_EQ(hexwords[6 - 2], 0x12345678); // TEST1
    EXPECT_EQ(hexwords[7 - 2], 12345678);   // TEST2
    EXPECT_EQ(hexwords[8 - 2], 0xdef);      // TEST3
    EXPECT_EQ(hexwords[9 - 2], 0);          // TEST4, but can't convert a 'Z'

    EXPECT_EQ(src.asciiString(), "BD42ABCD                        ");

    // No callouts
    EXPECT_FALSE(src.callouts());

    // May as well spot check the flatten/unflatten
    std::vector<uint8_t> data;
    Stream stream{data};
    src.flatten(stream);

    stream.offset(0);
    SRC newSRC{stream};

    EXPECT_TRUE(newSRC.valid());
    EXPECT_EQ(newSRC.isPowerFaultEvent(), src.isPowerFaultEvent());
    EXPECT_EQ(newSRC.asciiString(), src.asciiString());
    EXPECT_FALSE(newSRC.callouts());
}

// Test when the CCIN string isn't a 4 character number
TEST_F(SRCTest, BadCCINTest)
{
    message::Entry entry;
    entry.src.type = 0xBD;
    entry.src.reasonCode = 0xABCD;
    entry.subsystem = 0x42;
    entry.src.powerFault = false;

    std::vector<std::string> adData{};
    AdditionalData ad{adData};
    NiceMock<MockDataInterface> dataIface;

    // First it isn't a number, then it is too long,
    // then it is empty.
    EXPECT_CALL(dataIface, getMotherboardCCIN)
        .WillOnce(Return("X"))
        .WillOnce(Return("12345"))
        .WillOnce(Return(""));

    // The CCIN in the first half should still be 0 each time.
    {
        SRC src{entry, ad, dataIface};
        EXPECT_TRUE(src.valid());
        const auto& hexwords = src.hexwordData();
        EXPECT_EQ(hexwords[3 - 2] & 0xFFFF0000, 0x00000000);
    }

    {
        SRC src{entry, ad, dataIface};
        EXPECT_TRUE(src.valid());
        const auto& hexwords = src.hexwordData();
        EXPECT_EQ(hexwords[3 - 2] & 0xFFFF0000, 0x00000000);
    }

    {
        SRC src{entry, ad, dataIface};
        EXPECT_TRUE(src.valid());
        const auto& hexwords = src.hexwordData();
        EXPECT_EQ(hexwords[3 - 2] & 0xFFFF0000, 0x00000000);
    }
}

// Test the getErrorDetails function
TEST_F(SRCTest, MessageSubstitutionTest)
{
    auto path = SRCTest::writeData(testRegistry);
    message::Registry registry{path};
    auto entry = registry.lookup("0xABCD", message::LookupType::reasonCode);

    std::vector<std::string> adData{"COMPID=0x1", "FREQUENCY=0x4",
                                    "DURATION=30", "ERRORCODE=0x01ABCDEF"};
    AdditionalData ad{adData};
    NiceMock<MockDataInterface> dataIface;

    SRC src{*entry, ad, dataIface};
    EXPECT_TRUE(src.valid());

    auto errorDetails = src.getErrorDetails(registry, DetailLevel::message);
    ASSERT_TRUE(errorDetails);
    EXPECT_EQ(
        errorDetails.value(),
        "Comp 0x1 failed 0x4 times over 0x1E secs with ErrorCode 0x1ABCDEF");
}

// Test that an inventory path callout string is
// converted into the appropriate FRU callout.
TEST_F(SRCTest, InventoryCalloutTest)
{
    message::Entry entry;
    entry.src.type = 0xBD;
    entry.src.reasonCode = 0xABCD;
    entry.subsystem = 0x42;
    entry.src.powerFault = false;

    std::vector<std::string> adData{"CALLOUT_INVENTORY_PATH=motherboard"};
    AdditionalData ad{adData};
    NiceMock<MockDataInterface> dataIface;

    EXPECT_CALL(dataIface, getLocationCode("motherboard"))
        .WillOnce(Return("UTMS-P1"));

    EXPECT_CALL(dataIface, getHWCalloutFields("motherboard", _, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<1>("1234567"), SetArgReferee<2>("CCCC"),
                        SetArgReferee<3>("123456789ABC")));

    SRC src{entry, ad, dataIface};
    EXPECT_TRUE(src.valid());

    ASSERT_TRUE(src.callouts());

    EXPECT_EQ(src.callouts()->callouts().size(), 1);

    auto& callout = src.callouts()->callouts().front();

    EXPECT_EQ(callout->locationCode(), "UTMS-P1");
    EXPECT_EQ(callout->priority(), 'H');

    auto& fru = callout->fruIdentity();

    EXPECT_EQ(fru->getPN().value(), "1234567");
    EXPECT_EQ(fru->getCCIN().value(), "CCCC");
    EXPECT_EQ(fru->getSN().value(), "123456789ABC");

    // flatten and unflatten
    std::vector<uint8_t> data;
    Stream stream{data};
    src.flatten(stream);

    stream.offset(0);
    SRC newSRC{stream};
    EXPECT_TRUE(newSRC.valid());
    ASSERT_TRUE(src.callouts());
    EXPECT_EQ(src.callouts()->callouts().size(), 1);
}

// Test that when the location code can't be obtained that
// a procedure callout is used.
TEST_F(SRCTest, InventoryCalloutNoLocCodeTest)
{
    message::Entry entry;
    entry.src.type = 0xBD;
    entry.src.reasonCode = 0xABCD;
    entry.subsystem = 0x42;
    entry.src.powerFault = false;

    std::vector<std::string> adData{"CALLOUT_INVENTORY_PATH=motherboard"};
    AdditionalData ad{adData};
    NiceMock<MockDataInterface> dataIface;

    auto func = []() {
        throw sdbusplus::exception::SdBusError(5, "Error");
        return std::string{};
    };

    EXPECT_CALL(dataIface, getLocationCode("motherboard"))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(func));

    EXPECT_CALL(dataIface, getHWCalloutFields(_, _, _, _)).Times(0);

    SRC src{entry, ad, dataIface};
    EXPECT_TRUE(src.valid());

    ASSERT_TRUE(src.callouts());

    EXPECT_EQ(src.callouts()->callouts().size(), 1);

    auto& callout = src.callouts()->callouts().front();
    EXPECT_EQ(callout->locationCodeSize(), 0);
    EXPECT_EQ(callout->priority(), 'H');

    auto& fru = callout->fruIdentity();

    EXPECT_EQ(fru->getMaintProc().value(), "BMCSP01");
    EXPECT_FALSE(fru->getPN());
    EXPECT_FALSE(fru->getSN());
    EXPECT_FALSE(fru->getCCIN());

    // flatten and unflatten
    std::vector<uint8_t> data;
    Stream stream{data};
    src.flatten(stream);

    stream.offset(0);
    SRC newSRC{stream};
    EXPECT_TRUE(newSRC.valid());
    ASSERT_TRUE(src.callouts());
    EXPECT_EQ(src.callouts()->callouts().size(), 1);
}

// Test that when the VPD can't be obtained that
// a callout is still created.
TEST_F(SRCTest, InventoryCalloutNoVPDTest)
{
    message::Entry entry;
    entry.src.type = 0xBD;
    entry.src.reasonCode = 0xABCD;
    entry.subsystem = 0x42;
    entry.src.powerFault = false;

    std::vector<std::string> adData{"CALLOUT_INVENTORY_PATH=motherboard"};
    AdditionalData ad{adData};
    NiceMock<MockDataInterface> dataIface;

    EXPECT_CALL(dataIface, getLocationCode("motherboard"))
        .Times(1)
        .WillOnce(Return("UTMS-P10"));

    auto func = []() { throw sdbusplus::exception::SdBusError(5, "Error"); };

    EXPECT_CALL(dataIface, getHWCalloutFields("motherboard", _, _, _))
        .Times(1)
        .WillOnce(InvokeWithoutArgs(func));

    SRC src{entry, ad, dataIface};
    EXPECT_TRUE(src.valid());
    ASSERT_TRUE(src.callouts());
    EXPECT_EQ(src.callouts()->callouts().size(), 1);

    auto& callout = src.callouts()->callouts().front();
    EXPECT_EQ(callout->locationCode(), "UTMS-P10");
    EXPECT_EQ(callout->priority(), 'H');

    auto& fru = callout->fruIdentity();

    EXPECT_EQ(fru->getPN(), "");
    EXPECT_EQ(fru->getCCIN(), "");
    EXPECT_EQ(fru->getSN(), "");
    EXPECT_FALSE(fru->getMaintProc());

    // flatten and unflatten
    std::vector<uint8_t> data;
    Stream stream{data};
    src.flatten(stream);

    stream.offset(0);
    SRC newSRC{stream};
    EXPECT_TRUE(newSRC.valid());
    ASSERT_TRUE(src.callouts());
    EXPECT_EQ(src.callouts()->callouts().size(), 1);
}

TEST_F(SRCTest, RegistryCalloutTest)
{
    message::Entry entry;
    entry.src.type = 0xBD;
    entry.src.reasonCode = 0xABCD;
    entry.subsystem = 0x42;
    entry.src.powerFault = false;

    entry.callouts = R"(
        [
        {
            "System": "systemA",
            "CalloutList":
            [
                {
                    "Priority": "high",
                    "SymbolicFRU": "service_docs"
                },
                {
                    "Priority": "medium",
                    "Procedure": "no_vpd_for_fru"
                }
            ]
        },
        {
            "System": "systemB",
            "CalloutList":
            [
                {
                    "Priority": "high",
                    "LocCode": "P0-C8",
                    "SymbolicFRUTrusted": "service_docs"
                },
                {
                    "Priority": "medium",
                    "SymbolicFRUTrusted": "service_docs"
                }
            ]
        },
        {
            "System": "systemC",
            "CalloutList":
            [
                {
                    "Priority": "high",
                    "LocCode": "P0-C8"
                },
                {
                    "Priority": "medium",
                    "LocCode": "P0-C9"
                }
            ]
        }
        ])"_json;

    {
        // Call out a symbolic FRU and a procedure
        AdditionalData ad;
        NiceMock<MockDataInterface> dataIface;
        std::vector<std::string> names{"systemA"};

        EXPECT_CALL(dataIface, getSystemNames).WillOnce(ReturnRef(names));

        SRC src{entry, ad, dataIface};

        auto& callouts = src.callouts()->callouts();
        ASSERT_EQ(callouts.size(), 2);

        EXPECT_EQ(callouts[0]->locationCodeSize(), 0);
        EXPECT_EQ(callouts[0]->priority(), 'H');

        EXPECT_EQ(callouts[1]->locationCodeSize(), 0);
        EXPECT_EQ(callouts[1]->priority(), 'M');

        auto& fru1 = callouts[0]->fruIdentity();
        EXPECT_EQ(fru1->getPN().value(), "SVCDOCS");
        EXPECT_EQ(fru1->failingComponentType(), src::FRUIdentity::symbolicFRU);
        EXPECT_FALSE(fru1->getMaintProc());
        EXPECT_FALSE(fru1->getSN());
        EXPECT_FALSE(fru1->getCCIN());

        auto& fru2 = callouts[1]->fruIdentity();
        EXPECT_EQ(fru2->getMaintProc().value(), "BMCSP01");
        EXPECT_EQ(fru2->failingComponentType(),
                  src::FRUIdentity::maintenanceProc);
        EXPECT_FALSE(fru2->getPN());
        EXPECT_FALSE(fru2->getSN());
        EXPECT_FALSE(fru2->getCCIN());
    }

    {
        // Call out a trusted symbolic FRU with a location code, and
        // another one without.
        AdditionalData ad;
        NiceMock<MockDataInterface> dataIface;
        std::vector<std::string> names{"systemB"};

        EXPECT_CALL(dataIface, expandLocationCode).WillOnce(Return("P0-C8"));
        EXPECT_CALL(dataIface, getSystemNames).WillOnce(ReturnRef(names));

        SRC src{entry, ad, dataIface};

        auto& callouts = src.callouts()->callouts();
        EXPECT_EQ(callouts.size(), 2);

        EXPECT_EQ(callouts[0]->locationCode(), "P0-C8");
        EXPECT_EQ(callouts[0]->priority(), 'H');

        EXPECT_EQ(callouts[1]->locationCodeSize(), 0);
        EXPECT_EQ(callouts[1]->priority(), 'M');

        auto& fru1 = callouts[0]->fruIdentity();
        EXPECT_EQ(fru1->getPN().value(), "SVCDOCS");
        EXPECT_EQ(fru1->failingComponentType(),
                  src::FRUIdentity::symbolicFRUTrustedLocCode);
        EXPECT_FALSE(fru1->getMaintProc());
        EXPECT_FALSE(fru1->getSN());
        EXPECT_FALSE(fru1->getCCIN());

        // It asked for a trusted symbolic FRU, but no location code
        // was provided so it is switched back to a normal one
        auto& fru2 = callouts[1]->fruIdentity();
        EXPECT_EQ(fru2->getPN().value(), "SVCDOCS");
        EXPECT_EQ(fru2->failingComponentType(), src::FRUIdentity::symbolicFRU);
        EXPECT_FALSE(fru2->getMaintProc());
        EXPECT_FALSE(fru2->getSN());
        EXPECT_FALSE(fru2->getCCIN());
    }

    {
        // Two hardware callouts
        AdditionalData ad;
        NiceMock<MockDataInterface> dataIface;
        std::vector<std::string> names{"systemC"};

        EXPECT_CALL(dataIface, getSystemNames).WillOnce(ReturnRef(names));

        EXPECT_CALL(dataIface, expandLocationCode("P0-C8", 0))
            .WillOnce(Return("UXXX-P0-C8"));

        EXPECT_CALL(dataIface, expandLocationCode("P0-C9", 0))
            .WillOnce(Return("UXXX-P0-C9"));

        EXPECT_CALL(dataIface, getInventoryFromLocCode("P0-C8", 0))
            .WillOnce(Return(
                "/xyz/openbmc_project/inventory/chassis/motherboard/cpu0"));

        EXPECT_CALL(dataIface, getInventoryFromLocCode("P0-C9", 0))
            .WillOnce(Return(
                "/xyz/openbmc_project/inventory/chassis/motherboard/cpu1"));

        EXPECT_CALL(
            dataIface,
            getHWCalloutFields(
                "/xyz/openbmc_project/inventory/chassis/motherboard/cpu0", _, _,
                _))
            .Times(1)
            .WillOnce(DoAll(SetArgReferee<1>("1234567"),
                            SetArgReferee<2>("CCCC"),
                            SetArgReferee<3>("123456789ABC")));

        EXPECT_CALL(
            dataIface,
            getHWCalloutFields(
                "/xyz/openbmc_project/inventory/chassis/motherboard/cpu1", _, _,
                _))
            .Times(1)
            .WillOnce(DoAll(SetArgReferee<1>("2345678"),
                            SetArgReferee<2>("DDDD"),
                            SetArgReferee<3>("23456789ABCD")));

        SRC src{entry, ad, dataIface};

        auto& callouts = src.callouts()->callouts();
        EXPECT_EQ(callouts.size(), 2);

        EXPECT_EQ(callouts[0]->locationCode(), "UXXX-P0-C8");
        EXPECT_EQ(callouts[0]->priority(), 'H');

        auto& fru1 = callouts[0]->fruIdentity();
        EXPECT_EQ(fru1->getPN().value(), "1234567");
        EXPECT_EQ(fru1->getCCIN().value(), "CCCC");
        EXPECT_EQ(fru1->getSN().value(), "123456789ABC");

        EXPECT_EQ(callouts[1]->locationCode(), "UXXX-P0-C9");
        EXPECT_EQ(callouts[1]->priority(), 'M');

        auto& fru2 = callouts[1]->fruIdentity();
        EXPECT_EQ(fru2->getPN().value(), "2345678");
        EXPECT_EQ(fru2->getCCIN().value(), "DDDD");
        EXPECT_EQ(fru2->getSN().value(), "23456789ABCD");
    }
}

// Test looking up device path fails in the callout jSON.
TEST_F(SRCTest, DevicePathCalloutTest)
{
    message::Entry entry;
    entry.src.type = 0xBD;
    entry.src.reasonCode = 0xABCD;
    entry.subsystem = 0x42;
    entry.src.powerFault = false;

    const auto calloutJSON = R"(
    {
        "I2C":
        {
            "14":
            {
                "114":
                {
                    "Callouts":[
                    {
                        "Name": "/chassis/motherboard/cpu0",
                        "LocationCode": "P1-C40",
                        "Priority": "H"
                    },
                    {
                        "Name": "/chassis/motherboard",
                        "LocationCode": "P1",
                        "Priority": "M"
                    },
                    {
                        "Name": "/chassis/motherboard/bmc",
                        "LocationCode": "P1-C15",
                        "Priority": "L"
                    }
                    ],
                    "Dest": "proc 0 target"
                }
            }
        }
    })";

    auto dataPath = getPELReadOnlyDataPath();
    std::ofstream file{dataPath / "systemA_dev_callouts.json"};
    file << calloutJSON;
    file.close();

    NiceMock<MockDataInterface> dataIface;
    std::vector<std::string> names{"systemA"};

    EXPECT_CALL(dataIface, getSystemNames)
        .Times(5)
        .WillRepeatedly(ReturnRef(names));

    EXPECT_CALL(dataIface, getInventoryFromLocCode("P1-C40", 0))
        .Times(3)
        .WillRepeatedly(
            Return("/xyz/openbmc_project/inventory/chassis/motherboard/cpu0"));

    EXPECT_CALL(dataIface, getInventoryFromLocCode("P1", 0))
        .Times(3)
        .WillRepeatedly(
            Return("/xyz/openbmc_project/inventory/chassis/motherboard"));

    EXPECT_CALL(dataIface, getInventoryFromLocCode("P1-C15", 0))
        .Times(3)
        .WillRepeatedly(
            Return("/xyz/openbmc_project/inventory/chassis/motherboard/bmc"));

    EXPECT_CALL(dataIface,
                getLocationCode(
                    "/xyz/openbmc_project/inventory/chassis/motherboard/cpu0"))
        .Times(3)
        .WillRepeatedly(Return("Ufcs-P1-C40"));
    EXPECT_CALL(
        dataIface,
        getLocationCode("/xyz/openbmc_project/inventory/chassis/motherboard"))
        .Times(3)
        .WillRepeatedly(Return("Ufcs-P1"));
    EXPECT_CALL(dataIface,
                getLocationCode(
                    "/xyz/openbmc_project/inventory/chassis/motherboard/bmc"))
        .Times(3)
        .WillRepeatedly(Return("Ufcs-P1-C15"));

    EXPECT_CALL(
        dataIface,
        getHWCalloutFields(
            "/xyz/openbmc_project/inventory/chassis/motherboard/cpu0", _, _, _))
        .Times(3)
        .WillRepeatedly(DoAll(SetArgReferee<1>("1234567"),
                              SetArgReferee<2>("CCCC"),
                              SetArgReferee<3>("123456789ABC")));
    EXPECT_CALL(
        dataIface,
        getHWCalloutFields("/xyz/openbmc_project/inventory/chassis/motherboard",
                           _, _, _))
        .Times(3)
        .WillRepeatedly(DoAll(SetArgReferee<1>("7654321"),
                              SetArgReferee<2>("MMMM"),
                              SetArgReferee<3>("CBA987654321")));
    EXPECT_CALL(
        dataIface,
        getHWCalloutFields(
            "/xyz/openbmc_project/inventory/chassis/motherboard/bmc", _, _, _))
        .Times(3)
        .WillRepeatedly(DoAll(SetArgReferee<1>("7123456"),
                              SetArgReferee<2>("BBBB"),
                              SetArgReferee<3>("C123456789AB")));

    // Call this below with different AdditionalData values that
    // result in the same callouts.
    auto checkCallouts = [&entry, &dataIface](const auto& items) {
        AdditionalData ad{items};
        SRC src{entry, ad, dataIface};

        ASSERT_TRUE(src.callouts());
        auto& callouts = src.callouts()->callouts();

        ASSERT_EQ(callouts.size(), 3);

        {
            EXPECT_EQ(callouts[0]->priority(), 'H');
            EXPECT_EQ(callouts[0]->locationCode(), "Ufcs-P1-C40");

            auto& fru = callouts[0]->fruIdentity();
            EXPECT_EQ(fru->getPN().value(), "1234567");
            EXPECT_EQ(fru->getCCIN().value(), "CCCC");
            EXPECT_EQ(fru->getSN().value(), "123456789ABC");
        }
        {
            EXPECT_EQ(callouts[1]->priority(), 'M');
            EXPECT_EQ(callouts[1]->locationCode(), "Ufcs-P1");

            auto& fru = callouts[1]->fruIdentity();
            EXPECT_EQ(fru->getPN().value(), "7654321");
            EXPECT_EQ(fru->getCCIN().value(), "MMMM");
            EXPECT_EQ(fru->getSN().value(), "CBA987654321");
        }
        {
            EXPECT_EQ(callouts[2]->priority(), 'L');
            EXPECT_EQ(callouts[2]->locationCode(), "Ufcs-P1-C15");

            auto& fru = callouts[2]->fruIdentity();
            EXPECT_EQ(fru->getPN().value(), "7123456");
            EXPECT_EQ(fru->getCCIN().value(), "BBBB");
            EXPECT_EQ(fru->getSN().value(), "C123456789AB");
        }
    };

    {
        // Callouts based on the device path
        std::vector<std::string> items{
            "CALLOUT_ERRNO=5",
            "CALLOUT_DEVICE_PATH=/sys/devices/platform/ahb/ahb:apb/"
            "ahb:apb:bus@1e78a000/1e78a340.i2c-bus/i2c-14/14-0072"};

        checkCallouts(items);
    }

    {
        // Callouts based on the I2C bus and address
        std::vector<std::string> items{"CALLOUT_ERRNO=5", "CALLOUT_IIC_BUS=14",
                                       "CALLOUT_IIC_ADDR=0x72"};
        checkCallouts(items);
    }

    {
        // Also based on I2C bus and address, but with bus = /dev/i2c-14
        std::vector<std::string> items{"CALLOUT_ERRNO=5", "CALLOUT_IIC_BUS=14",
                                       "CALLOUT_IIC_ADDR=0x72"};
        checkCallouts(items);
    }

    {
        // Callout not found
        std::vector<std::string> items{
            "CALLOUT_ERRNO=5",
            "CALLOUT_DEVICE_PATH=/sys/devices/platform/ahb/ahb:apb/"
            "ahb:apb:bus@1e78a000/1e78a340.i2c-bus/i2c-24/24-0012"};

        AdditionalData ad{items};
        SRC src{entry, ad, dataIface};

        EXPECT_FALSE(src.callouts());
        ASSERT_EQ(src.getDebugData().size(), 1);
        EXPECT_EQ(src.getDebugData()[0],
                  "Problem looking up I2C callouts on 24 18: "
                  "[json.exception.out_of_range.403] key '24' not found");
    }

    {
        // Callout not found
        std::vector<std::string> items{"CALLOUT_ERRNO=5", "CALLOUT_IIC_BUS=22",
                                       "CALLOUT_IIC_ADDR=0x99"};
        AdditionalData ad{items};
        SRC src{entry, ad, dataIface};

        EXPECT_FALSE(src.callouts());
        ASSERT_EQ(src.getDebugData().size(), 1);
        EXPECT_EQ(src.getDebugData()[0],
                  "Problem looking up I2C callouts on 22 153: "
                  "[json.exception.out_of_range.403] key '22' not found");
    }

    fs::remove_all(dataPath);
}
