#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include "ldap_mapper_entry.hpp"
#include <xyz/openbmc_project/User/PrivilegeMapper/server.hpp>
#include <map>
#include <set>

namespace phosphor
{

namespace user
{

using MapperMgrIface =
    sdbusplus::xyz::openbmc_project::User::server::PrivilegeMapper;
using ObjectPath = sdbusplus::message::object_path;

// D-Bus root for LDAP privilege mapper
constexpr auto mapperMgrRoot = "/xyz/openbmc_project/user/ldap";

/** @class LDAPMapperMgr
 *
 *  @brief Responsible for managing LDAP groups to privilege mapping.
 */
class LDAPMapperMgr : public MapperMgrIface
{
  public:
    LDAPMapperMgr() = delete;
    ~LDAPMapperMgr() = default;
    LDAPMapperMgr(const LDAPMapperMgr &) = delete;
    LDAPMapperMgr &operator=(const LDAPMapperMgr &) = delete;
    LDAPMapperMgr(LDAPMapperMgr &&) = delete;
    LDAPMapperMgr &operator=(LDAPMapperMgr &&) = delete;

    /** @brief Constructs LDAPMapperMgr object.
     *
     *  @param[in] bus  - sdbusplus handler
     *  @param[in] path - D-Bus path
     *  @param[in] filePath - serialization directory path
     */
    LDAPMapperMgr(sdbusplus::bus::bus &bus, const char *path,
                  const char *filePath);

    /** @brief Creates a mapping for the group to the privilege
     *
     *  @param[in] groupName - Group Name to which the privilege needs to be
     *                         assigned.
     *  @param[in] privilege - The privilege role associated with the group.
     *
     *  @return On success return the D-Bus object path of the created privilege
     *          mapper entry.
     */
    ObjectPath create(std::string groupName, std::string privilege) override;

    /** @brief Delete privilege mapping for LDAP group
     *
     *  This method deletes the privilege mapping
     *
     *  @param[in] groupName - name of the LDAP group for which privilege
     *                         mapping is to be deleted.
     */
    void deletePrivilegeMapper(Id id);

    /** @brief Check if LDAP group privilege mapping requested is valid
     *
     *  Check if the privilege mapping already exists for the LDAP group name
     *  and group name is empty.
     *
     *  @param[in] groupName - LDAP group name
     *
     *  @return throw exception if the conditions are not met.
     */
    void checkPrivilegeMapper(const std::string &groupName);

    /** @brief Check if the privilege level is a valid one
     *
     *  @param[in] privilege - Privilege level
     *
     *  @return throw exception if the conditions are not met.
     */
    void checkPrivilegeLevel(const std::string &privilege);

    /** @brief Construct LDAP mapper entry D-Bus objects from their persisted
     *         representations.
     */
    void restore();

  private:
    /** @brief sdbusplus handler */
    sdbusplus::bus::bus &bus;

    /** @brief object path for the manager object*/
    const std::string path;

    /** @brief serialization directory path */
    std::string persistPath;

    /** @brief available privileges container */
    std::set<std::string> privMgr = {
        "priv-admin",
        "priv-operator",
        "priv-user",
        "priv-noaccess",
    };

    /** @brief Id of the last privilege mapper entry */
    Id entryId = 0;

    /** @brief container to hold privilege mapper objects */
    std::map<Id, std::unique_ptr<phosphor::user::LDAPMapperEntry>>
        PrivilegeMapperList;
};

} // namespace user
} // namespace phosphor
