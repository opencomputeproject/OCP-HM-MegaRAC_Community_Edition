#pragma once
#include "ipmi_fru_info_area.hpp"

#include <sdbusplus/bus.hpp>
#include <string>

namespace ipmi
{
namespace fru
{
using FRUId = uint8_t;
using FRUAreaMap = std::map<FRUId, FruAreaData>;

static constexpr auto xyzPrefix = "/xyz/openbmc_project/";
static constexpr auto invMgrInterface = "xyz.openbmc_project.Inventory.Manager";
static constexpr auto invObjPath = "/xyz/openbmc_project/inventory";
static constexpr auto propInterface = "org.freedesktop.DBus.Properties";
static constexpr auto invItemInterface = "xyz.openbmc_project.Inventory.Item";
static constexpr auto itemPresentProp = "Present";

/**
 * @brief Get fru area data as per IPMI specification
 *
 * @param[in] fruNum FRU ID
 *
 * @return FRU area data as per IPMI specification
 */
const FruAreaData& getFruAreaData(const FRUId& fruNum);

/**
 * @brief Register callback handler into DBUS for PropertyChange events
 *
 * @return negative value on failure
 */
int registerCallbackHandler();
} // namespace fru
} // namespace ipmi
