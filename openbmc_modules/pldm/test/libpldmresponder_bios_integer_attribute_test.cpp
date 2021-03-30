#include "libpldmresponder/bios_integer_attribute.hpp"
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

class TestBIOSIntegerAttribute : public ::testing::Test
{
  public:
    const auto& getIntegerInfo(const BIOSIntegerAttribute& attribute)
    {
        return attribute.integerInfo;
    }
};

TEST_F(TestBIOSIntegerAttribute, CtorTest)
{
    auto jsonIntegerReadOnly = R"({
         "attribute_name" : "SBE_IMAGE_MINIMUM_VALID_ECS",
         "lower_bound" : 1,
         "upper_bound" : 15,
         "scalar_increment" : 1,
         "default_value" : 2
      })"_json;

    BIOSIntegerAttribute integerReadOnly{jsonIntegerReadOnly, nullptr};
    EXPECT_EQ(integerReadOnly.name, "SBE_IMAGE_MINIMUM_VALID_ECS");
    EXPECT_TRUE(integerReadOnly.readOnly);
    auto& integerInfo = getIntegerInfo(integerReadOnly);
    EXPECT_EQ(integerInfo.lowerBound, 1);
    EXPECT_EQ(integerInfo.upperBound, 15);
    EXPECT_EQ(integerInfo.scalarIncrement, 1);
    EXPECT_EQ(integerInfo.defaultValue, 2);

    auto jsonIntegerReadOnlyError = R"({
         "attribute_name" : "SBE_IMAGE_MINIMUM_VALID_ECS",
         "lower_bound" : 1,
         "upper_bound" : 15,
         "scalar_increment" : 1,
         "default_valu" : 2
      })"_json; // default_valu -> default_value
    EXPECT_THROW((BIOSIntegerAttribute{jsonIntegerReadOnlyError, nullptr}),
                 Json::exception);

    auto jsonIntegerReadWrite = R"({
         "attribute_name" : "VDD_AVSBUS_RAIL",
         "lower_bound" : 0,
         "upper_bound" : 15,
         "scalar_increment" : 1,
         "default_value" : 0,
         "dbus":{
            "object_path" : "/xyz/openbmc_project/avsbus",
            "interface" : "xyz.openbmc.AvsBus.Manager",
            "property_type" : "uint8_t",
            "property_name" : "Rail"
         }
      })"_json;

    BIOSIntegerAttribute integerReadWrite{jsonIntegerReadWrite, nullptr};
    EXPECT_EQ(integerReadWrite.name, "VDD_AVSBUS_RAIL");
    EXPECT_TRUE(!integerReadWrite.readOnly);
}

TEST_F(TestBIOSIntegerAttribute, ConstructEntry)
{
    MockBIOSStringTable biosStringTable;
    MockdBusHandler dbusHandler;

    auto jsonIntegerReadOnly = R"({
         "attribute_name" : "VDD_AVSBUS_RAIL",
         "lower_bound" : 1,
         "upper_bound" : 15,
         "scalar_increment" : 1,
         "default_value" : 2
      })"_json;

    std::vector<uint8_t> expectedAttrEntry{
        0,    0,                   /* attr handle */
        0x83,                      /* attr type integer read-only*/
        5,    0,                   /* attr name handle */
        1,    0, 0, 0, 0, 0, 0, 0, /* lower bound */
        15,   0, 0, 0, 0, 0, 0, 0, /* upper bound */
        1,    0, 0, 0,             /* scalar increment */
        2,    0, 0, 0, 0, 0, 0, 0, /* defaut value */
    };
    std::vector<uint8_t> expectedAttrValueEntry{
        0,    0,                   /* attr handle */
        0x83,                      /* attr type integer read-only*/
        2,    0, 0, 0, 0, 0, 0, 0, /* current value */
    };

    BIOSIntegerAttribute integerReadOnly{jsonIntegerReadOnly, nullptr};

    ON_CALL(biosStringTable, findHandle(StrEq("VDD_AVSBUS_RAIL")))
        .WillByDefault(Return(5));

    checkConstructEntry(integerReadOnly, biosStringTable, expectedAttrEntry,
                        expectedAttrValueEntry);

    auto jsonIntegerReadWrite = R"({
         "attribute_name" : "VDD_AVSBUS_RAIL",
         "lower_bound" : 1,
         "upper_bound" : 15,
         "scalar_increment" : 1,
         "default_value" : 2,
         "dbus":{
            "object_path" : "/xyz/openbmc_project/avsbus",
            "interface" : "xyz.openbmc.AvsBus.Manager",
            "property_type" : "uint8_t",
            "property_name" : "Rail"
         }
      })"_json;
    BIOSIntegerAttribute integerReadWrite{jsonIntegerReadWrite, &dbusHandler};

    EXPECT_CALL(dbusHandler,
                getDbusPropertyVariant(StrEq("/xyz/openbmc_project/avsbus"),
                                       StrEq("Rail"),
                                       StrEq("xyz.openbmc.AvsBus.Manager")))
        .WillOnce(Throw(std::exception()));

    /* Set expected attr type to read-write */
    expectedAttrEntry[2] = PLDM_BIOS_INTEGER;
    expectedAttrValueEntry[2] = PLDM_BIOS_INTEGER;

    checkConstructEntry(integerReadWrite, biosStringTable, expectedAttrEntry,
                        expectedAttrValueEntry);

    EXPECT_CALL(dbusHandler,
                getDbusPropertyVariant(StrEq("/xyz/openbmc_project/avsbus"),
                                       StrEq("Rail"),
                                       StrEq("xyz.openbmc.AvsBus.Manager")))
        .WillOnce(Return(PropertyValue(uint8_t(7))));

    expectedAttrValueEntry = {
        0, 0,                   /* attr handle */
        3,                      /* attr type integer read-write*/
        7, 0, 0, 0, 0, 0, 0, 0, /* current value */
    };

    checkConstructEntry(integerReadWrite, biosStringTable, expectedAttrEntry,
                        expectedAttrValueEntry);
}

TEST_F(TestBIOSIntegerAttribute, setAttrValueOnDbus)
{
    MockdBusHandler dbusHandler;
    MockBIOSStringTable biosStringTable;

    auto jsonIntegerReadWrite = R"({
         "attribute_name" : "VDD_AVSBUS_RAIL",
         "lower_bound" : 1,
         "upper_bound" : 15,
         "scalar_increment" : 1,
         "default_value" : 2,
         "dbus":{
            "object_path" : "/xyz/openbmc_project/avsbus",
            "interface" : "xyz.openbmc.AvsBus.Manager",
            "property_type" : "uint8_t",
            "property_name" : "Rail"
         }
      })"_json;
    BIOSIntegerAttribute integerReadWrite{jsonIntegerReadWrite, &dbusHandler};
    DBusMapping dbusMapping{"/xyz/openbmc_project/avsbus",
                            "xyz.openbmc.AvsBus.Manager", "Rail", "uint8_t"};
    std::vector<uint8_t> attrValueEntry = {
        0, 0,                   /* attr handle */
        3,                      /* attr type integer read-write*/
        7, 0, 0, 0, 0, 0, 0, 0, /* current value */
    };

    auto entry = reinterpret_cast<pldm_bios_attr_val_table_entry*>(
        attrValueEntry.data());
    EXPECT_CALL(dbusHandler,
                setDbusProperty(dbusMapping, PropertyValue{uint8_t(7)}))
        .Times(1);
    integerReadWrite.setAttrValueOnDbus(entry, nullptr, biosStringTable);
}