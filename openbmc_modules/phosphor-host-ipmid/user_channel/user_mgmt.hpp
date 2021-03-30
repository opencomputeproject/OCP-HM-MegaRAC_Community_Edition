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
#pragma once
#include "user_layer.hpp"

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/named_recursive_mutex.hpp>
#include <cstdint>
#include <ctime>
#include <ipmid/api.hpp>
#include <sdbusplus/bus.hpp>
#include <variant>

namespace ipmi
{

using DbusUserPropVariant =
    std::variant<std::vector<std::string>, std::string, bool>;

using DbusUserObjPath = sdbusplus::message::object_path;

using DbusUserObjProperties =
    std::vector<std::pair<std::string, DbusUserPropVariant>>;

using DbusUserObjValue = std::map<std::string, DbusUserObjProperties>;

/**
 * @enum User update events.
 */
enum class UserUpdateEvent
{
    reservedEvent,
    userCreated,
    userDeleted,
    userRenamed,
    userGrpUpdated,
    userPrivUpdated,
    userStateUpdated
};

/** @struct UserPrivAccess
 *
 *  Structure for user privilege access (refer spec sec 22.22)
 */
struct UserPrivAccess
{
    uint8_t privilege;
    bool ipmiEnabled;
    bool linkAuthEnabled;
    bool accessCallback;
};

/** @struct UserInfo
 *
 *  Structure for user related information
 */
struct UserInfo
{
    uint8_t userName[ipmiMaxUserName];
    UserPrivAccess userPrivAccess[ipmiMaxChannels];
    bool userEnabled;
    bool userInSystem;
    bool fixedUserName;
    PayloadAccess payloadAccess[ipmiMaxChannels];
};

/** @struct UsersTbl
 *
 *  Structure for array of user related information
 */
struct UsersTbl
{
    //+1 to map with UserId directly. UserId 0 is reserved.
    UserInfo user[ipmiMaxUsers + 1];
};

/** @brief PAM User Authentication check
 *
 *  @param[in] username - username in string
 *  @param[in] password	- password in string
 *
 *  @return status
 */
bool pamUserCheckAuthenticate(std::string_view username,
                              std::string_view password);

class UserAccess;

UserAccess& getUserAccessObject();

class UserAccess
{
  public:
    UserAccess(const UserAccess&) = delete;
    UserAccess& operator=(const UserAccess&) = delete;
    UserAccess(UserAccess&&) = delete;
    UserAccess& operator=(UserAccess&&) = delete;

    ~UserAccess();
    UserAccess();

    /** @brief determines valid channel
     *
     *  @param[in] chNum - channel number
     *
     *  @return true if valid, false otherwise
     */
    static bool isValidChannel(const uint8_t chNum);

    /** @brief determines valid userId
     *
     *  @param[in] userId - user id
     *
     *  @return true if valid, false otherwise
     */
    static bool isValidUserId(const uint8_t userId);

    /** @brief determines valid user privilege
     *
     *  @param[in] priv - Privilege
     *
     *  @return true if valid, false otherwise
     */
    static bool isValidPrivilege(const uint8_t priv);

    /** @brief determines sync index to be mapped with common-user-management
     *
     *  @return Index which will be used as sync index
     */
    static uint8_t getUsrMgmtSyncIndex();

    /** @brief Converts system privilege to IPMI privilege
     *
     *  @param[in] value - Privilege in string
     *
     *  @return CommandPrivilege - IPMI privilege type
     */
    static CommandPrivilege convertToIPMIPrivilege(const std::string& value);

    /** @brief Converts IPMI privilege to system privilege
     *
     *  @param[in] value - IPMI privilege
     *
     *  @return System privilege in string
     */
    static std::string convertToSystemPrivilege(const CommandPrivilege& value);

    /** @brief determines whether user name is valid
     *
     *  @param[in] userNameInChar - user name
     *
     *  @return true if valid, false otherwise
     */
    bool isValidUserName(const std::string& userName);

    /** @brief determines whether ipmi is in available groups list
     *
     * @return true if ipmi group is present, false otherwise
     */
    bool isIpmiInAvailableGroupList();

    /** @brief provides user id of the user
     *
     *  @param[in] userName - user name
     *
     *  @return user id of the user, else invalid user id (0xFF), if user not
     * found
     */
    uint8_t getUserId(const std::string& userName);

    /** @brief provides user information
     *
     *  @param[in] userId - user id
     *
     *  @return UserInfo for the specified user id
     */
    UserInfo* getUserInfo(const uint8_t userId);

    /** @brief sets user information
     *
     *  @param[in] userId - user id
     *  @param[in] userInfo - user information
     *
     */
    void setUserInfo(const uint8_t userId, UserInfo* userInfo);

    /** @brief provides user name
     *
     *  @param[in] userId - user id
     *  @param[out] userName - user name
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc getUserName(const uint8_t userId, std::string& userName);

    /** @brief to set user name
     *
     *  @param[in] userId - user id
     *  @param[in] userName - user name
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc setUserName(const uint8_t userId, const std::string& userName);

    /** @brief to set user enabled state
     *
     *  @param[in] userId - user id
     *  @param[in] enabledState - enabled state of the user
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc setUserEnabledState(const uint8_t userId, const bool& enabledState);

    /** @brief to set user password
     *
     *  @param[in] userId - user id
     *  @param[in] userPassword  - new password of the user
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc setUserPassword(const uint8_t userId, const char* userPassword);

    /** @brief to set special user password
     *
     *  @param[in] userName - user name
     *  @param[in] userPassword  - new password of the user
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc setSpecialUserPassword(const std::string& userName,
                              const std::string& userPassword);

    /** @brief to set user privilege and access details
     *
     *  @param[in] userId - user id
     *  @param[in] chNum - channel number
     *  @param[in] privAccess - privilege access
     *  @param[in] otherPrivUpdates - other privilege update flag to update ipmi
     * enable, link authentication and access callback
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc setUserPrivilegeAccess(const uint8_t userId, const uint8_t chNum,
                              const UserPrivAccess& privAccess,
                              const bool& otherPrivUpdates);

    /** @brief to get user payload access details from userInfo entry.
     *
     *  @param[in] userInfo    - userInfo entry in usersTbl.
     *  @param[out] stdPayload - stdPayloadEnables1 in a 2D-array.
     *  @param[out] oemPayload - oemPayloadEnables1 in a 2D-array.
     *
     *  @details Update the given 2D-arrays using the payload access details
     *  available in the given userInfo entry (from usersTbl).
     *  This 2D-array will be mapped to a JSON object (which will be written to
     *  a JSON file subsequently).
     */
    void readPayloadAccessFromUserInfo(
        const UserInfo& userInfo,
        std::array<std::array<bool, ipmiMaxChannels>, payloadsPerByte>&
            stdPayload,
        std::array<std::array<bool, ipmiMaxChannels>, payloadsPerByte>&
            oemPayload);

    /** @brief to update user payload access details in userInfo entry.
     *
     *  @param[in] stdPayload - stdPayloadEnables1 in a 2D-array.
     *  @param[in] oemPayload - oemPayloadEnables1 in a 2D-array.
     *  @param[out] userInfo  - userInfo entry in usersTbl.
     *
     *  @details Update user payload access details of a given userInfo
     *  entry (in usersTbl) with the information provided in given 2D-arrays.
     *  This 2D-array was created out of a JSON object (which was created by
     *  parsing a JSON file).
     */
    void updatePayloadAccessInUserInfo(
        const std::array<std::array<bool, ipmiMaxChannels>, payloadsPerByte>&
            stdPayload,
        const std::array<std::array<bool, ipmiMaxChannels>, payloadsPerByte>&
            oemPayload,
        UserInfo& userInfo);

    /** @brief to set user payload access details
     *
     *  @param[in] chNum - channel number
     *  @param[in] operation - Enable / Disable
     *  @param[in] userId - user id
     *  @param[in] payloadAccess - payload access
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc setUserPayloadAccess(const uint8_t chNum, const uint8_t operation,
                            const uint8_t userId,
                            const PayloadAccess& payloadAccess);

    /** @brief reads user management related data from configuration file
     *
     */
    void readUserData();

    /** @brief writes user management related data to configuration file
     *
     */
    void writeUserData();

    /** @brief Funtion which checks and reload configuration file data if
     * needed.
     *
     */
    void checkAndReloadUserData();

    /** @brief provides user details from D-Bus user property data
     *
     *  @param[in] properties - D-Bus user property
     *  @param[out] usrGrps - user group details
     *  @param[out] usrPriv - user privilege
     *  @param[out] usrEnabled - enabled state of the user.
     *
     *  @return 0 for success, -errno for failure.
     */
    void getUserProperties(const DbusUserObjProperties& properties,
                           std::vector<std::string>& usrGrps,
                           std::string& usrPriv, bool& usrEnabled);

    /** @brief provides user details from D-Bus user object data
     *
     *  @param[in] userObjs - D-Bus user object
     *  @param[out] usrGrps - user group details
     *  @param[out] usrPriv - user privilege
     *  @param[out] usrEnabled - enabled state of the user.
     *
     *  @return 0 for success, -errno for failure.
     */
    int getUserObjProperties(const DbusUserObjValue& userObjs,
                             std::vector<std::string>& usrGrps,
                             std::string& usrPriv, bool& usrEnabled);

    /** @brief function to add user entry information to the configuration
     *
     *  @param[in] userName - user name
     *  @param[in] priv - privilege of the user
     *  @param[in] enabled - enabled state of the user
     *
     *  @return true for success, false for failure
     */
    bool addUserEntry(const std::string& userName, const std::string& priv,
                      const bool& enabled);

    /** @brief function to delete user entry based on user index
     *
     *  @param[in] usrIdx - user index
     *
     */
    void deleteUserIndex(const size_t& usrIdx);

    /** @brief function to get users table
     *
     */
    UsersTbl* getUsersTblPtr();

    std::unique_ptr<boost::interprocess::named_recursive_mutex> userMutex{
        nullptr};

  private:
    UsersTbl usersTbl;
    std::vector<std::string> availablePrivileges;
    std::vector<std::string> availableGroups;
    sdbusplus::bus::bus bus;
    std::time_t fileLastUpdatedTime;
    bool signalHndlrObject = false;
    boost::interprocess::file_lock sigHndlrLock;
    boost::interprocess::file_lock mutexCleanupLock;

    /** @brief function to get user configuration file timestamp
     *
     *  @return time stamp or -EIO for failure
     */
    std::time_t getUpdatedFileTime();

    /** @brief function to available system privileges and groups
     *
     */
    void getSystemPrivAndGroups();

    /** @brief function to init user data from configuration & D-Bus objects
     * and to register for signals
     *
     */
    void cacheUserDataFile();
};
} // namespace ipmi
