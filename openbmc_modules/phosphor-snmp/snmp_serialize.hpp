#pragma once

#include <experimental/filesystem>
#include "snmp_client.hpp"

namespace phosphor
{
namespace network
{
namespace snmp
{

constexpr auto SEPARATOR = "_";

namespace fs = std::experimental::filesystem;

/** @brief Serialize and persist SNMP manager/client D-Bus object.
 *  @param[in] id - filename of the persisted SNMP manager object.
 *  @param[in] manager - const reference to snmp client/manager object.
 *  @param[in] path -  path of persistent location where D-Bus object would be
 * saved.
 *  @return fs::path - pathname of persisted snmp manager/client file.
 */
fs::path serialize(Id id, const Client& manager, const fs::path& path);

/** @brief Deserialze SNMP manager/client info into a D-Bus object
 *  @param[in] path - pathname of persisted manager/client file.
 *  @param[in] manager - reference to snmp client/manager object
 *                       which is the target of deserialization.
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, Client& manager);

} // namespace snmp
} // namespace network
} // namespace phosphor
