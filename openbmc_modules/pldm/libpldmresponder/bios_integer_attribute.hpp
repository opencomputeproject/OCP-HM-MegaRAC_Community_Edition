#pragma once

#include "bios_attribute.hpp"

class TestBIOSIntegerAttribute;

namespace pldm
{
namespace responder
{
namespace bios
{

/** @class BIOSIntegerAttribute
 *  @brief Associate integer entry(attr table and attribute value table) and
 *         dbus attribute
 */
class BIOSIntegerAttribute : public BIOSAttribute
{
  public:
    friend class ::TestBIOSIntegerAttribute;

    /** @brief Construct a bios integer attribute
     *  @param[in] entry - Json Object
     *  @param[in] dbusHandler - Dbus Handler
     */
    BIOSIntegerAttribute(const Json& entry, DBusHandler* const dbusHandler);

    /** @brief Set Attribute value On Dbus according to the attribute value
     *         entry
     *  @param[in] attrValueEntry - The attribute value entry
     *  @param[in] attrEntry - The attribute entry corresponding to the
     *                         attribute value entry
     *  @param[in] stringTable - The string table
     */
    void
        setAttrValueOnDbus(const pldm_bios_attr_val_table_entry* attrValueEntry,
                           const pldm_bios_attr_table_entry* attrEntry,
                           const BIOSStringTable& stringTable) override;

    /** @brief Construct corresponding entries at the end of the attribute table
     *         and attribute value tables
     *  @param[in] stringTable - The string Table
     *  @param[in,out] attrTable - The attribute table
     *  @param[in,out] attrValueTable - The attribute value table
     */
    void constructEntry(const BIOSStringTable& stringTable, Table& attrTable,
                        Table& attrValueTable) override;

    int updateAttrVal(Table& newValue, uint16_t attrHdl, uint8_t attrType,
                      const PropertyValue& newPropVal);

  private:
    /** @brief Integer field from json */
    table::attribute::IntegerField integerInfo;

    /** @brief Get pldm value from dbus propertyValue */
    uint64_t getAttrValue(const PropertyValue& value);

    /** @brief Get value on dbus */
    uint64_t getAttrValue();
};

} // namespace bios
} // namespace responder
} // namespace pldm
