#include "bios_config.hpp"

#include "bios_enum_attribute.hpp"
#include "bios_integer_attribute.hpp"
#include "bios_string_attribute.hpp"
#include "bios_table.hpp"

#include <fstream>
#include <iostream>

namespace pldm
{
namespace responder
{
namespace bios
{
namespace
{

constexpr auto enumJsonFile = "enum_attrs.json";
constexpr auto stringJsonFile = "string_attrs.json";
constexpr auto integerJsonFile = "integer_attrs.json";

constexpr auto stringTableFile = "stringTable";
constexpr auto attrTableFile = "attributeTable";
constexpr auto attrValueTableFile = "attributeValueTable";

} // namespace

BIOSConfig::BIOSConfig(const char* jsonDir, const char* tableDir,
                       DBusHandler* const dbusHandler) :
    jsonDir(jsonDir),
    tableDir(tableDir), dbusHandler(dbusHandler)
{
    fs::create_directories(tableDir);
    constructAttributes();
}

void BIOSConfig::buildTables()
{
    auto stringTable = buildAndStoreStringTable();
    if (stringTable)
    {
        buildAndStoreAttrTables(*stringTable);
    }
}

std::optional<Table> BIOSConfig::getBIOSTable(pldm_bios_table_types tableType)
{
    fs::path tablePath;
    switch (tableType)
    {
        case PLDM_BIOS_STRING_TABLE:
            tablePath = tableDir / stringTableFile;
            break;
        case PLDM_BIOS_ATTR_TABLE:
            tablePath = tableDir / attrTableFile;
            break;
        case PLDM_BIOS_ATTR_VAL_TABLE:
            tablePath = tableDir / attrValueTableFile;
            break;
    }
    return loadTable(tablePath);
}

void BIOSConfig::constructAttributes()
{
    load(jsonDir / stringJsonFile, [this](const Json& entry) {
        constructAttribute<BIOSStringAttribute>(entry);
    });
    load(jsonDir / integerJsonFile, [this](const Json& entry) {
        constructAttribute<BIOSIntegerAttribute>(entry);
    });
    load(jsonDir / enumJsonFile, [this](const Json& entry) {
        constructAttribute<BIOSEnumAttribute>(entry);
    });
}

void BIOSConfig::buildAndStoreAttrTables(const Table& stringTable)
{
    BIOSStringTable biosStringTable(stringTable);

    if (biosAttributes.empty())
    {
        return;
    }

    Table attrTable, attrValueTable;

    for (auto& attr : biosAttributes)
    {
        try
        {
            attr->constructEntry(biosStringTable, attrTable, attrValueTable);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Construct Table Entry Error, AttributeName = "
                      << attr->name << std::endl;
        }
    }

    table::appendPadAndChecksum(attrTable);
    table::appendPadAndChecksum(attrValueTable);

    storeTable(tableDir / attrTableFile, attrTable);
    storeTable(tableDir / attrValueTableFile, attrValueTable);
}

std::optional<Table> BIOSConfig::buildAndStoreStringTable()
{
    std::set<std::string> strings;
    auto handler = [&strings](const Json& entry) {
        strings.emplace(entry.at("attribute_name"));
    };

    load(jsonDir / stringJsonFile, handler);
    load(jsonDir / integerJsonFile, handler);
    load(jsonDir / enumJsonFile, [&strings](const Json& entry) {
        strings.emplace(entry.at("attribute_name"));
        auto possibleValues = entry.at("possible_values");
        for (auto& pv : possibleValues)
        {
            strings.emplace(pv);
        }
    });

    if (strings.empty())
    {
        return std::nullopt;
    }

    Table table;
    for (const auto& elem : strings)
    {
        table::string::constructEntry(table, elem);
    }

    table::appendPadAndChecksum(table);
    storeTable(tableDir / stringTableFile, table);
    return table;
}

void BIOSConfig::storeTable(const fs::path& path, const Table& table)
{
    BIOSTable biosTable(path.c_str());
    biosTable.store(table);
}

std::optional<Table> BIOSConfig::loadTable(const fs::path& path)
{
    BIOSTable biosTable(path.c_str());
    if (biosTable.isEmpty())
    {
        return std::nullopt;
    }

    Table table;
    biosTable.load(table);
    return table;
}

void BIOSConfig::load(const fs::path& filePath, ParseHandler handler)
{
    std::ifstream file;
    Json jsonConf;
    if (fs::exists(filePath))
    {
        try
        {
            file.open(filePath);
            jsonConf = Json::parse(file);
            auto entries = jsonConf.at("entries");
            for (auto& entry : entries)
            {
                try
                {
                    handler(entry);
                }
                catch (const std::exception& e)
                {
                    std::cerr
                        << "Failed to parse JSON config file(entry handler) : "
                        << filePath.c_str() << ", " << e.what() << std::endl;
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to parse JSON config file : "
                      << filePath.c_str() << std::endl;
        }
    }
}

int BIOSConfig::checkAttrValueToUpdate(
    const pldm_bios_attr_val_table_entry* attrValueEntry,
    const pldm_bios_attr_table_entry* attrEntry, Table&)

{
    auto [attrHandle, attrType] =
        table::attribute_value::decodeHeader(attrValueEntry);

    switch (attrType)
    {
        case PLDM_BIOS_ENUMERATION:
        {
            auto value =
                table::attribute_value::decodeEnumEntry(attrValueEntry);
            auto [pvHdls, defIndex] =
                table::attribute::decodeEnumEntry(attrEntry);
            assert(value.size() == 1);
            if (value[0] >= pvHdls.size())
            {
                std::cerr << "Enum: Illgeal index, Index = " << (int)value[0]
                          << std::endl;
                return PLDM_ERROR_INVALID_DATA;
            }

            return PLDM_SUCCESS;
        }
        case PLDM_BIOS_INTEGER:
        {
            auto value =
                table::attribute_value::decodeIntegerEntry(attrValueEntry);
            auto [lower, upper, scalar, def] =
                table::attribute::decodeIntegerEntry(attrEntry);

            if (value < lower || value > upper)
            {
                std::cerr << "Integer: out of bound, value = " << value
                          << std::endl;
                return PLDM_ERROR_INVALID_DATA;
            }
            return PLDM_SUCCESS;
        }
        case PLDM_BIOS_STRING:
        {
            auto stringConf = table::attribute::decodeStringEntry(attrEntry);
            auto value =
                table::attribute_value::decodeStringEntry(attrValueEntry);
            if (value.size() < stringConf.minLength ||
                value.size() > stringConf.maxLength)
            {
                std::cerr << "String: Length error, string = " << value
                          << " length = " << value.size() << std::endl;
                return PLDM_ERROR_INVALID_LENGTH;
            }
            return PLDM_SUCCESS;
        }
        default:
            std::cerr << "ReadOnly or Unspported type, type = " << attrType
                      << std::endl;
            return PLDM_ERROR;
    };
}

int BIOSConfig::setAttrValue(const void* entry, size_t size)
{
    auto attrValueTable = getBIOSTable(PLDM_BIOS_ATTR_VAL_TABLE);
    auto attrTable = getBIOSTable(PLDM_BIOS_ATTR_TABLE);
    auto stringTable = getBIOSTable(PLDM_BIOS_STRING_TABLE);
    if (!attrValueTable || !attrTable || !stringTable)
    {
        return PLDM_BIOS_TABLE_UNAVAILABLE;
    }

    auto attrValueEntry =
        reinterpret_cast<const pldm_bios_attr_val_table_entry*>(entry);

    auto attrValHeader = table::attribute_value::decodeHeader(attrValueEntry);

    auto attrEntry =
        table::attribute::findByHandle(*attrTable, attrValHeader.attrHandle);
    if (!attrEntry)
    {
        return PLDM_ERROR;
    }

    auto rc = checkAttrValueToUpdate(attrValueEntry, attrEntry, *stringTable);
    if (rc != PLDM_SUCCESS)
    {
        return rc;
    }

    auto destTable =
        table::attribute_value::updateTable(*attrValueTable, entry, size);

    if (!destTable)
    {
        return PLDM_ERROR;
    }

    try
    {
        auto attrHeader = table::attribute::decodeHeader(attrEntry);

        BIOSStringTable biosStringTable(*stringTable);
        auto attrName = biosStringTable.findString(attrHeader.stringHandle);

        auto iter = std::find_if(
            biosAttributes.begin(), biosAttributes.end(),
            [&attrName](const auto& attr) { return attr->name == attrName; });

        if (iter == biosAttributes.end())
        {
            return PLDM_ERROR;
        }
        (*iter)->setAttrValueOnDbus(attrValueEntry, attrEntry, biosStringTable);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Set attribute value error: " << e.what() << std::endl;
        return PLDM_ERROR;
    }

    BIOSTable biosAttrValueTable((tableDir / attrValueTableFile).c_str());
    biosAttrValueTable.store(*destTable);
    return PLDM_SUCCESS;
}

void BIOSConfig::removeTables()
{
    try
    {
        fs::remove(tableDir / stringTableFile);
        fs::remove(tableDir / attrTableFile);
        fs::remove(tableDir / attrValueTableFile);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Remove the tables error: " << e.what() << std::endl;
    }
}

void BIOSConfig::processBiosAttrChangeNotification(
    const DbusChObjProperties& chProperties, uint32_t biosAttrIndex)
{
    const auto& dBusMap = biosAttributes[biosAttrIndex]->getDBusMap();
    const auto& propertyName = dBusMap->propertyName;
    const auto& attrName = biosAttributes[biosAttrIndex]->name;

    const auto it = chProperties.find(propertyName);
    if (it == chProperties.end())
    {
        return;
    }

    PropertyValue newPropVal = it->second;
    auto stringTable = getBIOSTable(PLDM_BIOS_STRING_TABLE);
    if (!stringTable.has_value())
    {
        std::cerr << "BIOS string table unavailable\n";
        return;
    }
    BIOSStringTable biosStringTable(*stringTable);
    uint16_t attrNameHdl{};
    try
    {
        attrNameHdl = biosStringTable.findHandle(attrName);
    }
    catch (std::invalid_argument& e)
    {
        std::cerr << "Could not find handle for BIOS string, ATTRIBUTE="
                  << attrName.c_str() << "\n";
        return;
    }

    auto attrTable = getBIOSTable(PLDM_BIOS_ATTR_TABLE);
    if (!attrTable.has_value())
    {
        std::cerr << "Attribute table not present\n";
        return;
    }
    const struct pldm_bios_attr_table_entry* tableEntry =
        table::attribute::findByStringHandle(*attrTable, attrNameHdl);
    if (tableEntry == nullptr)
    {
        std::cerr << "Attribute not found in attribute table, name= "
                  << attrName.c_str() << "name handle=" << attrNameHdl << "\n";
        return;
    }

    auto [attrHdl, attrType, stringHdl] =
        table::attribute::decodeHeader(tableEntry);

    auto attrValueSrcTable = getBIOSTable(PLDM_BIOS_ATTR_VAL_TABLE);

    if (!attrValueSrcTable.has_value())
    {
        std::cerr << "Attribute value table not present\n";
        return;
    }

    Table newValue;
    auto rc = biosAttributes[biosAttrIndex]->updateAttrVal(
        newValue, attrHdl, attrType, newPropVal);
    if (rc != PLDM_SUCCESS)
    {
        std::cerr << "Could not update the attribute value table for attribute "
                     "handle="
                  << attrHdl << " and type=" << (uint32_t)attrType << "\n";
        return;
    }
    auto destTable = table::attribute_value::updateTable(
        *attrValueSrcTable, newValue.data(), newValue.size());
    if (destTable.has_value())
    {
        storeTable(tableDir / attrValueTableFile, *destTable);
    }
}

} // namespace bios
} // namespace responder
} // namespace pldm
