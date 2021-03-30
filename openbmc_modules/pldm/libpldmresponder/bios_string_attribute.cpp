#include "bios_string_attribute.hpp"

#include "common/utils.hpp"

#include <iostream>
#include <tuple>
#include <variant>

namespace pldm
{
namespace responder
{
namespace bios
{

BIOSStringAttribute::BIOSStringAttribute(const Json& entry,
                                         DBusHandler* const dbusHandler) :
    BIOSAttribute(entry, dbusHandler)
{
    std::string strTypeTmp = entry.at("string_type");
    auto iter = strTypeMap.find(strTypeTmp);
    if (iter == strTypeMap.end())
    {
        std::cerr << "Wrong string type, STRING_TYPE=" << strTypeTmp
                  << " ATTRIBUTE_NAME=" << name << "\n";
        throw std::invalid_argument("Wrong string type");
    }
    stringInfo.stringType = static_cast<uint8_t>(iter->second);

    stringInfo.minLength = entry.at("minimum_string_length");
    stringInfo.maxLength = entry.at("maximum_string_length");
    stringInfo.defLength = entry.at("default_string_length");
    stringInfo.defString = entry.at("default_string");

    pldm_bios_table_attr_entry_string_info info = {
        0,
        readOnly,
        stringInfo.stringType,
        stringInfo.minLength,
        stringInfo.maxLength,
        stringInfo.defLength,
        stringInfo.defString.data(),
    };

    const char* errmsg;
    auto rc = pldm_bios_table_attr_entry_string_info_check(&info, &errmsg);
    if (rc != PLDM_SUCCESS)
    {
        std::cerr << "Wrong field for string attribute, ATTRIBUTE_NAME=" << name
                  << " ERRMSG=" << errmsg
                  << " MINIMUM_STRING_LENGTH=" << stringInfo.minLength
                  << " MAXIMUM_STRING_LENGTH=" << stringInfo.maxLength
                  << " DEFAULT_STRING_LENGTH=" << stringInfo.defLength
                  << " DEFAULT_STRING=" << stringInfo.defString << "\n";
        throw std::invalid_argument("Wrong field for string attribute");
    }
}

void BIOSStringAttribute::setAttrValueOnDbus(
    const pldm_bios_attr_val_table_entry* attrValueEntry,
    const pldm_bios_attr_table_entry*, const BIOSStringTable&)
{
    if (readOnly)
    {
        return;
    }

    PropertyValue value =
        table::attribute_value::decodeStringEntry(attrValueEntry);
    dbusHandler->setDbusProperty(*dBusMap, value);
}

std::string BIOSStringAttribute::getAttrValue()
{
    if (readOnly)
    {
        return stringInfo.defString;
    }
    try
    {
        return dbusHandler->getDbusProperty<std::string>(
            dBusMap->objectPath.c_str(), dBusMap->propertyName.c_str(),
            dBusMap->interface.c_str());
    }
    catch (const std::exception& e)
    {
        std::cerr << "Get String Attribute Value Error: AttributeName = "
                  << name << std::endl;
        return stringInfo.defString;
    }
}

void BIOSStringAttribute::constructEntry(const BIOSStringTable& stringTable,
                                         Table& attrTable,
                                         Table& attrValueTable)
{
    pldm_bios_table_attr_entry_string_info info = {
        stringTable.findHandle(name), readOnly,
        stringInfo.stringType,        stringInfo.minLength,
        stringInfo.maxLength,         stringInfo.defLength,
        stringInfo.defString.data(),
    };

    auto attrTableEntry =
        table::attribute::constructStringEntry(attrTable, &info);
    auto [attrHandle, attrType, _] =
        table::attribute::decodeHeader(attrTableEntry);
    auto currStr = getAttrValue();
    table::attribute_value::constructStringEntry(attrValueTable, attrHandle,
                                                 attrType, currStr);
}

int BIOSStringAttribute::updateAttrVal(Table& newValue, uint16_t attrHdl,
                                       uint8_t attrType,
                                       const PropertyValue& newPropVal)
{
    try
    {
        const auto& newStringValue = std::get<std::string>(newPropVal);
        table::attribute_value::constructStringEntry(newValue, attrHdl,
                                                     attrType, newStringValue);
    }
    catch (const std::bad_variant_access& e)
    {
        std::cerr << "invalid value passed for the property, error: "
                  << e.what() << "\n";
        return PLDM_ERROR;
    }
    return PLDM_SUCCESS;
}

} // namespace bios
} // namespace responder
} // namespace pldm
