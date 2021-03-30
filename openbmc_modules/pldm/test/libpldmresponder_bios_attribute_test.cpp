#include "libpldmresponder/bios_attribute.hpp"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

using namespace pldm::responder::bios;

class TestAttribute : public BIOSAttribute
{
  public:
    TestAttribute(const Json& entry, DBusHandler* const dbusHandler) :
        BIOSAttribute(entry, dbusHandler)
    {}

    void setAttrValueOnDbus(const pldm_bios_attr_val_table_entry*,
                            const pldm_bios_attr_table_entry*,
                            const BIOSStringTable&) override
    {}

    void constructEntry(const BIOSStringTable&, Table&, Table&) override
    {}

    const std::optional<DBusMapping>& getDbusMap()
    {
        return dBusMap;
    }

    int updateAttrVal(Table& /*newValue*/, uint16_t /*attrHdl*/,
                      uint8_t /*attrType*/,
                      const PropertyValue& /*newPropVal*/) override
    {
        return PLDM_SUCCESS;
    }
};

TEST(BIOSAttribute, CtorTest)
{
    auto jsonReadOnly = R"({
      "attribute_name" : "ReadOnly"
    })"_json;

    TestAttribute readOnly{jsonReadOnly, nullptr};
    EXPECT_EQ(readOnly.name, "ReadOnly");
    EXPECT_EQ(readOnly.readOnly, true);

    auto jsonReadOnlyError = R"({
      "attribute_nam":"ReadOnly"
    })"_json;
    using Json = nlohmann::json;

    EXPECT_THROW((TestAttribute{jsonReadOnlyError, nullptr}), Json::exception);

    auto jsonReadWrite = R"({
      "attribute_name":"ReadWrite",
      "dbus":
           {
               "object_path" : "/xyz/abc/def",
               "interface" : "xyz.openbmc.FWBoot.Side",
               "property_name" : "Side",
               "property_type" : "bool"
           }
    })"_json;

    TestAttribute readWrite{jsonReadWrite, nullptr};
    EXPECT_EQ(readWrite.name, "ReadWrite");
    EXPECT_EQ(readWrite.readOnly, false);
    auto dbusMap = readWrite.getDbusMap();
    EXPECT_NE(dbusMap, std::nullopt);
    EXPECT_EQ(dbusMap->objectPath, "/xyz/abc/def");
    EXPECT_EQ(dbusMap->interface, "xyz.openbmc.FWBoot.Side");
    EXPECT_EQ(dbusMap->propertyName, "Side");
    EXPECT_EQ(dbusMap->propertyType, "bool");

    auto jsonReadWriteError = R"({
      "attribute_name":"ReadWrite",
      "dbus":
           {
               "object_path" : "/xyz/abc/def",
               "interface" : "xyz.openbmc.FWBoot.Side",
               "property_name" : "Side"
           }
    })"_json; // missing property_type.

    EXPECT_THROW((TestAttribute{jsonReadWriteError, nullptr}), Json::exception);
}
