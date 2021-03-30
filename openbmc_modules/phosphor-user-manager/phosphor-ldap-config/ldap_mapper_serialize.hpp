#pragma once

#include <filesystem>
#include "config.h"
#include "ldap_mapper_entry.hpp"

namespace phosphor
{
namespace ldap
{

namespace fs = std::filesystem;

/** @brief Serialize and persist LDAP privilege mapper D-Bus object
 *
 *  @param[in] entry - LDAP privilege mapper entry
 *  @param[in] path - pathname of persisted LDAP mapper entry
 *
 *  @return fs::path - pathname of persisted error file
 */
fs::path serialize(const LDAPMapperEntry& entry, const fs::path& dir);

/** @brief Deserialize a persisted LDAP privilege mapper into a D-Bus object
 *
 *  @param[in] path - pathname of persisted file
 *  @param[in/out] entry - reference to  LDAP privilege mapper entry object
 *                         which is the target of deserialization.
 *
 *  @return bool - true if the deserialization was successful, false otherwise.
 */
bool deserialize(const fs::path& path, LDAPMapperEntry& entry);

} // namespace ldap
} // namespace phosphor
