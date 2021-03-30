#pragma once

#include <map>
#include <string>
#include <vector>

namespace ipmi
{
namespace fru
{
using FruAreaData = std::vector<uint8_t>;
using Section = std::string;
using Value = std::string;
using Property = std::string;
using PropertyMap = std::map<Property, Value>;
using FruInventoryData = std::map<Section, PropertyMap>;

/**
 * @brief Builds Fru area data from inventory data
 *
 * @param[in] invData FRU properties values read from inventory
 *
 * @return FruAreaData FRU area data as per IPMI specification
 */
FruAreaData buildFruAreaData(const FruInventoryData& inventory);

} // namespace fru
} // namespace ipmi
