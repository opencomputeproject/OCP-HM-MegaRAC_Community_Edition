#include "config.h"

#include "bios_enum_attribute.hpp"

#include "common/utils.hpp"

#include <iostream>

namespace pldm
{
namespace responder
{
namespace bios
{

BIOSEnumAttribute::BIOSEnumAttribute(const Json& entry,
                                     DBusHandler* const dbusHandler) :
    BIOSAttribute(entry, dbusHandler)
{
    std::string attrName = entry.at("attribute_name");
    Json pv = entry.at("possible_values");
    for (auto& val : pv)
    {
        possibleValues.emplace_back(val);
    }

    std::vector<std::string> defaultValues;
    Json dv = entry.at("default_values");
    for (auto& val : dv)
    {
        defaultValues.emplace_back(val);
    }
    assert(defaultValues.size() == 1);
    defaultValue = defaultValues[0];
    if (!readOnly)
    {
        auto dbusValues = entry.at("dbus").at("property_values");
        buildValMap(dbusValues);
    }
}

uint8_t BIOSEnumAttribute::getValueIndex(const std::string& value,
                                         const std::vector<std::string>& pVs)
{
    auto iter = std::find_if(pVs.begin(), pVs.end(),
                             [&value](const auto& v) { return v == value; });
    if (iter == pVs.end())
    {
        throw std::invalid_argument("value must be one of possible value");
    }
    return iter - pVs.begin();
}

std::vector<uint16_t> BIOSEnumAttribute::getPossibleValuesHandle(
    const BIOSStringTable& stringTable, const std::vector<std::string>& pVs)
{
    std::vector<uint16_t> possibleValuesHandle;
    for (const auto& pv : pVs)
    {
        auto handle = stringTable.findHandle(pv);
        possibleValuesHandle.push_back(handle);
    }

    return possibleValuesHandle;
}

void BIOSEnumAttribute::buildValMap(const Json& dbusVals)
{
    PropertyValue value;
    size_t pos = 0;
    for (auto it = dbusVals.begin(); it != dbusVals.end(); ++it, ++pos)
    {
        if (dBusMap->propertyType == "uint8_t")
        {
            value = static_cast<uint8_t>(it.value());
        }
        else if (dBusMap->propertyType == "uint16_t")
        {
            value = static_cast<uint16_t>(it.value());
        }
        else if (dBusMap->propertyType == "uint32_t")
        {
            value = static_cast<uint32_t>(it.value());
        }
        else if (dBusMap->propertyType == "uint64_t")
        {
            value = static_cast<uint64_t>(it.value());
        }
        else if (dBusMap->propertyType == "int16_t")
        {
            value = static_cast<int16_t>(it.value());
        }
        else if (dBusMap->propertyType == "int32_t")
        {
            value = static_cast<int32_t>(it.value());
        }
        else if (dBusMap->propertyType == "int64_t")
        {
            value = static_cast<int64_t>(it.value());
        }
        else if (dBusMap->propertyType == "bool")
        {
            value = static_cast<bool>(it.value());
        }
        else if (dBusMap->propertyType == "double")
        {
            value = static_cast<double>(it.value());
        }
        else if (dBusMap->propertyType == "string")
        {
            value = static_cast<std::string>(it.value());
        }
        else
        {
            std::cerr << "Unknown D-Bus property type, TYPE="
                      << dBusMap->propertyType << "\n";
            throw std::invalid_argument("Unknown D-BUS property type");
        }
        valMap.emplace(value, possibleValues[pos]);
    }
}

uint8_t BIOSEnumAttribute::getAttrValueIndex()
{
    auto defaultValueIndex = getValueIndex(defaultValue, possibleValues);
    if (readOnly)
    {
        return defaultValueIndex;
    }

    try
    {
        auto propValue = dbusHandler->getDbusPropertyVariant(
            dBusMap->objectPath.c_str(), dBusMap->propertyName.c_str(),
            dBusMap->interface.c_str());
        auto iter = valMap.find(propValue);
        if (iter == valMap.end())
        {
            return defaultValueIndex;
        }
        auto currentValue = iter->second;
        return getValueIndex(currentValue, possibleValues);
    }
    catch (const std::exception& e)
    {
        return defaultValueIndex;
    }
}

void BIOSEnumAttribute::setAttrValueOnDbus(
    const pldm_bios_attr_val_table_entry* attrValueEntry,
    const pldm_bios_attr_table_entry* attrEntry,
    const BIOSStringTable& stringTable)
{
    if (readOnly)
    {
        return;
    }
    auto [pvHdls, _] = table::attribute::decodeEnumEntry(attrEntry);
    auto currHdls = table::attribute_value::decodeEnumEntry(attrValueEntry);

    assert(currHdls.size() == 1);

    auto valueString = stringTable.findString(pvHdls[currHdls[0]]);

    auto it = std::find_if(valMap.begin(), valMap.end(),
                           [&valueString](const auto& typePair) {
                               return typePair.second == valueString;
                           });
    if (it == valMap.end())
    {
        return;
    }

    dbusHandler->setDbusProperty(*dBusMap, it->first);
}

void BIOSEnumAttribute::constructEntry(const BIOSStringTable& stringTable,
                                       Table& attrTable, Table& attrValueTable)
{
    auto possibleValuesHandle =
        getPossibleValuesHandle(stringTable, possibleValues);
    std::vector<uint8_t> defaultIndices(1, 0);
    defaultIndices[0] = getValueIndex(defaultValue, possibleValues);

    pldm_bios_table_attr_entry_enum_info info = {
        stringTable.findHandle(name),         readOnly,
        (uint8_t)possibleValuesHandle.size(), possibleValuesHandle.data(),
        (uint8_t)defaultIndices.size(),       defaultIndices.data(),
    };

    auto attrTableEntry =
        table::attribute::constructEnumEntry(attrTable, &info);
    auto [attrHandle, attrType, _] =
        table::attribute::decodeHeader(attrTableEntry);

    std::vector<uint8_t> currValueIndices(1, 0);
    currValueIndices[0] = getAttrValueIndex();

    table::attribute_value::constructEnumEntry(attrValueTable, attrHandle,
                                               attrType, currValueIndices);
}

int BIOSEnumAttribute::updateAttrVal(Table& newValue, uint16_t attrHdl,
                                     uint8_t attrType,
                                     const PropertyValue& newPropVal)
{
    auto iter = valMap.find(newPropVal);
    if (iter == valMap.end())
    {
        std::cerr << "Could not find index for new BIOS enum, value="
                  << std::get<std::string>(newPropVal) << "\n";
        return PLDM_ERROR;
    }
    auto currentValue = iter->second;
    std::vector<uint8_t> handleIndices{
        getValueIndex(currentValue, possibleValues)};
    table::attribute_value::constructEnumEntry(newValue, attrHdl, attrType,
                                               handleIndices);
    return PLDM_SUCCESS;
}

} // namespace bios
} // namespace responder
} // namespace pldm
