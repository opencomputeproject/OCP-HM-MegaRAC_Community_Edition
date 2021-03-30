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

#include "user_layer.hpp"

#include "passwd_mgr.hpp"
#include "user_mgmt.hpp"

namespace
{
ipmi::PasswdMgr passwdMgr;
}

namespace ipmi
{

Cc ipmiUserInit()
{
    getUserAccessObject();
    return ccSuccess;
}

std::string ipmiUserGetPassword(const std::string& userName)
{
    return passwdMgr.getPasswdByUserName(userName);
}

Cc ipmiClearUserEntryPassword(const std::string& userName)
{
    if (passwdMgr.updateUserEntry(userName, "") != 0)
    {
        return ccUnspecifiedError;
    }
    return ccSuccess;
}

Cc ipmiRenameUserEntryPassword(const std::string& userName,
                               const std::string& newUserName)
{
    if (passwdMgr.updateUserEntry(userName, newUserName) != 0)
    {
        return ccUnspecifiedError;
    }
    return ccSuccess;
}

bool ipmiUserIsValidUserId(const uint8_t userId)
{
    return UserAccess::isValidUserId(userId);
}

bool ipmiUserIsValidPrivilege(const uint8_t priv)
{
    return UserAccess::isValidPrivilege(priv);
}

uint8_t ipmiUserGetUserId(const std::string& userName)
{
    return getUserAccessObject().getUserId(userName);
}

Cc ipmiUserSetUserName(const uint8_t userId, const char* userName)
{
    std::string newUser(userName, 0, ipmiMaxUserName);
    return getUserAccessObject().setUserName(userId, newUser);
}

Cc ipmiUserSetUserName(const uint8_t userId, const std::string& userName)
{
    std::string newUser(userName, 0, ipmiMaxUserName);
    return getUserAccessObject().setUserName(userId, newUser);
}

Cc ipmiUserGetUserName(const uint8_t userId, std::string& userName)
{
    return getUserAccessObject().getUserName(userId, userName);
}

Cc ipmiUserSetUserPassword(const uint8_t userId, const char* userPassword)
{
    return getUserAccessObject().setUserPassword(userId, userPassword);
}

Cc ipmiSetSpecialUserPassword(const std::string& userName,
                              const std::string& userPassword)
{
    return getUserAccessObject().setSpecialUserPassword(userName, userPassword);
}

Cc ipmiUserGetAllCounts(uint8_t& maxChUsers, uint8_t& enabledUsers,
                        uint8_t& fixedUsers)
{
    maxChUsers = ipmiMaxUsers;
    UsersTbl* userData = getUserAccessObject().getUsersTblPtr();
    enabledUsers = 0;
    fixedUsers = 0;
    // user index 0 is reserved, starts with 1
    for (size_t count = 1; count <= ipmiMaxUsers; ++count)
    {
        if (userData->user[count].userEnabled)
        {
            enabledUsers++;
        }
        if (userData->user[count].fixedUserName)
        {
            fixedUsers++;
        }
    }
    return ccSuccess;
}

Cc ipmiUserUpdateEnabledState(const uint8_t userId, const bool& state)
{
    return getUserAccessObject().setUserEnabledState(userId, state);
}

Cc ipmiUserCheckEnabled(const uint8_t userId, bool& state)
{
    if (!UserAccess::isValidUserId(userId))
    {
        return ccParmOutOfRange;
    }
    UserInfo* userInfo = getUserAccessObject().getUserInfo(userId);
    state = userInfo->userEnabled;
    return ccSuccess;
}

Cc ipmiUserGetPrivilegeAccess(const uint8_t userId, const uint8_t chNum,
                              PrivAccess& privAccess)
{

    if (!UserAccess::isValidChannel(chNum))
    {
        return ccInvalidFieldRequest;
    }
    if (!UserAccess::isValidUserId(userId))
    {
        return ccParmOutOfRange;
    }
    UserInfo* userInfo = getUserAccessObject().getUserInfo(userId);
    privAccess.privilege = userInfo->userPrivAccess[chNum].privilege;
    privAccess.ipmiEnabled = userInfo->userPrivAccess[chNum].ipmiEnabled;
    privAccess.linkAuthEnabled =
        userInfo->userPrivAccess[chNum].linkAuthEnabled;
    privAccess.accessCallback = userInfo->userPrivAccess[chNum].accessCallback;
    return ccSuccess;
}

Cc ipmiUserSetPrivilegeAccess(const uint8_t userId, const uint8_t chNum,
                              const PrivAccess& privAccess,
                              const bool& otherPrivUpdates)
{
    UserPrivAccess userPrivAccess;
    userPrivAccess.privilege = privAccess.privilege;
    if (otherPrivUpdates)
    {
        userPrivAccess.ipmiEnabled = privAccess.ipmiEnabled;
        userPrivAccess.linkAuthEnabled = privAccess.linkAuthEnabled;
        userPrivAccess.accessCallback = privAccess.accessCallback;
    }
    return getUserAccessObject().setUserPrivilegeAccess(
        userId, chNum, userPrivAccess, otherPrivUpdates);
}

bool ipmiUserPamAuthenticate(std::string_view userName,
                             std::string_view userPassword)
{
    return pamUserCheckAuthenticate(userName, userPassword);
}

Cc ipmiUserSetUserPayloadAccess(const uint8_t chNum, const uint8_t operation,
                                const uint8_t userId,
                                const PayloadAccess& payloadAccess)
{

    if (!UserAccess::isValidChannel(chNum))
    {
        return ccInvalidFieldRequest;
    }
    if (!UserAccess::isValidUserId(userId))
    {
        return ccParmOutOfRange;
    }

    return getUserAccessObject().setUserPayloadAccess(chNum, operation, userId,
                                                      payloadAccess);
}

Cc ipmiUserGetUserPayloadAccess(const uint8_t chNum, const uint8_t userId,
                                PayloadAccess& payloadAccess)
{

    if (!UserAccess::isValidChannel(chNum))
    {
        return ccInvalidFieldRequest;
    }
    if (!UserAccess::isValidUserId(userId))
    {
        return ccParmOutOfRange;
    }

    UserInfo* userInfo = getUserAccessObject().getUserInfo(userId);

    payloadAccess.stdPayloadEnables1 =
        userInfo->payloadAccess[chNum].stdPayloadEnables1;
    payloadAccess.stdPayloadEnables2Reserved =
        userInfo->payloadAccess[chNum].stdPayloadEnables2Reserved;
    payloadAccess.oemPayloadEnables1 =
        userInfo->payloadAccess[chNum].oemPayloadEnables1;
    payloadAccess.oemPayloadEnables2Reserved =
        userInfo->payloadAccess[chNum].oemPayloadEnables2Reserved;

    return ccSuccess;
}

} // namespace ipmi
