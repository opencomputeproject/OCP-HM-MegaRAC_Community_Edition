/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/User/Common/error.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include "user_mgr.hpp"
#include "users.hpp"
#include "config.h"

namespace phosphor
{
namespace user
{

using namespace phosphor::logging;
using InsufficientPermission =
    sdbusplus::xyz::openbmc_project::Common::Error::InsufficientPermission;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using InvalidArgument =
    sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
using NoResource =
    sdbusplus::xyz::openbmc_project::User::Common::Error::NoResource;

using Argument = xyz::openbmc_project::Common::InvalidArgument;

/** @brief Constructs UserMgr object.
 *
 *  @param[in] bus  - sdbusplus handler
 *  @param[in] path - D-Bus path
 *  @param[in] groups - users group list
 *  @param[in] priv - user privilege
 *  @param[in] enabled - user enabled state
 *  @param[in] parent - user manager - parent object
 */
Users::Users(sdbusplus::bus::bus &bus, const char *path,
             std::vector<std::string> groups, std::string priv, bool enabled,
             UserMgr &parent) :
    Interfaces(bus, path, true),
    userName(std::experimental::filesystem::path(path).filename()),
    manager(parent)
{
    UsersIface::userPrivilege(priv, true);
    UsersIface::userGroups(groups, true);
    UsersIface::userEnabled(enabled, true);

    this->emit_object_added();
}

/** @brief delete user method.
 *  This method deletes the user as requested
 *
 */
void Users::delete_(void)
{
    manager.deleteUser(userName);
}

/** @brief update user privilege
 *
 *  @param[in] value - User privilege
 */
std::string Users::userPrivilege(std::string value)
{
    if (value == UsersIface::userPrivilege())
    {
        return value;
    }
    manager.updateGroupsAndPriv(userName, UsersIface::userGroups(), value);
    return UsersIface::userPrivilege(value);
}

/** @brief list user privilege
 *
 */
std::string Users::userPrivilege(void) const
{
    return UsersIface::userPrivilege();
}

/** @brief update user groups
 *
 *  @param[in] value - User groups
 */
std::vector<std::string> Users::userGroups(std::vector<std::string> value)
{
    if (value == UsersIface::userGroups())
    {
        return value;
    }
    std::sort(value.begin(), value.end());
    manager.updateGroupsAndPriv(userName, value, UsersIface::userPrivilege());
    return UsersIface::userGroups(value);
}

/** @brief list user groups
 *
 */
std::vector<std::string> Users::userGroups(void) const
{
    return UsersIface::userGroups();
}

/** @brief lists user enabled state
 *
 */
bool Users::userEnabled(void) const
{
    return UsersIface::userEnabled();
}

/** @brief update user enabled state
 *
 *  @param[in] value - bool value
 */
bool Users::userEnabled(bool value)
{
    if (value == UsersIface::userEnabled())
    {
        return value;
    }
    manager.userEnable(userName, value);
    return UsersIface::userEnabled(value);
}

/** @brief lists user locked state for failed attempt
 *
 **/
bool Users::userLockedForFailedAttempt(void) const
{
    return manager.userLockedForFailedAttempt(userName);
}

/** @brief unlock user locked state for failed attempt
 *
 * @param[in]: value - false - unlock user account, true - no action taken
 **/
bool Users::userLockedForFailedAttempt(bool value)
{
    if (value != false)
    {
        return userLockedForFailedAttempt();
    }
    else
    {
        return manager.userLockedForFailedAttempt(userName, value);
    }
}

/** @brief indicates if the user's password is expired
 *
 **/
bool Users::userPasswordExpired(void) const
{
    return manager.userPasswordExpired(userName);
}

} // namespace user
} // namespace phosphor
