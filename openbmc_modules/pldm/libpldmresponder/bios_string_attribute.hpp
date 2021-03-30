#pragma once
#include "bios_attribute.hpp"

#include <string>

class TestBIOSStringAttribute;

namespace pldm
{
namespace responder
{
namespace bios
{

/** @class BIOSStringAttribute
 *  @brief Associate string entry(attr table and attribute value table) and dbus
 *         attribute
 */
class BIOSStringAttribute : public BIOSAttribute
{
  public:
    friend class ::TestBIOSStringAttribute;

    /** @brief BIOS string types */
    enum class Encoding : uint8_t
    {
        UNKNOWN = 0x00,
        ASCII = 0x01,
        HEX = 0x02,
        UTF_8 = 0x03,
        UTF_16LE = 0x04,
        UTF_16BE = 0x05,
        VENDOR_SPECIFIC = 0xFF
    };

    /** brief Mapping of string to enum for string type */
    inline static const std::map<std::string, Encoding> strTypeMap{
        {"Unknown", Encoding::UNKNOWN},
        {"ASCII", Encoding::ASCII},
        {"Hex", Encoding::HEX},
        {"UTF-8", Encoding::UTF_8},
        {"UTF-16LE", Encoding::UTF_16LE},
        {"UTF-16LE", Encoding::UTF_16LE},
        {"Vendor Specific", Encoding::VENDOR_SPECIFIC}};

    /** @brief Construct a bios string attribute
     *  @param[in] entry - Json Object
     *  @param[in] dbusHandler - Dbus Handler
     */
    BIOSStringAttribute(const Json& entry, DBusHandler* const dbusHandler);

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
    /** @brief string field from json */
    table::attribute::StringField stringInfo;

    /** @brief Get attribute value on dbus */
    std::string getAttrValue();
};

} // namespace bios
} // namespace responder
} // namespace pldm
