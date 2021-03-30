#pragma once

#include "sdbusplus.hpp"

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace fan
{
namespace control
{
namespace match
{

using namespace phosphor::fan;
using namespace sdbusplus::bus::match;

/**
 * @brief A match function that constructs a PropertiesChanged match string
 * @details Constructs a PropertiesChanged match string with a given object
 * path and interface
 *
 * @param[in] obj - Object's path name
 * @param[in] iface - Interface name
 *
 * @return - A PropertiesChanged match string
 */
inline auto propertiesChanged(const std::string& obj, const std::string& iface)
{
    return rules::propertiesChanged(obj, iface);
}

/**
 * @brief A match function that constructs an InterfacesAdded match string
 * @details Constructs an InterfacesAdded match string with a given object
 * path
 *
 * @param[in] obj - Object's path name
 *
 * @return - An InterfacesAdded match string
 */
inline auto interfacesAdded(const std::string& obj)
{
    return rules::interfacesAdded(obj);
}

/**
 * @brief A match function that constructs an InterfacesRemoved match string
 * @details Constructs an InterfacesRemoved match string with a given object
 * path
 *
 * @param[in] obj - Object's path name
 *
 * @return - An InterfacesRemoved match string
 */
inline auto interfacesRemoved(const std::string& obj)
{
    return rules::interfacesRemoved(obj);
}

/**
 * @brief A match function that constructs a NameOwnerChanged match string
 * @details Constructs a NameOwnerChanged match string with a given object
 * path and interface
 *
 * @param[in] obj - Object's path name
 * @param[in] iface - Interface name
 *
 * @return - A NameOwnerChanged match string
 */
inline auto nameOwnerChanged(const std::string& obj, const std::string& iface)
{
    std::string noc;
    try
    {
        noc = rules::nameOwnerChanged(util::SDBusPlus::getService(obj, iface));
    }
    catch (const util::DBusError& e)
    {
        // Unable to construct NameOwnerChanged match string
    }
    return noc;
}

} // namespace match
} // namespace control
} // namespace fan
} // namespace phosphor
