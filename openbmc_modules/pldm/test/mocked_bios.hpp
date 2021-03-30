#include "libpldmresponder/bios_table.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::ElementsAreArray;
using namespace pldm::responder::bios;

class MockBIOSStringTable : public BIOSStringTable
{
  public:
    MockBIOSStringTable() : BIOSStringTable({})
    {}

    MOCK_METHOD(uint16_t, findHandle, (const std::string&), (const override));

    MOCK_METHOD(std::string, findString, (const uint16_t), (const override));
};

void checkHeader(const Table& attrEntry, const Table& attrValueEntry)
{
    auto attrHeader = table::attribute::decodeHeader(
        reinterpret_cast<const pldm_bios_attr_table_entry*>(attrEntry.data()));
    auto attrValueHeader = table::attribute_value::decodeHeader(
        reinterpret_cast<const pldm_bios_attr_val_table_entry*>(
            attrValueEntry.data()));

    EXPECT_EQ(attrHeader.attrHandle, attrValueHeader.attrHandle);
}

void checkEntry(Table& entry, Table& expectedEntry)
{
    /** backup the attr handle */
    auto attr0 = entry[0], eAttr0 = expectedEntry[0];
    auto attr1 = entry[1], eAttr1 = expectedEntry[1];

    /** attr handle is computed by libpldm, set it to 0 to test */
    entry[0] = 0, expectedEntry[0] = 0;
    entry[1] = 0, expectedEntry[1] = 0;

    EXPECT_THAT(entry, ElementsAreArray(expectedEntry));

    /** restore the attr handle */
    entry[0] = attr0, expectedEntry[0] = eAttr0;
    entry[1] = attr1, expectedEntry[1] = eAttr1;
}

void checkConstructEntry(BIOSAttribute& attribute, BIOSStringTable& stringTable,
                         Table& expectedAttrEntry,
                         Table& expectedAttrValueEntry)
{
    Table attrEntry, attrValueEntry;
    attribute.constructEntry(stringTable, attrEntry, attrValueEntry);

    checkHeader(attrEntry, attrValueEntry);
    checkEntry(attrEntry, expectedAttrEntry);
    checkEntry(attrValueEntry, expectedAttrValueEntry);
}