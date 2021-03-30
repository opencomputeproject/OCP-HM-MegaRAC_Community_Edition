#include "bios_table.hpp"

#include "bios_table.h"

#include <fstream>

namespace pldm
{

namespace responder
{

namespace bios
{

BIOSTable::BIOSTable(const char* filePath) : filePath(filePath)
{}

bool BIOSTable::isEmpty() const noexcept
{
    bool empty = false;
    try
    {
        empty = fs::is_empty(filePath);
    }
    catch (fs::filesystem_error& e)
    {
        return true;
    }
    return empty;
}

void BIOSTable::store(const Table& table)
{
    std::ofstream stream(filePath.string(), std::ios::out | std::ios::binary);
    stream.write(reinterpret_cast<const char*>(table.data()), table.size());
}

void BIOSTable::load(Response& response) const
{
    auto currSize = response.size();
    auto fileSize = fs::file_size(filePath);
    response.resize(currSize + fileSize);
    std::ifstream stream(filePath.string(), std::ios::in | std::ios::binary);
    stream.read(reinterpret_cast<char*>(response.data() + currSize), fileSize);
}

BIOSStringTable::BIOSStringTable(const Table& stringTable) :
    stringTable(stringTable)
{}

BIOSStringTable::BIOSStringTable(const BIOSTable& biosTable)
{
    biosTable.load(stringTable);
}

std::string BIOSStringTable::findString(uint16_t handle) const
{
    auto stringEntry = pldm_bios_table_string_find_by_handle(
        stringTable.data(), stringTable.size(), handle);
    if (stringEntry == nullptr)
    {
        throw std::invalid_argument("Invalid String Handle");
    }
    return table::string::decodeString(stringEntry);
}

uint16_t BIOSStringTable::findHandle(const std::string& name) const
{
    auto stringEntry = pldm_bios_table_string_find_by_string(
        stringTable.data(), stringTable.size(), name.c_str());
    if (stringEntry == nullptr)
    {
        throw std::invalid_argument("Invalid String Name");
    }

    return table::string::decodeHandle(stringEntry);
}

namespace table
{

void appendPadAndChecksum(Table& table)
{
    auto sizeWithoutPad = table.size();
    auto padAndChecksumSize = pldm_bios_table_pad_checksum_size(sizeWithoutPad);
    table.resize(table.size() + padAndChecksumSize);

    pldm_bios_table_append_pad_checksum(table.data(), table.size(),
                                        sizeWithoutPad);
}

namespace string
{

uint16_t decodeHandle(const pldm_bios_string_table_entry* entry)
{
    return pldm_bios_table_string_entry_decode_handle(entry);
}

std::string decodeString(const pldm_bios_string_table_entry* entry)
{
    auto strLength = pldm_bios_table_string_entry_decode_string_length(entry);
    std::vector<char> buffer(strLength + 1 /* sizeof '\0' */);
    pldm_bios_table_string_entry_decode_string(entry, buffer.data(),
                                               buffer.size());
    return std::string(buffer.data(), buffer.data() + strLength);
}
const pldm_bios_string_table_entry* constructEntry(Table& table,
                                                   const std::string& str)
{
    auto tableSize = table.size();
    auto entryLength = pldm_bios_table_string_entry_encode_length(str.length());
    table.resize(tableSize + entryLength);
    pldm_bios_table_string_entry_encode(table.data() + tableSize, entryLength,
                                        str.c_str(), str.length());
    return reinterpret_cast<pldm_bios_string_table_entry*>(table.data() +
                                                           tableSize);
}

} // namespace string

namespace attribute
{

TableHeader decodeHeader(const pldm_bios_attr_table_entry* entry)
{
    auto attrHandle = pldm_bios_table_attr_entry_decode_attribute_handle(entry);
    auto attrType = pldm_bios_table_attr_entry_decode_attribute_type(entry);
    auto stringHandle = pldm_bios_table_attr_entry_decode_string_handle(entry);
    return {attrHandle, attrType, stringHandle};
}

const pldm_bios_attr_table_entry* findByHandle(const Table& table,
                                               uint16_t handle)
{
    return pldm_bios_table_attr_find_by_handle(table.data(), table.size(),
                                               handle);
}

const pldm_bios_attr_table_entry* findByStringHandle(const Table& table,
                                                     uint16_t handle)
{
    return pldm_bios_table_attr_find_by_string_handle(table.data(),
                                                      table.size(), handle);
}

const pldm_bios_attr_table_entry*
    constructStringEntry(Table& table,
                         pldm_bios_table_attr_entry_string_info* info)
{
    auto entryLength =
        pldm_bios_table_attr_entry_string_encode_length(info->def_length);

    auto tableSize = table.size();
    table.resize(tableSize + entryLength, 0);
    pldm_bios_table_attr_entry_string_encode(table.data() + tableSize,
                                             entryLength, info);
    return reinterpret_cast<pldm_bios_attr_table_entry*>(table.data() +
                                                         tableSize);
}

const pldm_bios_attr_table_entry*
    constructIntegerEntry(Table& table,
                          pldm_bios_table_attr_entry_integer_info* info)
{
    auto entryLength = pldm_bios_table_attr_entry_integer_encode_length();
    auto tableSize = table.size();
    table.resize(tableSize + entryLength, 0);
    pldm_bios_table_attr_entry_integer_encode(table.data() + tableSize,
                                              entryLength, info);
    return reinterpret_cast<pldm_bios_attr_table_entry*>(table.data() +
                                                         tableSize);
}

StringField decodeStringEntry(const pldm_bios_attr_table_entry* entry)
{
    auto strType = pldm_bios_table_attr_entry_string_decode_string_type(entry);
    auto minLength = pldm_bios_table_attr_entry_string_decode_min_length(entry);
    auto maxLength = pldm_bios_table_attr_entry_string_decode_max_length(entry);
    auto defLength =
        pldm_bios_table_attr_entry_string_decode_def_string_length(entry);

    std::vector<char> buffer(defLength + 1);
    pldm_bios_table_attr_entry_string_decode_def_string(entry, buffer.data(),
                                                        buffer.size());
    return {strType, minLength, maxLength, defLength,
            std::string(buffer.data(), buffer.data() + defLength)};
}

IntegerField decodeIntegerEntry(const pldm_bios_attr_table_entry* entry)
{
    uint64_t lower, upper, def;
    uint32_t scalar;

    pldm_bios_table_attr_entry_integer_decode(entry, &lower, &upper, &scalar,
                                              &def);
    return {lower, upper, scalar, def};
}

const pldm_bios_attr_table_entry*
    constructEnumEntry(Table& table, pldm_bios_table_attr_entry_enum_info* info)
{
    auto entryLength = pldm_bios_table_attr_entry_enum_encode_length(
        info->pv_num, info->def_num);

    auto tableSize = table.size();
    table.resize(tableSize + entryLength, 0);
    pldm_bios_table_attr_entry_enum_encode(table.data() + tableSize,
                                           entryLength, info);

    return reinterpret_cast<pldm_bios_attr_table_entry*>(table.data() +
                                                         tableSize);
}

EnumField decodeEnumEntry(const pldm_bios_attr_table_entry* entry)
{
    uint8_t pvNum = pldm_bios_table_attr_entry_enum_decode_pv_num(entry);
    std::vector<uint16_t> pvHdls(pvNum, 0);
    pldm_bios_table_attr_entry_enum_decode_pv_hdls(entry, pvHdls.data(), pvNum);
    auto defNum = pldm_bios_table_attr_entry_enum_decode_def_num(entry);
    std::vector<uint8_t> defIndices(defNum, 0);
    pldm_bios_table_attr_entry_enum_decode_def_indices(entry, defIndices.data(),
                                                       defIndices.size());
    return {pvHdls, defIndices};
}

} // namespace attribute

namespace attribute_value
{

TableHeader decodeHeader(const pldm_bios_attr_val_table_entry* entry)
{
    auto handle =
        pldm_bios_table_attr_value_entry_decode_attribute_handle(entry);
    auto type = pldm_bios_table_attr_value_entry_decode_attribute_type(entry);
    return {handle, type};
}

std::string decodeStringEntry(const pldm_bios_attr_val_table_entry* entry)
{
    variable_field currentString{};
    pldm_bios_table_attr_value_entry_string_decode_string(entry,
                                                          &currentString);
    return std::string(currentString.ptr,
                       currentString.ptr + currentString.length);
}

uint64_t decodeIntegerEntry(const pldm_bios_attr_val_table_entry* entry)
{
    return pldm_bios_table_attr_value_entry_integer_decode_cv(entry);
}

std::vector<uint8_t>
    decodeEnumEntry(const pldm_bios_attr_val_table_entry* entry)
{
    auto number = pldm_bios_table_attr_value_entry_enum_decode_number(entry);
    std::vector<uint8_t> currHdls(number, 0);
    pldm_bios_table_attr_value_entry_enum_decode_handles(entry, currHdls.data(),
                                                         currHdls.size());
    return currHdls;
}

const pldm_bios_attr_val_table_entry*
    constructStringEntry(Table& table, uint16_t attrHandle, uint8_t attrType,
                         const std::string& str)
{
    auto strLen = str.size();
    auto entryLength =
        pldm_bios_table_attr_value_entry_encode_string_length(strLen);
    auto tableSize = table.size();
    table.resize(tableSize + entryLength);
    pldm_bios_table_attr_value_entry_encode_string(
        table.data() + tableSize, entryLength, attrHandle, attrType, strLen,
        str.c_str());
    return reinterpret_cast<pldm_bios_attr_val_table_entry*>(table.data() +
                                                             tableSize);
}

const pldm_bios_attr_val_table_entry* constructIntegerEntry(Table& table,
                                                            uint16_t attrHandle,
                                                            uint8_t attrType,
                                                            uint64_t value)
{
    auto entryLength = pldm_bios_table_attr_value_entry_encode_integer_length();

    auto tableSize = table.size();
    table.resize(tableSize + entryLength);
    pldm_bios_table_attr_value_entry_encode_integer(
        table.data() + tableSize, entryLength, attrHandle, attrType, value);
    return reinterpret_cast<pldm_bios_attr_val_table_entry*>(table.data() +
                                                             tableSize);
}

const pldm_bios_attr_val_table_entry*
    constructEnumEntry(Table& table, uint16_t attrHandle, uint8_t attrType,
                       const std::vector<uint8_t>& handleIndices)
{
    auto entryLength = pldm_bios_table_attr_value_entry_encode_enum_length(
        handleIndices.size());
    auto tableSize = table.size();
    table.resize(tableSize + entryLength);
    pldm_bios_table_attr_value_entry_encode_enum(
        table.data() + tableSize, entryLength, attrHandle, attrType,
        handleIndices.size(), handleIndices.data());
    return reinterpret_cast<pldm_bios_attr_val_table_entry*>(table.data() +
                                                             tableSize);
}

std::optional<Table> updateTable(const Table& table, const void* entry,
                                 size_t size)
{
    // Replace the old attribute with the new attribute, the size of table will
    // change:
    //   sizeof(newTableBuffer) = srcTableSize + sizeof(newAttribute) -
    //                      sizeof(oldAttribute) + pad(4-byte alignment, max =
    //                      3)
    // For simplicity, we use
    //   sizeof(newTableBuffer) = srcTableSize + sizeof(newAttribute) + 3
    size_t destBufferLength = table.size() + size + 3;
    Table destTable(destBufferLength);

    auto rc = pldm_bios_table_attr_value_copy_and_update(
        table.data(), table.size(), destTable.data(), &destBufferLength, entry,
        size);
    if (rc != PLDM_SUCCESS)
    {
        return std::nullopt;
    }
    destTable.resize(destBufferLength);

    return destTable;
}

} // namespace attribute_value

} // namespace table

} // namespace bios
} // namespace responder
} // namespace pldm
