#pragma once

#include "libpldm/bios.h"
#include "libpldm/bios_table.h"

#include <stdint.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace pldm
{

namespace responder
{

namespace bios
{

using Table = std::vector<uint8_t>;
using Response = std::vector<uint8_t>;
namespace fs = std::filesystem;

/** @class BIOSTable
 *
 *  @brief Provides APIs for storing and loading BIOS tables
 *
 *  Typical usage is as follows:
 *  BIOSTable table(BIOS_STRING_TABLE_FILE_PATH);
 *  if (table.isEmpty()) { // no persisted table
 *    // construct BIOSTable 't'
 *    // prepare Response 'r'
 *    // send response to GetBIOSTable
 *    table.store(t); // persisted
 *  }
 *  else { // persisted table exists
 *    // create Response 'r' which has response fields (except
 *    // the table itself) to a GetBIOSTable command
 *    table.load(r); // actual table will be pushed back to the vector
 *  }
 */
class BIOSTable
{
  public:
    /** @brief Ctor - set file path to persist BIOS table
     *
     *  @param[in] filePath - file where BIOS table should be persisted
     */
    BIOSTable(const char* filePath);

    /** @brief Checks if there's a persisted BIOS table
     *
     *  @return bool - true if table exists, false otherwise
     */
    bool isEmpty() const noexcept;

    /** @brief Persist a BIOS table(string/attribute/attribute value)
     *
     *  @param[in] table - BIOS table
     */
    void store(const Table& table);

    /** @brief Load BIOS table from persistent store to memory
     *
     *  @param[in,out] response - PLDM response message to GetBIOSTable
     *  (excluding table), table will be pushed back to this.
     */
    void load(Response& response) const;

  private:
    // file storing PLDM BIOS table
    fs::path filePath;
};

/** @class BIOSStringTableInterface
 *  @brief Provide interfaces to the BIOS string table operations
 */
class BIOSStringTableInterface
{
  public:
    virtual ~BIOSStringTableInterface() = default;

    /** @brief Find the string name from the BIOS string table for a string
     * handle
     *  @param[in] handle - string handle
     *  @return name of the corresponding BIOS string
     */
    virtual std::string findString(uint16_t handle) const = 0;

    /** @brief Find the string handle from the BIOS string table by the given
     *         name
     *  @param[in] name - name of the BIOS string
     *  @return handle of the string
     */
    virtual uint16_t findHandle(const std::string& name) const = 0;
};

/** @class BIOSStringTable
 *  @brief Collection of BIOS string table operations.
 */
class BIOSStringTable : public BIOSStringTableInterface
{
  public:
    /** @brief Constructs BIOSStringTable
     *
     *  @param[in] stringTable - The stringTable in RAM
     */
    BIOSStringTable(const Table& stringTable);

    /** @brief Constructs BIOSStringTable
     *
     *  @param[in] biosTable - The BIOSTable
     */
    BIOSStringTable(const BIOSTable& biosTable);

    /** @brief Find the string name from the BIOS string table for a string
     * handle
     *  @param[in] handle - string handle
     *  @return name of the corresponding BIOS string
     *  @throw std::invalid_argument if the string can not be found.
     */
    std::string findString(uint16_t handle) const override;

    /** @brief Find the string handle from the BIOS string table by the given
     *         name
     *  @param[in] name - name of the BIOS string
     *  @return handle of the string
     *  @throw std::invalid_argument if the string can not be found
     */
    uint16_t findHandle(const std::string& name) const override;

  private:
    Table stringTable;
};

namespace table
{

/** @brief Append Pad and Checksum
 *
 *  @param[in,out] table - table to be appended with pad and checksum
 */
void appendPadAndChecksum(Table& table);

namespace string
{

/** @brief Get the string handle for the entry
 *  @param[in] entry - Pointer to a bios string table entry
 *  @return Handle to identify a string in the bios string table
 */
uint16_t decodeHandle(const pldm_bios_string_table_entry* entry);

/** @brief Get the string from the entry
 *  @param[in] entry - Pointer to a bios string table entry
 *  @return The String
 */
std::string decodeString(const pldm_bios_string_table_entry* entry);

/** @brief construct entry of string table at the end of the given
 *         table
 *  @param[in,out] table - The given table
 *  @param[in] str - string itself
 *  @return pointer to the constructed entry
 */
const pldm_bios_string_table_entry* constructEntry(Table& table,
                                                   const std::string& str);

} // namespace string

namespace attribute
{

/** @struct TableHeader
 *  @brief Header of attribute table
 */
struct TableHeader
{
    uint16_t attrHandle;
    uint8_t attrType;
    uint16_t stringHandle;
};

/** @brief Decode header of attribute table entry
 *  @param[in] entry - Pointer to an attribute table entry
 *  @return Attribute table header
 */
TableHeader decodeHeader(const pldm_bios_attr_table_entry* entry);

/** @brief Find attribute entry by handle
 *  @param[in] table - attribute table
 *  @param[in] handle - attribute handle
 *  @return Pointer to the attribute table entry
 */
const pldm_bios_attr_table_entry* findByHandle(const Table& table,
                                               uint16_t handle);

/** @brief Find attribute entry by string handle
 *  @param[in] table - attribute table
 *  @param[in] handle - string handle
 *  @return Pointer to the attribute table entry
 */
const pldm_bios_attr_table_entry* findByStringHandle(const Table& table,
                                                     uint16_t handle);

/** @struct StringField
 *  @brief String field of attribute table
 */
struct StringField
{
    uint8_t stringType;
    uint16_t minLength;
    uint16_t maxLength;
    uint16_t defLength;
    std::string defString;
};

/** @brief decode string entry of attribute table
 *  @param[in] entry - Pointer to an attribute table entry
 *  @return String field of the entry
 */
StringField decodeStringEntry(const pldm_bios_attr_table_entry* entry);

/** @brief construct string entry of attribute table at the end of the given
 *         table
 *  @param[in,out] table - The given table
 *  @param[in] info - string info
 *  @return pointer to the constructed entry
 */
const pldm_bios_attr_table_entry*
    constructStringEntry(Table& table,
                         pldm_bios_table_attr_entry_string_info* info);

/** @struct IntegerField
 *  @brief Integer field of attribute table
 */
struct IntegerField
{
    uint64_t lowerBound;
    uint64_t upperBound;
    uint32_t scalarIncrement;
    uint64_t defaultValue;
};

/** @brief construct integer entry of attribute table at the end of the
 *         given table
 *  @param[in,out] table - The given table
 *  @param[in] info - integer info
 *  @return pointer to the constructed entry
 */
const pldm_bios_attr_table_entry*
    constructIntegerEntry(Table& table,
                          pldm_bios_table_attr_entry_integer_info* info);

/** @brief decode integer entry of attribute table
 *  @param[in] entry - Pointer to an attribute table entry
 *  @return Integer field of the entry
 */
IntegerField decodeIntegerEntry(const pldm_bios_attr_table_entry* entry);

/** @struct EnumField
 *  @brief Enum field of attribute table
 */
struct EnumField
{
    std::vector<uint16_t> possibleValueStringHandle;
    std::vector<uint8_t> defaultValueIndex;
};

/** @brief decode enum entry of attribute table
 *  @param[in] entry - Pointer to an attribute table entry
 *  @return Enum field of the entry
 */
EnumField decodeEnumEntry(const pldm_bios_attr_table_entry* entry);

/** @brief construct enum entry of attribute table at the end of the
 *         given table
 *  @param[in,out] table - The given table
 *  @param[in] info - enum info
 *  @return pointer to the constructed entry
 */
const pldm_bios_attr_table_entry*
    constructEnumEntry(Table& table,
                       pldm_bios_table_attr_entry_enum_info* info);

} // namespace attribute

namespace attribute_value
{

/** @struct TableHeader
 *  @brief Header of attribute value table
 */
struct TableHeader
{
    uint16_t attrHandle;
    uint8_t attrType;
};

/** @brief Decode header of attribute value table
 *  @param[in] entry - Pointer to an attribute value table entry
 *  @return Attribute value table header
 */
TableHeader decodeHeader(const pldm_bios_attr_val_table_entry* entry);

/** @brief Decode string entry of attribute value table
 *  @param[in] entry - Pointer to an attribute value table entry
 *  @return The decoded string
 */
std::string decodeStringEntry(const pldm_bios_attr_val_table_entry* entry);

/** @brief Decode integer entry of attribute value table
 *  @param[in] entry - Pointer to an attribute value table entry
 *  @return The decoded integer
 */
uint64_t decodeIntegerEntry(const pldm_bios_attr_val_table_entry* entry);

/** @brief Decode enum entry of attribute value table
 *  @param[in] entry - Pointer to an attribute value table entry
 *  @return Current value string handle indices
 */
std::vector<uint8_t>
    decodeEnumEntry(const pldm_bios_attr_val_table_entry* entry);

/** @brief Construct string entry of attribute value table at the end of the
 *         given table
 *  @param[in] table - The given table
 *  @param[in] attrHandle - attribute handle
 *  @param[in] attrType - attribute type
 *  @param[in] str - The string
 *  @return Pointer to the constructed entry
 */
const pldm_bios_attr_val_table_entry*
    constructStringEntry(Table& table, uint16_t attrHandle, uint8_t attrType,
                         const std::string& str);

/** @brief Construct integer entry of attribute value table at the end of
 *         the given table
 *  @param[in] table - The given table
 *  @param[in] attrHandle - attribute handle
 *  @param[in] attrType - attribute type
 *  @param[in] value - The integer
 *  @return Pointer to the constructed entry
 */
const pldm_bios_attr_val_table_entry* constructIntegerEntry(Table& table,
                                                            uint16_t attrHandle,
                                                            uint8_t attrType,
                                                            uint64_t value);

/** @brief Construct enum entry of attribute value table at the end of
 *         the given table
 *  @param[in] table - The given table
 *  @param[in] attrHandle - attribute handle
 *  @param[in] attrType - attribute type
 *  @param[in] handleIndices -  handle indices
 *  @return Pointer to the constructed entry
 */
const pldm_bios_attr_val_table_entry*
    constructEnumEntry(Table& table, uint16_t attrHandle, uint8_t attrType,
                       const std::vector<uint8_t>& handleIndices);

/** @brief construct a table with an new entry
 *  @param[in] table - the table need to be updated
 *  @param[in] entry - the new attribute value entry
 *  @param[in] size - size of the new entry
 *  @return newly constructed table, std::nullopt if failed
 */
std::optional<Table> updateTable(const Table& table, const void* entry,
                                 size_t size);

} // namespace attribute_value

} // namespace table

} // namespace bios
} // namespace responder
} // namespace pldm
