#include "libpldmresponder/bios_enum_attribute.hpp"
#include "mocked_bios.hpp"
#include "mocked_utils.hpp"

#include <nlohmann/json.hpp>

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::Throw;

class TestBIOSEnumAttribute : public ::testing::Test
{
  public:
    const auto& getPossibleValues(const BIOSEnumAttribute& attribute)
    {
        return attribute.possibleValues;
    }

    const auto& getDefaultValue(const BIOSEnumAttribute& attribute)
    {
        return attribute.defaultValue;
    }
};

TEST_F(TestBIOSEnumAttribute, CtorTest)
{
    auto jsonEnumReadOnly = R"({
         "attribute_name" : "CodeUpdatePolicy",
         "possible_values" : [ "Concurrent", "Disruptive" ],
         "default_values" : [ "Concurrent" ]
      })"_json;

    BIOSEnumAttribute enumReadOnly{jsonEnumReadOnly, nullptr};
    EXPECT_EQ(enumReadOnly.name, "CodeUpdatePolicy");
    EXPECT_TRUE(enumReadOnly.readOnly);
    EXPECT_THAT(getPossibleValues(enumReadOnly),
                ElementsAreArray({"Concurrent", "Disruptive"}));
    EXPECT_EQ(getDefaultValue(enumReadOnly), "Concurrent");

    auto jsonEnumReadOnlyError = R"({
         "attribute_name" : "CodeUpdatePolicy",
         "possible_value" : [ "Concurrent", "Disruptive" ],
         "default_values" : [ "Concurrent" ]
      })"_json; // possible_value -> possible_values
    EXPECT_THROW((BIOSEnumAttribute{jsonEnumReadOnlyError, nullptr}),
                 Json::exception);

    auto jsonEnumReadWrite = R"({
         "attribute_name" : "FWBootSide",
         "possible_values" : [ "Perm", "Temp" ],
         "default_values" : [ "Perm" ],
         "dbus":
            {
               "object_path" : "/xyz/abc/def",
               "interface" : "xyz.openbmc.FWBoot.Side",
               "property_name" : "Side",
               "property_type" : "bool",
               "property_values" : [true, false]
            }
      })"_json;

    BIOSEnumAttribute enumReadWrite{jsonEnumReadWrite, nullptr};
    EXPECT_EQ(enumReadWrite.name, "FWBootSide");
    EXPECT_TRUE(!enumReadWrite.readOnly);
}

TEST_F(TestBIOSEnumAttribute, ConstructEntry)
{
    MockBIOSStringTable biosStringTable;
    MockdBusHandler dbusHandler;

    auto jsonEnumReadOnly = R"({
         "attribute_name" : "CodeUpdatePolicy",
         "possible_values" : [ "Concurrent", "Disruptive" ],
         "default_values" : [ "Disruptive" ]
      })"_json;

    std::vector<uint8_t> expectedAttrEntry{
        0,    0, /* attr handle */
        0x80,    /* attr type enum read-only*/
        4,    0, /* attr name handle */
        2,       /* number of possible value */
        2,    0, /* possible value handle */
        3,    0, /* possible value handle */
        1,       /* number of default value */
        1        /* defaut value string handle index */
    };

    std::vector<uint8_t> expectedAttrValueEntry{
        0, 0, /* attr handle */
        0x80, /* attr type enum read-only*/
        1,    /* number of current value */
        1     /* current value string handle index */
    };

    BIOSEnumAttribute enumReadOnly{jsonEnumReadOnly, nullptr};

    ON_CALL(biosStringTable, findHandle(StrEq("Concurrent")))
        .WillByDefault(Return(2));
    ON_CALL(biosStringTable, findHandle(StrEq("Disruptive")))
        .WillByDefault(Return(3));
    ON_CALL(biosStringTable, findHandle(StrEq("CodeUpdatePolicy")))
        .WillByDefault(Return(4));

    checkConstructEntry(enumReadOnly, biosStringTable, expectedAttrEntry,
                        expectedAttrValueEntry);

    auto jsonEnumReadWrite = R"({
         "attribute_name" : "CodeUpdatePolicy",
         "possible_values" : [ "Concurrent", "Disruptive" ],
         "default_values" : [ "Disruptive" ],
         "dbus":
            {
               "object_path" : "/xyz/abc/def",
               "interface" : "xyz.openbmc.abc.def",
               "property_name" : "Policy",
               "property_type" : "bool",
               "property_values" : [true, false]
          }
      })"_json;

    BIOSEnumAttribute enumReadWrite{jsonEnumReadWrite, &dbusHandler};

    EXPECT_CALL(dbusHandler,
                getDbusPropertyVariant(StrEq("/xyz/abc/def"), StrEq("Policy"),
                                       StrEq("xyz.openbmc.abc.def")))
        .WillOnce(Throw(std::exception()));

    /* Set expected attr type to read-write */
    expectedAttrEntry[2] = PLDM_BIOS_ENUMERATION;
    expectedAttrValueEntry[2] = PLDM_BIOS_ENUMERATION;

    checkConstructEntry(enumReadWrite, biosStringTable, expectedAttrEntry,
                        expectedAttrValueEntry);

    EXPECT_CALL(dbusHandler,
                getDbusPropertyVariant(StrEq("/xyz/abc/def"), StrEq("Policy"),
                                       StrEq("xyz.openbmc.abc.def")))
        .WillOnce(Return(PropertyValue(true)));

    expectedAttrValueEntry = {
        0, 0, /* attr handle */
        0,    /* attr type enum read-write*/
        1,    /* number of current value */
        0     /* current value string handle index */
    };

    checkConstructEntry(enumReadWrite, biosStringTable, expectedAttrEntry,
                        expectedAttrValueEntry);
}

TEST_F(TestBIOSEnumAttribute, setAttrValueOnDbus)
{
    MockBIOSStringTable biosStringTable;
    MockdBusHandler dbusHandler;

    auto jsonEnumReadWrite = R"({
         "attribute_name" : "CodeUpdatePolicy",
         "possible_values" : [ "Concurrent", "Disruptive" ],
         "default_values" : [ "Disruptive" ],
         "dbus":
            {
               "object_path" : "/xyz/abc/def",
               "interface" : "xyz.openbmc.abc.def",
               "property_name" : "Policy",
               "property_type" : "bool",
               "property_values" : [true, false]
          }
      })"_json;
    DBusMapping dbusMapping{"/xyz/abc/def", "xyz.openbmc.abc.def", "Policy",
                            "bool"};

    BIOSEnumAttribute enumReadWrite{jsonEnumReadWrite, &dbusHandler};

    std::vector<uint8_t> attrEntry{
        0, 0, /* attr handle */
        0,    /* attr type enum read-only*/
        4, 0, /* attr name handle */
        2,    /* number of possible value */
        2, 0, /* possible value handle */
        3, 0, /* possible value handle */
        1,    /* number of default value */
        1     /* defaut value string handle index */
    };

    ON_CALL(biosStringTable, findString(2))
        .WillByDefault(Return(std::string("Concurrent")));
    ON_CALL(biosStringTable, findString(3))
        .WillByDefault(Return(std::string("Disruptive")));

    std::vector<uint8_t> attrValueEntry{
        0, 0, /* attr handle */
        0,    /* attr type enum read-only*/
        1,    /* number of current value */
        0     /* current value string handle index */
    };

    EXPECT_CALL(dbusHandler,
                setDbusProperty(dbusMapping, PropertyValue{bool(true)}))
        .Times(1);
    enumReadWrite.setAttrValueOnDbus(
        reinterpret_cast<pldm_bios_attr_val_table_entry*>(
            attrValueEntry.data()),
        reinterpret_cast<pldm_bios_attr_table_entry*>(attrEntry.data()),
        biosStringTable);
}