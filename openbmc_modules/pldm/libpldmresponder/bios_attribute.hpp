#pragma once

#include "bios_table.h"

#include "bios_table.hpp"
#include "common/utils.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pldm
{
namespace responder
{
namespace bios
{

using Json = nlohmann::json;
using namespace pldm::utils;

/** @class BIOSAttribute
 *  @brief Provide interfaces to implement specific types of attributes
 */
class BIOSAttribute
{
  public:
    /** @brief Construct a bios attribute
     *  @param[in] entry - Json Object
     *  @param[in] dbusHandler - Dbus Handler
     */
    BIOSAttribute(const Json& entry, DBusHandler* const dbusHandler);

    /** Virtual destructor
     */
    virtual ~BIOSAttribute() = default;

    /** @brief Set Attribute value On Dbus according to the attribute value
     * entry
     *  @param[in] attrValueEntry - The attribute value entry
     *  @param[in] attrEntry - The attribute entry corresponding to the
     *                         attribute value entry
     *  @param[in] stringTable - The string table
     */
    virtual void
        setAttrValueOnDbus(const pldm_bios_attr_val_table_entry* attrValueEntry,
                           const pldm_bios_attr_table_entry* attrEntry,
                           const BIOSStringTable& stringTable) = 0;

    /** @brief Construct corresponding entries at the end of the attribute table
     *         and attribute value tables
     *  @param[in] stringTable - The string Table
     *  @param[in,out] attrTable - The attribute table
     *  @param[in,out] attrValueTable - The attribute value table
     */
    virtual void constructEntry(const BIOSStringTable& stringTable,
                                Table& attrTable, Table& attrValueTable) = 0;

    /** @brief Method to update the value for an attribute
     *  @param[in,out] newValue - An attribute value table row with the new
     * value for the attribute
     *  @param[in] attrHdl - attribute handle
     *  @param[in] attrType - attribute type
     *  @param[in] newPropVal - The new value
     *  @return PLDM Success or failure status
     */
    virtual int updateAttrVal(Table& newValue, uint16_t attrHdl,
                              uint8_t attrType,
                              const PropertyValue& newPropVal) = 0;

    /** @brief Method to return the D-Bus map */
    std::optional<DBusMapping> getDBusMap();

    /** @brief Name of this attribute */
    const std::string name;

    /** Weather this attribute is read-only */
    const bool readOnly;

  protected:
    /** @brief dbus backend, nullopt if this attribute is read-only*/
    std::optional<DBusMapping> dBusMap;

    /** @brief dbus handler */
    DBusHandler* const dbusHandler;
};

} // namespace bios
} // namespace responder
} // namespace pldm
