#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/User/PrivilegeMapperEntry/server.hpp>
#include <xyz/openbmc_project/Object/Delete/server.hpp>

namespace phosphor
{
namespace ldap
{

namespace Base = sdbusplus::xyz::openbmc_project;
using Entry =
    sdbusplus::xyz::openbmc_project::User::server::PrivilegeMapperEntry;
using Delete = sdbusplus::xyz::openbmc_project::Object::server::Delete;
using Interfaces = sdbusplus::server::object::object<Entry, Delete>;

// Forward declaration for Config
class Config;

using Id = size_t;

/** @class LDAPMapperEntry
 *
 *  @brief This D-Bus object represents the privilege level for the LDAP group.
 */
class LDAPMapperEntry : public Interfaces
{
  public:
    LDAPMapperEntry() = delete;
    ~LDAPMapperEntry() = default;
    LDAPMapperEntry(const LDAPMapperEntry &) = delete;
    LDAPMapperEntry &operator=(const LDAPMapperEntry &) = delete;
    LDAPMapperEntry(LDAPMapperEntry &&) = default;
    LDAPMapperEntry &operator=(LDAPMapperEntry &&) = default;

    /** @brief Constructs LDAP privilege mapper entry object.
     *
     *  @param[in] bus  - sdbusplus handler
     *  @param[in] path - D-Bus path
     *  @param[in] filePath - serialization directory path
     *  @param[in] groupName - LDAP group name
     *  @param[in] privilege - the privilege for the group
     *  @param[in] parent - LDAP privilege mapper manager
     */
    LDAPMapperEntry(sdbusplus::bus::bus &bus, const char *path,
                    const char *filePath, const std::string &groupName,
                    const std::string &privilege, Config &parent);

    /** @brief Constructs LDAP privilege mapper entry object
     *
     *  @param[in] bus  - sdbusplus handler
     *  @param[in] path - D-Bus path
     *  @param[in] filePath - serialization directory path
     *  @param[in] parent - LDAP privilege mapper manager
     */
    LDAPMapperEntry(sdbusplus::bus::bus &bus, const char *path,
                    const char *filePath, Config &parent);

    /** @brief Delete privilege mapper entry object
     *
     *  This method deletes the privilege mapper entry.
     */
    void delete_(void) override;

    /** @brief Update the group name of the mapper object
     *
     *  @param[in] value - group name
     *
     *  @return On success the updated group name
     */
    std::string groupName(std::string value) override;

    /** @brief Update privilege associated with LDAP group
     *
     *  @param[in] value - privilege level
     *
     *  @return On success the updated privilege level
     */
    std::string privilege(std::string value) override;

    using sdbusplus::xyz::openbmc_project::User::server::PrivilegeMapperEntry::
        privilege;

    using sdbusplus::xyz::openbmc_project::User::server::PrivilegeMapperEntry::
        groupName;

  private:
    Id id;
    Config &manager;

    /** @brief serialization directory path */
    std::string persistPath;
};

} // namespace ldap
} // namespace phosphor
