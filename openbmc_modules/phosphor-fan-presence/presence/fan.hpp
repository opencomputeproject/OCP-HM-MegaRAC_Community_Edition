#pragma once

#include <string>
#include <tuple>

namespace phosphor
{
namespace fan
{
namespace presence
{

/** @brief PrettyName and inventory path. */
using Fan = std::tuple<std::string, std::string>;

/**
 * @brief Update the presence state.
 *
 * Update the Present property of the
 * xyz.openbmc_project.Inventory.Item interface.
 *
 * @param[in] fan - The fan to update.
 * @param[in] newState - The new state of the fan.
 */
void setPresence(const Fan& fan, bool newState);

/**
 * @brief Read the presence state.
 *
 * Read the Present property of the
 * xyz.openbmc_project.Inventory.Item
 *
 * @param[in] fan - The fan to read.
 */
bool getPresence(const Fan& fan);
} // namespace presence
} // namespace fan
} // namespace phosphor
