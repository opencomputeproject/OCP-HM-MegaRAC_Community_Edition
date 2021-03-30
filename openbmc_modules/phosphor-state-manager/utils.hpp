#pragma once

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace state
{
namespace manager
{
namespace utils
{

/** @brief Get service name from object path and interface
 *
 * @param[in] bus          - The Dbus bus object
 * @param[in] path         - The Dbus object path
 * @param[in] interface    - The Dbus interface
 *
 * @return The name of the service
 */
std::string getService(sdbusplus::bus::bus& bus, std::string path,
                       std::string interface);

/** @brief Set the value of property
 *
 * @param[in] bus          - The Dbus bus object
 * @param[in] path         - The Dbus object path
 * @param[in] interface    - The Dbus interface
 * @param[in] property     - The property name to set
 * @param[in] value        - The value of property
 */
void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& interface, const std::string& property,
                 const std::string& value);

} // namespace utils
} // namespace manager
} // namespace state
} // namespace phosphor