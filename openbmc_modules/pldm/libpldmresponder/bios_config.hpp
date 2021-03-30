#pragma once

#include "bios_table.h"

#include "bios_attribute.hpp"
#include "bios_table.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace pldm
{
namespace responder
{
namespace bios
{

/** @class BIOSConfig
 *  @brief Manager BIOS Attributes
 */
class BIOSConfig
{
  public:
    BIOSConfig() = delete;
    BIOSConfig(const BIOSConfig&) = delete;
    BIOSConfig(BIOSConfig&&) = delete;
    BIOSConfig& operator=(const BIOSConfig&) = delete;
    BIOSConfig& operator=(BIOSConfig&&) = delete;
    ~BIOSConfig() = default;

    /** @brief Construct BIOSConfig
     *  @param[in] jsonDir - The directory where json file exists
     *  @param[in] tableDir - The directory where the persistent table is placed
     *  @param[in] dbusHandler - Dbus Handler
     */
    explicit BIOSConfig(const char* jsonDir, const char* tableDir,
                        DBusHandler* const dbusHandler);

    /** @brief Set attribute value on dbus and attribute value table
     *  @param[in] entry - attribute value entry
     *  @param[in] size - size of the attribute value entry
     *  @return pldm_completion_codes
     */
    int setAttrValue(const void* entry, size_t size);

    /** @brief Remove the persistent tables */
    void removeTables();

    /** @brief Build bios tables(string,attribute,attribute value table)*/
    void buildTables();

    /** @brief Get BIOS table of specified type
     *  @param[in] tableType - The table type
     *  @return The bios table, std::nullopt if the table is unaviliable
     */
    std::optional<Table> getBIOSTable(pldm_bios_table_types tableType);

  private:
    const fs::path jsonDir;
    const fs::path tableDir;
    DBusHandler* const dbusHandler;

    // vector persists all attributes
    using BIOSAttributes = std::vector<std::unique_ptr<BIOSAttribute>>;
    BIOSAttributes biosAttributes;

    using propName = std::string;
    using DbusChObjProperties = std::map<propName, PropertyValue>;

    // vector to catch the D-Bus property change signals for BIOS attributes
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> biosAttrMatch;

    /** @brief Method to update a BIOS attribute when the corresponding Dbus
     *  property is changed
     *  @param[in] chProperties - list of properties which have changed
     *  @param[in] biosAttrIndex - Index of BIOSAttribute pointer in
     * biosAttributes
     *  @return - none
     */
    void processBiosAttrChangeNotification(
        const DbusChObjProperties& chProperties, uint32_t biosAttrIndex);

    /** @brief Construct an attribute and persist it
     *  @tparam T - attribute type
     *  @param[in] entry - json entry
     */
    template <typename T>
    void constructAttribute(const Json& entry)
    {
        try
        {
            biosAttributes.push_back(std::make_unique<T>(entry, dbusHandler));
            auto biosAttrIndex = biosAttributes.size() - 1;
            auto dBusMap = biosAttributes[biosAttrIndex]->getDBusMap();

            if (dBusMap.has_value())
            {
                using namespace sdbusplus::bus::match::rules;
                biosAttrMatch.push_back(
                    std::make_unique<sdbusplus::bus::match::match>(
                        pldm::utils::DBusHandler::getBus(),
                        propertiesChanged(dBusMap->objectPath,
                                          dBusMap->interface),
                        [this,
                         biosAttrIndex](sdbusplus::message::message& msg) {
                            DbusChObjProperties props;
                            std::string iface;
                            msg.read(iface, props);
                            processBiosAttrChangeNotification(props,
                                                              biosAttrIndex);
                        }));
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Constructs Attribute Error, " << e.what()
                      << std::endl;
        }
    }

    /** Construct attributes and persist them */
    void constructAttributes();

    using ParseHandler = std::function<void(const Json& entry)>;

    /** @brief Helper function to parse json
     *  @param[in] filePath - Path of json file
     *  @param[in] handler - Handler to process each entry in the json
     */
    void load(const fs::path& filePath, ParseHandler handler);

    /** @brief Build String Table and persist it
     *  @return The built string table, std::nullopt if it fails.
     */
    std::optional<Table> buildAndStoreStringTable();

    /** @brief Build attr table and attr value table and persist them
     *  @param[in] stringTable - The string Table
     */
    void buildAndStoreAttrTables(const Table& stringTable);

    /** @brief Persist the table
     *  @param[in] path - Path to persist the table
     *  @param[in] table - The table
     */
    void storeTable(const fs::path& path, const Table& table);

    /** @brief Load bios table to ram
     *  @param[in] path - Path of the table
     *  @return The table, std::nullopt if loading fails
     */
    std::optional<Table> loadTable(const fs::path& path);

    /** @brief Check the attribute value to update
     *  @param[in] attrValueEntry - The attribute value entry to update
     *  @param[in] attrEntry - The attribute table entry
     *  @param[in] stringTable - The string  table
     *  @return pldm_completion_codes
     */
    int checkAttrValueToUpdate(
        const pldm_bios_attr_val_table_entry* attrValueEntry,
        const pldm_bios_attr_table_entry* attrEntry, Table& stringTable);
};

} // namespace bios
} // namespace responder
} // namespace pldm
