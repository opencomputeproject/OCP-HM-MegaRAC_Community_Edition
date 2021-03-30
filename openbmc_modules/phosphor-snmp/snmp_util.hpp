#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <map>
#include <string>

#include <sdbusplus/server.hpp>

namespace phosphor
{

/* Need a custom deleter for freeing up addrinfo */
struct AddrDeleter
{
    void operator()(addrinfo* addrPtr) const
    {
        freeaddrinfo(addrPtr);
    }
};

using AddrPtr = std::unique_ptr<addrinfo, AddrDeleter>;

using DbusInterface = std::string;
using DbusProperty = std::string;

using Value = std::variant<bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
                           int64_t, uint64_t, std::string>;

using PropertyMap = std::map<DbusProperty, Value>;

using DbusInterfaceMap = std::map<DbusInterface, PropertyMap>;

using ObjectValueTree =
    std::map<sdbusplus::message::object_path, DbusInterfaceMap>;

/** @brief Gets all managed objects associated with the given object
 *         path and service.
 *  @param[in] bus - D-Bus Bus Object.
 *  @param[in] service - D-Bus service name.
 *  @param[in] objPath - D-Bus object path.
 *  @return On success returns the map of name value pair.
 */
ObjectValueTree getManagedObjects(sdbusplus::bus::bus& bus,
                                  const std::string& service,
                                  const std::string& objPath);

namespace network
{

/** @brief Resolves the given address to IP address.
 *         Given address could be hostname or IP address.
 *         if given address is not valid then it throws an exception.
 *  @param[in] address - address which needs to be converted into IP address.
 *  @return the IP address.
 */
std::string resolveAddress(const std::string& address);

namespace snmp
{

/** @brief Gets all the snmp manager info.
 *  @return the list of manager info in the format
 *          of ipaddress:port
 */
std::vector<std::string> getManagers();

} // namespace snmp
} // namespace network

} // namespace phosphor
