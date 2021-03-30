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
#include "elog_entry.hpp"
#include "extensions/openpower-pels/generic.hpp"
#include "extensions/openpower-pels/pel.hpp"
#include "mocks.hpp"
#include "pel_utils.hpp"

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;
using namespace openpower::pels;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgReferee;

class PELTest : public CleanLogID
{
};

fs::path makeTempDir()
{
    char path[] = "/tmp/tempdirXXXXXX";
    std::filesystem::path dir = mkdtemp(path);
    return dir;
}

int writeFileAndGetFD(const fs::path& dir, const std::vector<uint8_t>& data)
{
    static size_t count = 0;
    fs::path path = dir / (std::string{"file"} + std::to_string(count));
    std::ofstream stream{path};
    count++;

    stream.write(reinterpret_cast<const char*>(data.data()), data.size());
    stream.close();

    FILE* fp = fopen(path.c_str(), "r");
    return fileno(fp);
}

TEST_F(PELTest, FlattenTest)
{
    auto data = pelDataFactory(TestPELType::pelSimple);
    auto pel = std::make_unique<PEL>(data);

    // Check a few fields
    EXPECT_TRUE(pel->valid());
    EXPECT_EQ(pel->id(), 0x80818283);
    EXPECT_EQ(pel->plid(), 0x50515253);
    EXPECT_EQ(pel->userHeader().subsystem(), 0x10);
    EXPECT_EQ(pel->userHeader().actionFlags(), 0x80C0);

    // Test that data in == data out
    auto flattenedData = pel->data();
    EXPECT_EQ(data, flattenedData);
    EXPECT_EQ(flattenedData.size(), pel->size());
}

TEST_F(PELTest, CommitTimeTest)
{
    auto data = pelDataFactory(TestPELType::pelSimple);
    auto pel = std::make_unique<PEL>(data);

    auto origTime = pel->commitTime();
    pel->setCommitTime();
    auto newTime = pel->commitTime();

    EXPECT_NE(origTime, newTime);

    // Make a new PEL and check new value is still there
    auto newData = pel->data();
    auto newPel = std::make_unique<PEL>(newData);
    EXPECT_EQ(newTime, newPel->commitTime());
}

TEST_F(PELTest, AssignIDTest)
{
    auto data = pelDataFactory(TestPELType::pelSimple);
    auto pel = std::make_unique<PEL>(data);

    auto origID = pel->id();
    pel->assignID();
    auto newID = pel->id();

    EXPECT_NE(origID, newID);

    // Make a new PEL and check new value is still there
    auto newData = pel->data();
    auto newPel = std::make_unique<PEL>(newData);
    EXPECT_EQ(newID, newPel->id());
}

TEST_F(PELTest, WithLogIDTest)
{
    auto data = pelDataFactory(TestPELType::pelSimple);
    auto pel = std::make_unique<PEL>(data, 0x42);

    EXPECT_TRUE(pel->valid());
    EXPECT_EQ(pel->obmcLogID(), 0x42);
}

TEST_F(PELTest, InvalidPELTest)
{
    auto data = pelDataFactory(TestPELType::pelSimple);

    // Too small
    data.resize(PrivateHeader::flattenedSize());

    auto pel = std::make_unique<PEL>(data);

    EXPECT_TRUE(pel->privateHeader().valid());
    EXPECT_FALSE(pel->userHeader().valid());
    EXPECT_FALSE(pel->valid());

    // Now corrupt the private header
    data = pelDataFactory(TestPELType::pelSimple);
    data.at(0) = 0;
    pel = std::make_unique<PEL>(data);

    EXPECT_FALSE(pel->privateHeader().valid());
    EXPECT_TRUE(pel->userHeader().valid());
    EXPECT_FALSE(pel->valid());
}

TEST_F(PELTest, EmptyDataTest)
{
    std::vector<uint8_t> data;
    auto pel = std::make_unique<PEL>(data);

    EXPECT_FALSE(pel->privateHeader().valid());
    EXPECT_FALSE(pel->userHeader().valid());
    EXPECT_FALSE(pel->valid());
}

TEST_F(PELTest, CreateFromRegistryTest)
{
    message::Entry regEntry;
    uint64_t timestamp = 5;

    regEntry.name = "test";
    regEntry.subsystem = 5;
    regEntry.actionFlags = 0xC000;
    regEntry.src.type = 0xBD;
    regEntry.src.reasonCode = 0x1234;

    std::vector<std::string> data{"KEY1=VALUE1"};
    AdditionalData ad{data};
    NiceMock<MockDataInterface> dataIface;
    PelFFDC ffdc;

    PEL pel{regEntry, 42,   timestamp, phosphor::logging::Entry::Level::Error,
            ad,       ffdc, dataIface};

    EXPECT_TRUE(pel.valid());
    EXPECT_EQ(pel.privateHeader().obmcLogID(), 42);
    EXPECT_EQ(pel.userHeader().severity(), 0x40);

    EXPECT_EQ(pel.primarySRC().value()->asciiString(),
              "BD051234                        ");

    // Check that certain optional sections have been created
    size_t mtmsCount = 0;
    size_t euhCount = 0;
    size_t udCount = 0;

    for (const auto& section : pel.optionalSections())
    {
        if (section->header().id ==
            static_cast<uint16_t>(SectionID::failingMTMS))
        {
            mtmsCount++;
        }
        else if (section->header().id ==
                 static_cast<uint16_t>(SectionID::extendedUserHeader))
        {
            euhCount++;
        }
        else if (section->header().id ==
                 static_cast<uint16_t>(SectionID::userData))
        {
            udCount++;
        }
    }

    EXPECT_EQ(mtmsCount, 1);
    EXPECT_EQ(euhCount, 1);
    EXPECT_EQ(udCount, 2); // AD section and sysInfo section
}

// Test that when the AdditionalData size is over 16KB that
// the PEL that's created is exactly 16KB since the UserData
// section that contains all that data was pruned.
TEST_F(PELTest, CreateTooBigADTest)
{
    message::Entry regEntry;
    uint64_t timestamp = 5;

    regEntry.name = "test";
    regEntry.subsystem = 5;
    regEntry.actionFlags = 0xC000;
    regEntry.src.type = 0xBD;
    regEntry.src.reasonCode = 0x1234;
    PelFFDC ffdc;

    // Over the 16KB max PEL size
    std::string bigAD{"KEY1="};
    bigAD += std::string(17000, 'G');

    std::vector<std::string> data{bigAD};
    AdditionalData ad{data};
    NiceMock<MockDataInterface> dataIface;

    PEL pel{regEntry, 42,   timestamp, phosphor::logging::Entry::Level::Error,
            ad,       ffdc, dataIface};

    EXPECT_TRUE(pel.valid());
    EXPECT_EQ(pel.size(), 16384);

    // Make sure that there are still 2 UD sections.
    size_t udCount = 0;
    for (const auto& section : pel.optionalSections())
    {
        if (section->header().id == static_cast<uint16_t>(SectionID::userData))
        {
            udCount++;
        }
    }

    EXPECT_EQ(udCount, 2); // AD section and sysInfo section
}

// Test that we'll create Generic optional sections for sections that
// there aren't explicit classes for.
TEST_F(PELTest, GenericSectionTest)
{
    auto data = pelDataFactory(TestPELType::pelSimple);

    std::vector<uint8_t> section1{0x58, 0x58, // ID 'XX'
                                  0x00, 0x18, // Size
                                  0x01, 0x02, // version, subtype
                                  0x03, 0x04, // comp ID

                                  // some data
                                  0x20, 0x30, 0x05, 0x09, 0x11, 0x1E, 0x1, 0x63,
                                  0x20, 0x31, 0x06, 0x0F, 0x09, 0x22, 0x3A,
                                  0x00};

    std::vector<uint8_t> section2{
        0x59, 0x59, // ID 'YY'
        0x00, 0x20, // Size
        0x01, 0x02, // version, subtype
        0x03, 0x04, // comp ID

        // some data
        0x20, 0x30, 0x05, 0x09, 0x11, 0x1E, 0x1, 0x63, 0x20, 0x31, 0x06, 0x0F,
        0x09, 0x22, 0x3A, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    // Add the new sections at the end
    data.insert(data.end(), section1.begin(), section1.end());
    data.insert(data.end(), section2.begin(), section2.end());

    // Increment the section count
    data.at(27) += 2;
    auto origData = data;

    PEL pel{data};

    const auto& sections = pel.optionalSections();

    bool foundXX = false;
    bool foundYY = false;

    // Check that we can find these 2 Generic sections
    for (const auto& section : sections)
    {
        if (section->header().id == 0x5858)
        {
            foundXX = true;
            EXPECT_NE(dynamic_cast<Generic*>(section.get()), nullptr);
        }
        else if (section->header().id == 0x5959)
        {
            foundYY = true;
            EXPECT_NE(dynamic_cast<Generic*>(section.get()), nullptr);
        }
    }

    EXPECT_TRUE(foundXX);
    EXPECT_TRUE(foundYY);

    // Now flatten and check
    auto newData = pel.data();

    EXPECT_EQ(origData, newData);
}

// Test that an invalid section will still get a Generic object
TEST_F(PELTest, InvalidGenericTest)
{
    auto data = pelDataFactory(TestPELType::pelSimple);

    // Not a valid section
    std::vector<uint8_t> section1{0x01, 0x02, 0x03};

    data.insert(data.end(), section1.begin(), section1.end());

    // Increment the section count
    data.at(27) += 1;

    PEL pel{data};
    EXPECT_FALSE(pel.valid());

    const auto& sections = pel.optionalSections();

    bool foundGeneric = false;
    for (const auto& section : sections)
    {
        if (dynamic_cast<Generic*>(section.get()) != nullptr)
        {
            foundGeneric = true;
            EXPECT_EQ(section->valid(), false);
            break;
        }
    }

    EXPECT_TRUE(foundGeneric);
}

// Create a UserData section out of AdditionalData
TEST_F(PELTest, MakeUDSectionTest)
{
    std::vector<std::string> ad{"KEY1=VALUE1", "KEY2=VALUE2", "KEY3=VALUE3",
                                "ESEL=TEST"};
    AdditionalData additionalData{ad};

    auto ud = util::makeADUserDataSection(additionalData);

    EXPECT_TRUE(ud->valid());
    EXPECT_EQ(ud->header().id, 0x5544);
    EXPECT_EQ(ud->header().version, 0x01);
    EXPECT_EQ(ud->header().subType, 0x01);
    EXPECT_EQ(ud->header().componentID, 0x2000);

    const auto& d = ud->data();

    std::string jsonString{d.begin(), d.end()};

    std::string expectedJSON =
        R"({"KEY1":"VALUE1","KEY2":"VALUE2","KEY3":"VALUE3"})";

    // The actual data is null padded to a 4B boundary.
    std::vector<uint8_t> expectedData;
    expectedData.resize(52, '\0');
    memcpy(expectedData.data(), expectedJSON.data(), expectedJSON.size());

    EXPECT_EQ(d, expectedData);

    // Ensure we can read this as JSON
    auto newJSON = nlohmann::json::parse(jsonString);
    EXPECT_EQ(newJSON["KEY1"], "VALUE1");
    EXPECT_EQ(newJSON["KEY2"], "VALUE2");
    EXPECT_EQ(newJSON["KEY3"], "VALUE3");
}

// Create the UserData section that contains system info
TEST_F(PELTest, SysInfoSectionTest)
{
    MockDataInterface dataIface;

    EXPECT_CALL(dataIface, getBMCFWVersionID()).WillOnce(Return("ABCD1234"));
    EXPECT_CALL(dataIface, getBMCState()).WillOnce(Return("State.Ready"));
    EXPECT_CALL(dataIface, getChassisState()).WillOnce(Return("State.On"));
    EXPECT_CALL(dataIface, getHostState()).WillOnce(Return("State.Off"));

    std::string pid = "_PID=" + std::to_string(getpid());
    std::vector<std::string> ad{pid};
    AdditionalData additionalData{ad};

    auto ud = util::makeSysInfoUserDataSection(additionalData, dataIface);

    EXPECT_TRUE(ud->valid());
    EXPECT_EQ(ud->header().id, 0x5544);
    EXPECT_EQ(ud->header().version, 0x01);
    EXPECT_EQ(ud->header().subType, 0x01);
    EXPECT_EQ(ud->header().componentID, 0x2000);

    // Pull out the JSON data and check it.
    const auto& d = ud->data();
    std::string jsonString{d.begin(), d.end()};
    auto json = nlohmann::json::parse(jsonString);

    // Ensure the 'Process Name' entry contains 'pel_test'
    auto name = json["Process Name"].get<std::string>();
    EXPECT_NE(name.find("pel_test"), std::string::npos);

    auto version = json["BMC Version ID"].get<std::string>();
    EXPECT_EQ(version, "ABCD1234");

    auto state = json["BMCState"].get<std::string>();
    EXPECT_EQ(state, "Ready");

    state = json["ChassisState"].get<std::string>();
    EXPECT_EQ(state, "On");

    state = json["HostState"].get<std::string>();
    EXPECT_EQ(state, "Off");
}

// Test that the sections that override
//     virtual std::optional<std::string> Section::getJSON() const
// return valid JSON.
TEST_F(PELTest, SectionJSONTest)
{
    auto data = pelDataFactory(TestPELType::pelSimple);
    PEL pel{data};

    // Check that all JSON returned from the sections is
    // parseable by nlohmann::json, which will throw an
    // exception and fail the test if there is a problem.

    // The getJSON() response needs to be wrapped in a { } to make
    // actual valid JSON (PEL::toJSON() usually handles that).

    auto jsonString = pel.privateHeader().getJSON();

    // PrivateHeader always prints JSON
    ASSERT_TRUE(jsonString);
    *jsonString = '{' + *jsonString + '}';
    auto json = nlohmann::json::parse(*jsonString);

    jsonString = pel.userHeader().getJSON();

    // UserHeader always prints JSON
    ASSERT_TRUE(jsonString);
    *jsonString = '{' + *jsonString + '}';
    json = nlohmann::json::parse(*jsonString);

    for (const auto& section : pel.optionalSections())
    {
        // The optional sections may or may not have implemented getJSON().
        jsonString = section->getJSON();
        if (jsonString)
        {
            *jsonString = '{' + *jsonString + '}';
            auto json = nlohmann::json::parse(*jsonString);
        }
    }
}

PelFFDCfile getJSONFFDC(const fs::path& dir)
{
    PelFFDCfile ffdc;
    ffdc.format = UserDataFormat::json;
    ffdc.subType = 5;
    ffdc.version = 42;

    auto inputJSON = R"({
        "key1": "value1",
        "key2": 42,
        "key3" : [1, 2, 3, 4, 5],
        "key4": {"key5": "value5"}
    })"_json;

    // Write the JSON to a file and get its descriptor.
    auto s = inputJSON.dump();
    std::vector<uint8_t> data{s.begin(), s.end()};
    ffdc.fd = writeFileAndGetFD(dir, data);

    return ffdc;
}

TEST_F(PELTest, MakeJSONFileUDSectionTest)
{
    auto dir = makeTempDir();

    {
        auto ffdc = getJSONFFDC(dir);

        auto ud = util::makeFFDCuserDataSection(0x2002, ffdc);
        close(ffdc.fd);
        ASSERT_TRUE(ud);
        ASSERT_TRUE(ud->valid());
        EXPECT_EQ(ud->header().id, 0x5544);

        EXPECT_EQ(ud->header().version,
                  static_cast<uint8_t>(UserDataFormatVersion::json));
        EXPECT_EQ(ud->header().subType,
                  static_cast<uint8_t>(UserDataFormat::json));
        EXPECT_EQ(ud->header().componentID,
                  static_cast<uint16_t>(ComponentID::phosphorLogging));

        // Pull the JSON back out of the the UserData section
        const auto& d = ud->data();
        std::string js{d.begin(), d.end()};
        auto json = nlohmann::json::parse(js);

        EXPECT_EQ("value1", json["key1"].get<std::string>());
        EXPECT_EQ(42, json["key2"].get<int>());

        std::vector<int> key3Values{1, 2, 3, 4, 5};
        EXPECT_EQ(key3Values, json["key3"].get<std::vector<int>>());

        std::map<std::string, std::string> key4Values{{"key5", "value5"}};
        auto actual = json["key4"].get<std::map<std::string, std::string>>();
        EXPECT_EQ(key4Values, actual);
    }

    {
        // A bad FD
        PelFFDCfile ffdc;
        ffdc.format = UserDataFormat::json;
        ffdc.subType = 5;
        ffdc.version = 42;
        ffdc.fd = 10000;

        // The section shouldn't get made
        auto ud = util::makeFFDCuserDataSection(0x2002, ffdc);
        ASSERT_FALSE(ud);
    }

    fs::remove_all(dir);
}

PelFFDCfile getCBORFFDC(const fs::path& dir)
{
    PelFFDCfile ffdc;
    ffdc.format = UserDataFormat::cbor;
    ffdc.subType = 5;
    ffdc.version = 42;

    auto inputJSON = R"({
        "key1": "value1",
        "key2": 42,
        "key3" : [1, 2, 3, 4, 5],
        "key4": {"key5": "value5"}
    })"_json;

    // Convert the JSON to CBOR and write it to a file
    auto data = nlohmann::json::to_cbor(inputJSON);
    ffdc.fd = writeFileAndGetFD(dir, data);

    return ffdc;
}

TEST_F(PELTest, MakeCBORFileUDSectionTest)
{
    auto dir = makeTempDir();

    auto ffdc = getCBORFFDC(dir);
    auto ud = util::makeFFDCuserDataSection(0x2002, ffdc);
    close(ffdc.fd);
    ASSERT_TRUE(ud);
    ASSERT_TRUE(ud->valid());
    EXPECT_EQ(ud->header().id, 0x5544);

    EXPECT_EQ(ud->header().version,
              static_cast<uint8_t>(UserDataFormatVersion::cbor));
    EXPECT_EQ(ud->header().subType, static_cast<uint8_t>(UserDataFormat::cbor));
    EXPECT_EQ(ud->header().componentID,
              static_cast<uint16_t>(ComponentID::phosphorLogging));

    // Pull the CBOR back out of the PEL section
    // The number of pad bytes to make the section be 4B aligned
    // was added at the end, read it and then remove it and the
    // padding before parsing it.
    auto data = ud->data();
    Stream stream{data};
    stream.offset(data.size() - 4);
    uint32_t pad;
    stream >> pad;

    data.resize(data.size() - 4 - pad);

    auto json = nlohmann::json::from_cbor(data);

    EXPECT_EQ("value1", json["key1"].get<std::string>());
    EXPECT_EQ(42, json["key2"].get<int>());

    std::vector<int> key3Values{1, 2, 3, 4, 5};
    EXPECT_EQ(key3Values, json["key3"].get<std::vector<int>>());

    std::map<std::string, std::string> key4Values{{"key5", "value5"}};
    auto actual = json["key4"].get<std::map<std::string, std::string>>();
    EXPECT_EQ(key4Values, actual);

    fs::remove_all(dir);
}

PelFFDCfile getTextFFDC(const fs::path& dir)
{
    PelFFDCfile ffdc;
    ffdc.format = UserDataFormat::text;
    ffdc.subType = 5;
    ffdc.version = 42;

    std::string text{"this is some text that will be used for FFDC"};
    std::vector<uint8_t> data{text.begin(), text.end()};

    ffdc.fd = writeFileAndGetFD(dir, data);

    return ffdc;
}

TEST_F(PELTest, MakeTextFileUDSectionTest)
{
    auto dir = makeTempDir();

    auto ffdc = getTextFFDC(dir);
    auto ud = util::makeFFDCuserDataSection(0x2002, ffdc);
    close(ffdc.fd);
    ASSERT_TRUE(ud);
    ASSERT_TRUE(ud->valid());
    EXPECT_EQ(ud->header().id, 0x5544);

    EXPECT_EQ(ud->header().version,
              static_cast<uint8_t>(UserDataFormatVersion::text));
    EXPECT_EQ(ud->header().subType, static_cast<uint8_t>(UserDataFormat::text));
    EXPECT_EQ(ud->header().componentID,
              static_cast<uint16_t>(ComponentID::phosphorLogging));

    // Get the text back out
    std::string text{ud->data().begin(), ud->data().end()};
    EXPECT_EQ(text, "this is some text that will be used for FFDC");

    fs::remove_all(dir);
}

PelFFDCfile getCustomFFDC(const fs::path& dir, const std::vector<uint8_t>& data)
{
    PelFFDCfile ffdc;
    ffdc.format = UserDataFormat::custom;
    ffdc.subType = 5;
    ffdc.version = 42;

    ffdc.fd = writeFileAndGetFD(dir, data);

    return ffdc;
}

TEST_F(PELTest, MakeCustomFileUDSectionTest)
{
    auto dir = makeTempDir();

    {
        std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8};

        auto ffdc = getCustomFFDC(dir, data);
        auto ud = util::makeFFDCuserDataSection(0x2002, ffdc);
        close(ffdc.fd);
        ASSERT_TRUE(ud);
        ASSERT_TRUE(ud->valid());
        EXPECT_EQ(ud->header().size, 8 + 8); // data size + header size
        EXPECT_EQ(ud->header().id, 0x5544);

        EXPECT_EQ(ud->header().version, 42);
        EXPECT_EQ(ud->header().subType, 5);
        EXPECT_EQ(ud->header().componentID, 0x2002);

        // Get the data back out
        std::vector<uint8_t> newData{ud->data().begin(), ud->data().end()};
        EXPECT_EQ(data, newData);
    }

    // Do the same thing again, but make it be non 4B aligned
    // so the data gets padded.
    {
        std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8, 9};

        auto ffdc = getCustomFFDC(dir, data);
        auto ud = util::makeFFDCuserDataSection(0x2002, ffdc);
        close(ffdc.fd);
        ASSERT_TRUE(ud);
        ASSERT_TRUE(ud->valid());
        EXPECT_EQ(ud->header().size, 12 + 8); // data size + header size
        EXPECT_EQ(ud->header().id, 0x5544);

        EXPECT_EQ(ud->header().version, 42);
        EXPECT_EQ(ud->header().subType, 5);
        EXPECT_EQ(ud->header().componentID, 0x2002);

        // Get the data back out
        std::vector<uint8_t> newData{ud->data().begin(), ud->data().end()};

        // pad the original to 12B so we can compare
        data.push_back(0);
        data.push_back(0);
        data.push_back(0);

        EXPECT_EQ(data, newData);
    }

    fs::remove_all(dir);
}

// Test Adding FFDC from files to a PEL
TEST_F(PELTest, CreateWithFFDCTest)
{
    auto dir = makeTempDir();
    message::Entry regEntry;
    uint64_t timestamp = 5;

    regEntry.name = "test";
    regEntry.subsystem = 5;
    regEntry.actionFlags = 0xC000;
    regEntry.src.type = 0xBD;
    regEntry.src.reasonCode = 0x1234;

    std::vector<std::string> additionalData{"KEY1=VALUE1"};
    AdditionalData ad{additionalData};
    NiceMock<MockDataInterface> dataIface;
    PelFFDC ffdc;

    std::vector<uint8_t> customData{1, 2, 3, 4, 5, 6, 7, 8};

    // This will be trimmed when added
    std::vector<uint8_t> hugeCustomData(17000, 0x42);

    ffdc.emplace_back(std::move(getJSONFFDC(dir)));
    ffdc.emplace_back(std::move(getCBORFFDC(dir)));
    ffdc.emplace_back(std::move(getTextFFDC(dir)));
    ffdc.emplace_back(std::move(getCustomFFDC(dir, customData)));
    ffdc.emplace_back(std::move(getCustomFFDC(dir, hugeCustomData)));

    PEL pel{regEntry, 42,   timestamp, phosphor::logging::Entry::Level::Error,
            ad,       ffdc, dataIface};

    EXPECT_TRUE(pel.valid());

    // Clipped to the max
    EXPECT_EQ(pel.size(), 16384);

    // Check for the FFDC sections
    size_t udCount = 0;
    Section* ud = nullptr;

    for (const auto& section : pel.optionalSections())
    {
        if (section->header().id == static_cast<uint16_t>(SectionID::userData))
        {
            udCount++;
            ud = section.get();
        }
    }

    EXPECT_EQ(udCount, 7); // AD section, sysInfo, 5 ffdc sections

    // Check the last section was trimmed to
    // something a bit less that 17000.
    EXPECT_GT(ud->header().size, 14000);
    EXPECT_LT(ud->header().size, 16000);

    fs::remove_all(dir);
}

// Create a PEL with device callouts
TEST_F(PELTest, CreateWithDevCalloutsTest)
{
    message::Entry regEntry;
    uint64_t timestamp = 5;

    regEntry.name = "test";
    regEntry.subsystem = 5;
    regEntry.actionFlags = 0xC000;
    regEntry.src.type = 0xBD;
    regEntry.src.reasonCode = 0x1234;

    NiceMock<MockDataInterface> dataIface;
    PelFFDC ffdc;

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
                        "LocationCode": "P1",
                        "Priority": "H"
                        }
                    ],
                    "Dest": "proc 0 target"
                }
            }
        }
    })";

    std::vector<std::string> names{"systemA"};
    EXPECT_CALL(dataIface, getSystemNames)
        .Times(2)
        .WillRepeatedly(ReturnRef(names));

    EXPECT_CALL(dataIface,
                getLocationCode(
                    "/xyz/openbmc_project/inventory/chassis/motherboard/cpu0"))
        .WillOnce(Return("UXXX-P1"));

    EXPECT_CALL(dataIface, getInventoryFromLocCode("P1", 0))
        .WillOnce(
            Return("/xyz/openbmc_project/inventory/chassis/motherboard/cpu0"));

    EXPECT_CALL(
        dataIface,
        getHWCalloutFields(
            "/xyz/openbmc_project/inventory/chassis/motherboard/cpu0", _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>("1234567"), SetArgReferee<2>("CCCC"),
                        SetArgReferee<3>("123456789ABC")));

    auto dataPath = getPELReadOnlyDataPath();
    std::ofstream file{dataPath / "systemA_dev_callouts.json"};
    file << calloutJSON;
    file.close();

    {
        std::vector<std::string> data{
            "CALLOUT_ERRNO=5",
            "CALLOUT_DEVICE_PATH=/sys/devices/platform/ahb/ahb:apb/"
            "ahb:apb:bus@1e78a000/1e78a340.i2c-bus/i2c-14/14-0072"};

        AdditionalData ad{data};

        PEL pel{
            regEntry, 42,   timestamp, phosphor::logging::Entry::Level::Error,
            ad,       ffdc, dataIface};

        ASSERT_TRUE(pel.primarySRC().value()->callouts());
        auto& callouts = pel.primarySRC().value()->callouts()->callouts();
        ASSERT_EQ(callouts.size(), 1);

        EXPECT_EQ(callouts[0]->priority(), 'H');
        EXPECT_EQ(callouts[0]->locationCode(), "UXXX-P1");

        auto& fru = callouts[0]->fruIdentity();
        EXPECT_EQ(fru->getPN().value(), "1234567");
        EXPECT_EQ(fru->getCCIN().value(), "CCCC");
        EXPECT_EQ(fru->getSN().value(), "123456789ABC");

        const auto& section = pel.optionalSections().back();

        ASSERT_EQ(section->header().id, 0x5544); // UD
        auto ud = static_cast<UserData*>(section.get());

        // Check that there was a UserData section added that
        // contains debug details about the device.
        const auto& d = ud->data();
        std::string jsonString{d.begin(), d.end()};
        auto actualJSON = nlohmann::json::parse(jsonString);

        auto expectedJSON = R"(
            {
                "PEL Internal Debug Data": {
                    "SRC": [
                      "I2C: bus: 14 address: 114 dest: proc 0 target"
                    ]
                }
            }
        )"_json;

        EXPECT_EQ(actualJSON, expectedJSON);
    }

    {
        // Device path not found (wrong i2c addr), so no callouts
        std::vector<std::string> data{
            "CALLOUT_ERRNO=5",
            "CALLOUT_DEVICE_PATH=/sys/devices/platform/ahb/ahb:apb/"
            "ahb:apb:bus@1e78a000/1e78a340.i2c-bus/i2c-14/14-0099"};

        AdditionalData ad{data};

        PEL pel{
            regEntry, 42,   timestamp, phosphor::logging::Entry::Level::Error,
            ad,       ffdc, dataIface};

        // no callouts
        EXPECT_FALSE(pel.primarySRC().value()->callouts());

        // Now check that there was a UserData section
        // that contains the lookup error.
        const auto& section = pel.optionalSections().back();

        ASSERT_EQ(section->header().id, 0x5544); // UD
        auto ud = static_cast<UserData*>(section.get());

        const auto& d = ud->data();

        std::string jsonString{d.begin(), d.end()};

        auto actualJSON = nlohmann::json::parse(jsonString);

        auto expectedJSON =
            "{\"PEL Internal Debug Data\":{\"SRC\":"
            "[\"Problem looking up I2C callouts on 14 153: "
            "[json.exception.out_of_range.403] key '153' not found\"]}}"_json;

        EXPECT_EQ(actualJSON, expectedJSON);
    }

    fs::remove_all(dataPath);
}
