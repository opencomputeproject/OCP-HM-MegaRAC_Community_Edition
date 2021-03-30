#include "libpldmresponder/bios_string_attribute.hpp"
#include "mocked_bios.hpp"
#include "mocked_utils.hpp"

#include <nlohmann/json.hpp>

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace pldm::responder::bios;
using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::Throw;

class TestBIOSStringAttribute : public ::testing::Test
{
  public:
    const auto& getStringInfo(const BIOSStringAttribute& biosStringAttribute)
    {
        return biosStringAttribute.stringInfo;
    }
};

TEST_F(TestBIOSStringAttribute, CtorTest)
{
    auto jsonStringReadOnly = R"(  {
            "attribute_name" : "str_example3",
            "string_type" : "ASCII",
            "minimum_string_length" : 1,
            "maximum_string_length" : 100,
            "default_string_length" : 2,
            "default_string" : "ef"
        })"_json;
    BIOSStringAttribute stringReadOnly{jsonStringReadOnly, nullptr};
    EXPECT_EQ(stringReadOnly.name, "str_example3");
    EXPECT_TRUE(stringReadOnly.readOnly);

    auto& stringInfo = getStringInfo(stringReadOnly);
    EXPECT_EQ(stringInfo.stringType,
              static_cast<uint8_t>(BIOSStringAttribute::Encoding::ASCII));
    EXPECT_EQ(stringInfo.minLength, 1);
    EXPECT_EQ(stringInfo.maxLength, 100);
    EXPECT_EQ(stringInfo.defLength, 2);
    EXPECT_EQ(stringInfo.defString, "ef");

    auto jsonStringReadOnlyError = R"(  {
            "attribute_name" : "str_example3",
            "string_type" : "ASCII",
            "minimum_string_length" : 1,
            "maximum_string_length" : 100,
            "default_string" : "ef"
        })"_json; // missing default_string_length

    EXPECT_THROW((BIOSStringAttribute{jsonStringReadOnlyError, nullptr}),
                 Json::exception);

    auto jsonStringReadWrite = R"({
            "attribute_name" : "str_example1",
            "string_type" : "ASCII",
            "minimum_string_length" : 1,
            "maximum_string_length" : 100,
            "default_string_length" : 3,
            "default_string" : "abc",
            "dbus" : {
                "object_path" : "/xyz/abc/def",
                "interface" : "xyz.openbmc_project.str_example1.value",
                "property_name" : "Str_example1",
                "property_type" : "string"
            }
        })"_json;
    BIOSStringAttribute stringReadWrite{jsonStringReadWrite, nullptr};

    EXPECT_EQ(stringReadWrite.name, "str_example1");
    EXPECT_TRUE(!stringReadWrite.readOnly);
}

TEST_F(TestBIOSStringAttribute, ConstructEntry)
{
    MockBIOSStringTable biosStringTable;
    MockdBusHandler dbusHandler;

    auto jsonStringReadOnly = R"({
            "attribute_name" : "str_example1",
            "string_type" : "ASCII",
            "minimum_string_length" : 1,
            "maximum_string_length" : 100,
            "default_string_length" : 3,
            "default_string" : "abc"
        })"_json;

    std::vector<uint8_t> expectedAttrEntry{
        0,    0,       /* attr handle */
        0x81,          /* attr type string read-only */
        5,    0,       /* attr name handle */
        1,             /* string type */
        1,    0,       /* minimum length of the string in bytes */
        100,  0,       /* maximum length of the string in bytes */
        3,    0,       /* length of default string in length */
        'a',  'b', 'c' /* default string  */
    };

    std::vector<uint8_t> expectedAttrValueEntry{
        0,    0,        /* attr handle */
        0x81,           /* attr type string read-only */
        3,    0,        /* current string length */
        'a',  'b', 'c', /* defaut value string handle index */
    };

    ON_CALL(biosStringTable, findHandle(StrEq("str_example1")))
        .WillByDefault(Return(5));
    BIOSStringAttribute stringReadOnly{jsonStringReadOnly, nullptr};

    checkConstructEntry(stringReadOnly, biosStringTable, expectedAttrEntry,
                        expectedAttrValueEntry);

    auto jsonStringReadWrite = R"({
            "attribute_name" : "str_example1",
            "string_type" : "ASCII",
            "minimum_string_length" : 1,
            "maximum_string_length" : 100,
            "default_string_length" : 3,
            "default_string" : "abc",
            "dbus" : {
                "object_path" : "/xyz/abc/def",
                "interface" : "xyz.openbmc_project.str_example1.value",
                "property_name" : "Str_example1",
                "property_type" : "string"
            }
        })"_json;
    BIOSStringAttribute stringReadWrite{jsonStringReadWrite, &dbusHandler};

    /* Set expected attr type to read-write */
    expectedAttrEntry[2] = PLDM_BIOS_STRING;
    expectedAttrValueEntry[2] = PLDM_BIOS_STRING;

    EXPECT_CALL(
        dbusHandler,
        getDbusPropertyVariant(StrEq("/xyz/abc/def"), StrEq("Str_example1"),
                               StrEq("xyz.openbmc_project.str_example1.value")))
        .WillOnce(Throw(std::exception()));

    checkConstructEntry(stringReadWrite, biosStringTable, expectedAttrEntry,
                        expectedAttrValueEntry);

    EXPECT_CALL(
        dbusHandler,
        getDbusPropertyVariant(StrEq("/xyz/abc/def"), StrEq("Str_example1"),
                               StrEq("xyz.openbmc_project.str_example1.value")))
        .WillOnce(Return(PropertyValue(std::string("abcd"))));

    expectedAttrValueEntry = {
        0,   0,             /* attr handle */
        1,                  /* attr type string read-write */
        4,   0,             /* current string length */
        'a', 'b', 'c', 'd', /* defaut value string handle index */
    };

    checkConstructEntry(stringReadWrite, biosStringTable, expectedAttrEntry,
                        expectedAttrValueEntry);
}

TEST_F(TestBIOSStringAttribute, setAttrValueOnDbus)
{
    auto jsonStringReadWrite = R"({
            "attribute_name" : "str_example1",
            "string_type" : "ASCII",
            "minimum_string_length" : 1,
            "maximum_string_length" : 100,
            "default_string_length" : 3,
            "default_string" : "abc",
            "dbus" : {
                "object_path" : "/xyz/abc/def",
                "interface" : "xyz.openbmc_project.str_example1.value",
                "property_name" : "Str_example1",
                "property_type" : "string"
            }
        })"_json;

    MockdBusHandler dbusHandler;
    MockBIOSStringTable biosStringTable;

    BIOSStringAttribute stringReadWrite{jsonStringReadWrite, &dbusHandler};
    DBusMapping dbusMapping{"/xyz/abc/def",
                            "xyz.openbmc_project.str_example1.value",
                            "Str_example1", "string"};
    std::vector<uint8_t> attrValueEntry{
        0,   0,             /* attr handle */
        1,                  /* attr type string read-write */
        4,   0,             /* current string length */
        'a', 'b', 'c', 'd', /* defaut value string handle index */
    };
    auto entry = reinterpret_cast<pldm_bios_attr_val_table_entry*>(
        attrValueEntry.data());
    PropertyValue value = std::string("abcd");
    EXPECT_CALL(dbusHandler, setDbusProperty(dbusMapping, value)).Times(1);
    stringReadWrite.setAttrValueOnDbus(entry, nullptr, biosStringTable);
}
