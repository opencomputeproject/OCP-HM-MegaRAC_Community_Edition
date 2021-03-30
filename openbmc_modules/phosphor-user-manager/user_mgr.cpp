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

#include <shadow.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <regex>
#include <algorithm>
#include <numeric>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <boost/algorithm/string/split.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/User/Common/error.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include "shadowlock.hpp"
#include "file.hpp"
#include "user_mgr.hpp"
#include "users.hpp"
#include "config.h"

namespace phosphor
{
namespace user
{

static constexpr const char *passwdFileName = "/etc/passwd";
static constexpr size_t ipmiMaxUsers = 15;
static constexpr size_t ipmiMaxUserNameLen = 16;
static constexpr size_t systemMaxUserNameLen = 30;
static constexpr size_t maxSystemUsers = 30;
static constexpr const char *grpSsh = "ssh";
static constexpr uint8_t minPasswdLength = 8;
static constexpr int success = 0;
static constexpr int failure = -1;

// pam modules related
static constexpr const char *pamTally2 = "pam_tally2.so";
static constexpr const char *pamCrackLib = "pam_cracklib.so";
static constexpr const char *pamPWHistory = "pam_pwhistory.so";
static constexpr const char *minPasswdLenProp = "minlen";
static constexpr const char *remOldPasswdCount = "remember";
static constexpr const char *maxFailedAttempt = "deny";
static constexpr const char *unlockTimeout = "unlock_time";
static constexpr const char *pamPasswdConfigFile = "/etc/pam.d/common-password";
static constexpr const char *pamAuthConfigFile = "/etc/pam.d/common-auth";

// Object Manager related
static constexpr const char *ldapMgrObjBasePath =
    "/xyz/openbmc_project/user/ldap";

// Object Mapper related
static constexpr const char *objMapperService =
    "xyz.openbmc_project.ObjectMapper";
static constexpr const char *objMapperPath =
    "/xyz/openbmc_project/object_mapper";
static constexpr const char *objMapperInterface =
    "xyz.openbmc_project.ObjectMapper";

using namespace phosphor::logging;
using InsufficientPermission =
    sdbusplus::xyz::openbmc_project::Common::Error::InsufficientPermission;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using InvalidArgument =
    sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
using UserNameExists =
    sdbusplus::xyz::openbmc_project::User::Common::Error::UserNameExists;
using UserNameDoesNotExist =
    sdbusplus::xyz::openbmc_project::User::Common::Error::UserNameDoesNotExist;
using UserNameGroupFail =
    sdbusplus::xyz::openbmc_project::User::Common::Error::UserNameGroupFail;
using NoResource =
    sdbusplus::xyz::openbmc_project::User::Common::Error::NoResource;

using Argument = xyz::openbmc_project::Common::InvalidArgument;

template <typename... ArgTypes>
static std::vector<std::string> executeCmd(const char *path,
                                           ArgTypes &&... tArgs)
{
    std::vector<std::string> stdOutput;
    boost::process::ipstream stdOutStream;
    boost::process::child execProg(path, const_cast<char *>(tArgs)...,
                                   boost::process::std_out > stdOutStream);
    std::string stdOutLine;

    while (stdOutStream && std::getline(stdOutStream, stdOutLine) &&
           !stdOutLine.empty())
    {
        stdOutput.emplace_back(stdOutLine);
    }

    execProg.wait();

    int retCode = execProg.exit_code();
    if (retCode)
    {
        log<level::ERR>("Command execution failed", entry("PATH=%d", path),
                        entry("RETURN_CODE:%d", retCode));
        elog<InternalFailure>();
    }

    return stdOutput;
}

static std::string getCSVFromVector(std::vector<std::string> vec)
{
    switch (vec.size())
    {
        case 0:
        {
            return "";
        }
        break;

        case 1:
        {
            return std::string{vec[0]};
        }
        break;

        default:
        {
            return std::accumulate(
                std::next(vec.begin()), vec.end(), vec[0],
                [](std::string a, std::string b) { return a + ',' + b; });
        }
    }
}

static bool removeStringFromCSV(std::string &csvStr, const std::string &delStr)
{
    std::string::size_type delStrPos = csvStr.find(delStr);
    if (delStrPos != std::string::npos)
    {
        // need to also delete the comma char
        if (delStrPos == 0)
        {
            csvStr.erase(delStrPos, delStr.size() + 1);
        }
        else
        {
            csvStr.erase(delStrPos - 1, delStr.size() + 1);
        }
        return true;
    }
    return false;
}

bool UserMgr::isUserExist(const std::string &userName)
{
    if (userName.empty())
    {
        log<level::ERR>("User name is empty");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("User name"),
                              Argument::ARGUMENT_VALUE("Null"));
    }
    if (usersList.find(userName) == usersList.end())
    {
        return false;
    }
    return true;
}

void UserMgr::throwForUserDoesNotExist(const std::string &userName)
{
    if (isUserExist(userName) == false)
    {
        log<level::ERR>("User does not exist",
                        entry("USER_NAME=%s", userName.c_str()));
        elog<UserNameDoesNotExist>();
    }
}

void UserMgr::throwForUserExists(const std::string &userName)
{
    if (isUserExist(userName) == true)
    {
        log<level::ERR>("User already exists",
                        entry("USER_NAME=%s", userName.c_str()));
        elog<UserNameExists>();
    }
}

void UserMgr::throwForUserNameConstraints(
    const std::string &userName, const std::vector<std::string> &groupNames)
{
    if (std::find(groupNames.begin(), groupNames.end(), "ipmi") !=
        groupNames.end())
    {
        if (userName.length() > ipmiMaxUserNameLen)
        {
            log<level::ERR>("IPMI user name length limitation",
                            entry("SIZE=%d", userName.length()));
            elog<UserNameGroupFail>(
                xyz::openbmc_project::User::Common::UserNameGroupFail::REASON(
                    "IPMI length"));
        }
    }
    if (userName.length() > systemMaxUserNameLen)
    {
        log<level::ERR>("User name length limitation",
                        entry("SIZE=%d", userName.length()));
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("User name"),
                              Argument::ARGUMENT_VALUE("Invalid length"));
    }
    if (!std::regex_match(userName.c_str(),
                          std::regex("[a-zA-z_][a-zA-Z_0-9]*")))
    {
        log<level::ERR>("Invalid user name",
                        entry("USER_NAME=%s", userName.c_str()));
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("User name"),
                              Argument::ARGUMENT_VALUE("Invalid data"));
    }
}

void UserMgr::throwForMaxGrpUserCount(
    const std::vector<std::string> &groupNames)
{
    if (std::find(groupNames.begin(), groupNames.end(), "ipmi") !=
        groupNames.end())
    {
        if (getIpmiUsersCount() >= ipmiMaxUsers)
        {
            log<level::ERR>("IPMI user limit reached");
            elog<NoResource>(
                xyz::openbmc_project::User::Common::NoResource::REASON(
                    "ipmi user count reached"));
        }
    }
    else
    {
        if (usersList.size() > 0 && (usersList.size() - getIpmiUsersCount()) >=
                                        (maxSystemUsers - ipmiMaxUsers))
        {
            log<level::ERR>("Non-ipmi User limit reached");
            elog<NoResource>(
                xyz::openbmc_project::User::Common::NoResource::REASON(
                    "Non-ipmi user count reached"));
        }
    }
    return;
}

void UserMgr::throwForInvalidPrivilege(const std::string &priv)
{
    if (!priv.empty() &&
        (std::find(privMgr.begin(), privMgr.end(), priv) == privMgr.end()))
    {
        log<level::ERR>("Invalid privilege");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("Privilege"),
                              Argument::ARGUMENT_VALUE(priv.c_str()));
    }
}

void UserMgr::throwForInvalidGroups(const std::vector<std::string> &groupNames)
{
    for (auto &group : groupNames)
    {
        if (std::find(groupsMgr.begin(), groupsMgr.end(), group) ==
            groupsMgr.end())
        {
            log<level::ERR>("Invalid Group Name listed");
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("GroupName"),
                                  Argument::ARGUMENT_VALUE(group.c_str()));
        }
    }
}

void UserMgr::createUser(std::string userName,
                         std::vector<std::string> groupNames, std::string priv,
                         bool enabled)
{
    throwForInvalidPrivilege(priv);
    throwForInvalidGroups(groupNames);
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();
    throwForUserExists(userName);
    throwForUserNameConstraints(userName, groupNames);
    throwForMaxGrpUserCount(groupNames);

    std::string groups = getCSVFromVector(groupNames);
    bool sshRequested = removeStringFromCSV(groups, grpSsh);

    // treat privilege as a group - This is to avoid using different file to
    // store the same.
    if (!priv.empty())
    {
        if (groups.size() != 0)
        {
            groups += ",";
        }
        groups += priv;
    }
    try
    {
        executeCmd("/usr/sbin/useradd", userName.c_str(), "-G", groups.c_str(),
                   "-m", "-N", "-s",
                   (sshRequested ? "/bin/sh" : "/bin/nologin"), "-e",
                   (enabled ? "" : "1970-01-02"));
    }
    catch (const InternalFailure &e)
    {
        log<level::ERR>("Unable to create new user");
        elog<InternalFailure>();
    }

    // Add the users object before sending out the signal
    std::string userObj = std::string(usersObjPath) + "/" + userName;
    std::sort(groupNames.begin(), groupNames.end());
    usersList.emplace(
        userName, std::move(std::make_unique<phosphor::user::Users>(
                      bus, userObj.c_str(), groupNames, priv, enabled, *this)));

    log<level::INFO>("User created successfully",
                     entry("USER_NAME=%s", userName.c_str()));
    return;
}

void UserMgr::deleteUser(std::string userName)
{
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();
    throwForUserDoesNotExist(userName);
    try
    {
        executeCmd("/usr/sbin/userdel", userName.c_str(), "-r");
    }
    catch (const InternalFailure &e)
    {
        log<level::ERR>("User delete failed",
                        entry("USER_NAME=%s", userName.c_str()));
        elog<InternalFailure>();
    }

    usersList.erase(userName);

    log<level::INFO>("User deleted successfully",
                     entry("USER_NAME=%s", userName.c_str()));
    return;
}

void UserMgr::renameUser(std::string userName, std::string newUserName)
{
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();
    throwForUserDoesNotExist(userName);
    throwForUserExists(newUserName);
    throwForUserNameConstraints(newUserName,
                                usersList[userName].get()->userGroups());
    try
    {
        std::string newHomeDir = "/home/" + newUserName;
        executeCmd("/usr/sbin/usermod", "-l", newUserName.c_str(),
                   userName.c_str(), "-d", newHomeDir.c_str(), "-m");
    }
    catch (const InternalFailure &e)
    {
        log<level::ERR>("User rename failed",
                        entry("USER_NAME=%s", userName.c_str()));
        elog<InternalFailure>();
    }
    const auto &user = usersList[userName];
    std::string priv = user.get()->userPrivilege();
    std::vector<std::string> groupNames = user.get()->userGroups();
    bool enabled = user.get()->userEnabled();
    std::string newUserObj = std::string(usersObjPath) + "/" + newUserName;
    // Special group 'ipmi' needs a way to identify user renamed, in order to
    // update encrypted password. It can't rely only on InterfacesRemoved &
    // InterfacesAdded. So first send out userRenamed signal.
    this->userRenamed(userName, newUserName);
    usersList.erase(userName);
    usersList.emplace(
        newUserName,
        std::move(std::make_unique<phosphor::user::Users>(
            bus, newUserObj.c_str(), groupNames, priv, enabled, *this)));
    return;
}

void UserMgr::updateGroupsAndPriv(const std::string &userName,
                                  const std::vector<std::string> &groupNames,
                                  const std::string &priv)
{
    throwForInvalidPrivilege(priv);
    throwForInvalidGroups(groupNames);
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();
    throwForUserDoesNotExist(userName);
    const std::vector<std::string> &oldGroupNames =
        usersList[userName].get()->userGroups();
    std::vector<std::string> groupDiff;
    // Note: already dealing with sorted group lists.
    std::set_symmetric_difference(oldGroupNames.begin(), oldGroupNames.end(),
                                  groupNames.begin(), groupNames.end(),
                                  std::back_inserter(groupDiff));
    if (std::find(groupDiff.begin(), groupDiff.end(), "ipmi") !=
        groupDiff.end())
    {
        throwForUserNameConstraints(userName, groupNames);
        throwForMaxGrpUserCount(groupNames);
    }

    std::string groups = getCSVFromVector(groupNames);
    bool sshRequested = removeStringFromCSV(groups, grpSsh);

    // treat privilege as a group - This is to avoid using different file to
    // store the same.
    if (!priv.empty())
    {
        if (groups.size() != 0)
        {
            groups += ",";
        }
        groups += priv;
    }
    try
    {
        executeCmd("/usr/sbin/usermod", userName.c_str(), "-G", groups.c_str(),
                   "-s", (sshRequested ? "/bin/sh" : "/bin/nologin"));
    }
    catch (const InternalFailure &e)
    {
        log<level::ERR>("Unable to modify user privilege / groups");
        elog<InternalFailure>();
    }

    log<level::INFO>("User groups / privilege updated successfully",
                     entry("USER_NAME=%s", userName.c_str()));
    return;
}

uint8_t UserMgr::minPasswordLength(uint8_t value)
{
    if (value == AccountPolicyIface::minPasswordLength())
    {
        return value;
    }
    if (value < minPasswdLength)
    {
        return value;
    }
    if (setPamModuleArgValue(pamCrackLib, minPasswdLenProp,
                             std::to_string(value)) != success)
    {
        log<level::ERR>("Unable to set minPasswordLength");
        elog<InternalFailure>();
    }
    return AccountPolicyIface::minPasswordLength(value);
}

uint8_t UserMgr::rememberOldPasswordTimes(uint8_t value)
{
    if (value == AccountPolicyIface::rememberOldPasswordTimes())
    {
        return value;
    }
    if (setPamModuleArgValue(pamPWHistory, remOldPasswdCount,
                             std::to_string(value)) != success)
    {
        log<level::ERR>("Unable to set rememberOldPasswordTimes");
        elog<InternalFailure>();
    }
    return AccountPolicyIface::rememberOldPasswordTimes(value);
}

uint16_t UserMgr::maxLoginAttemptBeforeLockout(uint16_t value)
{
    if (value == AccountPolicyIface::maxLoginAttemptBeforeLockout())
    {
        return value;
    }
    if (setPamModuleArgValue(pamTally2, maxFailedAttempt,
                             std::to_string(value)) != success)
    {
        log<level::ERR>("Unable to set maxLoginAttemptBeforeLockout");
        elog<InternalFailure>();
    }
    return AccountPolicyIface::maxLoginAttemptBeforeLockout(value);
}

uint32_t UserMgr::accountUnlockTimeout(uint32_t value)
{
    if (value == AccountPolicyIface::accountUnlockTimeout())
    {
        return value;
    }
    if (setPamModuleArgValue(pamTally2, unlockTimeout, std::to_string(value)) !=
        success)
    {
        log<level::ERR>("Unable to set accountUnlockTimeout");
        elog<InternalFailure>();
    }
    return AccountPolicyIface::accountUnlockTimeout(value);
}

int UserMgr::getPamModuleArgValue(const std::string &moduleName,
                                  const std::string &argName,
                                  std::string &argValue)
{
    std::string fileName;
    if (moduleName == pamTally2)
    {
        fileName = pamAuthConfigFile;
    }
    else
    {
        fileName = pamPasswdConfigFile;
    }
    std::ifstream fileToRead(fileName, std::ios::in);
    if (!fileToRead.is_open())
    {
        log<level::ERR>("Failed to open pam configuration file",
                        entry("FILE_NAME=%s", fileName.c_str()));
        return failure;
    }
    std::string line;
    auto argSearch = argName + "=";
    size_t startPos = 0;
    size_t endPos = 0;
    while (getline(fileToRead, line))
    {
        // skip comments section starting with #
        if ((startPos = line.find('#')) != std::string::npos)
        {
            if (startPos == 0)
            {
                continue;
            }
            // skip comments after meaningful section and process those
            line = line.substr(0, startPos);
        }
        if (line.find(moduleName) != std::string::npos)
        {
            if ((startPos = line.find(argSearch)) != std::string::npos)
            {
                if ((endPos = line.find(' ', startPos)) == std::string::npos)
                {
                    endPos = line.size();
                }
                startPos += argSearch.size();
                argValue = line.substr(startPos, endPos - startPos);
                return success;
            }
        }
    }
    return failure;
}

int UserMgr::setPamModuleArgValue(const std::string &moduleName,
                                  const std::string &argName,
                                  const std::string &argValue)
{
    std::string fileName;
    if (moduleName == pamTally2)
    {
        fileName = pamAuthConfigFile;
    }
    else
    {
        fileName = pamPasswdConfigFile;
    }
    std::string tmpFileName = fileName + "_tmp";
    std::ifstream fileToRead(fileName, std::ios::in);
    std::ofstream fileToWrite(tmpFileName, std::ios::out);
    if (!fileToRead.is_open() || !fileToWrite.is_open())
    {
        log<level::ERR>("Failed to open pam configuration /tmp file",
                        entry("FILE_NAME=%s", fileName.c_str()));
        return failure;
    }
    std::string line;
    auto argSearch = argName + "=";
    size_t startPos = 0;
    size_t endPos = 0;
    bool found = false;
    while (getline(fileToRead, line))
    {
        // skip comments section starting with #
        if ((startPos = line.find('#')) != std::string::npos)
        {
            if (startPos == 0)
            {
                fileToWrite << line << std::endl;
                continue;
            }
            // skip comments after meaningful section and process those
            line = line.substr(0, startPos);
        }
        if (line.find(moduleName) != std::string::npos)
        {
            if ((startPos = line.find(argSearch)) != std::string::npos)
            {
                if ((endPos = line.find(' ', startPos)) == std::string::npos)
                {
                    endPos = line.size();
                }
                startPos += argSearch.size();
                fileToWrite << line.substr(0, startPos) << argValue
                            << line.substr(endPos, line.size() - endPos)
                            << std::endl;
                found = true;
                continue;
            }
        }
        fileToWrite << line << std::endl;
    }
    fileToWrite.close();
    fileToRead.close();
    if (found)
    {
        if (std::rename(tmpFileName.c_str(), fileName.c_str()) == 0)
        {
            return success;
        }
    }
    return failure;
}

void UserMgr::userEnable(const std::string &userName, bool enabled)
{
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();
    throwForUserDoesNotExist(userName);
    try
    {
        executeCmd("/usr/sbin/usermod", userName.c_str(), "-e",
                   (enabled ? "" : "1970-01-02"));
    }
    catch (const InternalFailure &e)
    {
        log<level::ERR>("Unable to modify user enabled state");
        elog<InternalFailure>();
    }

    log<level::INFO>("User enabled/disabled state updated successfully",
                     entry("USER_NAME=%s", userName.c_str()),
                     entry("ENABLED=%d", enabled));
    return;
}

/**
 * pam_tally2 app will provide the user failure count and failure status
 * in second line of output with words position [0] - user name,
 * [1] - failure count, [2] - latest timestamp, [3] - failure timestamp
 * [4] - failure app
 **/

static constexpr size_t t2UserIdx = 0;
static constexpr size_t t2FailCntIdx = 1;
static constexpr size_t t2OutputIndex = 1;

bool UserMgr::userLockedForFailedAttempt(const std::string &userName)
{
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();
    std::vector<std::string> output;

    output = executeCmd("/usr/sbin/pam_tally2", "-u", userName.c_str());

    std::vector<std::string> splitWords;
    boost::algorithm::split(splitWords, output[t2OutputIndex],
                            boost::algorithm::is_any_of("\t "),
                            boost::token_compress_on);

    try
    {
        unsigned long tmp = std::stoul(splitWords[t2FailCntIdx], nullptr);
        uint16_t value16 = 0;
        if (tmp > std::numeric_limits<decltype(value16)>::max())
        {
            throw std::out_of_range("Out of range");
        }
        value16 = static_cast<decltype(value16)>(tmp);
        if (AccountPolicyIface::maxLoginAttemptBeforeLockout() != 0 &&
            value16 >= AccountPolicyIface::maxLoginAttemptBeforeLockout())
        {
            return true; // User account is locked out
        }
        return false; // User account is un-locked
    }
    catch (const std::exception &e)
    {
        log<level::ERR>("Exception for userLockedForFailedAttempt",
                        entry("WHAT=%s", e.what()));
        throw;
    }
}

bool UserMgr::userLockedForFailedAttempt(const std::string &userName,
                                         const bool &value)
{
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();
    std::vector<std::string> output;
    if (value == true)
    {
        return userLockedForFailedAttempt(userName);
    }
    output = executeCmd("/usr/sbin/pam_tally2", "-u", userName.c_str(), "-r");

    std::vector<std::string> splitWords;
    boost::algorithm::split(splitWords, output[t2OutputIndex],
                            boost::algorithm::is_any_of("\t "),
                            boost::token_compress_on);

    return userLockedForFailedAttempt(userName);
}

bool UserMgr::userPasswordExpired(const std::string &userName)
{
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();

    struct spwd spwd
    {
    };
    struct spwd *spwdPtr = nullptr;
    auto buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (buflen < -1)
    {
        // Use a default size if there is no hard limit suggested by sysconf()
        buflen = 1024;
    }
    std::vector<char> buffer(buflen);
    auto status =
        getspnam_r(userName.c_str(), &spwd, buffer.data(), buflen, &spwdPtr);
    // On success, getspnam_r() returns zero, and sets *spwdPtr to spwd.
    // If no matching password record was found, these functions return 0
    // and store NULL in *spwdPtr
    if ((status == 0) && (&spwd == spwdPtr))
    {
        // Determine password validity per "chage" docs, where:
        //   spwd.sp_lstchg == 0 means password is expired, and
        //   spwd.sp_max == -1 means the password does not expire.
        constexpr long seconds_per_day = 60 * 60 * 24;
        long today = static_cast<long>(time(NULL)) / seconds_per_day;
        if ((spwd.sp_lstchg == 0) ||
            ((spwd.sp_max != -1) && ((spwd.sp_max + spwd.sp_lstchg) < today)))
        {
            return true;
        }
    }
    else
    {
        log<level::ERR>("User does not exist",
                        entry("USER_NAME=%s", userName.c_str()));
        elog<UserNameDoesNotExist>();
    }

    return false;
}

UserSSHLists UserMgr::getUserAndSshGrpList()
{
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();

    std::vector<std::string> userList;
    std::vector<std::string> sshUsersList;
    struct passwd pw, *pwp = nullptr;
    std::array<char, 1024> buffer{};

    phosphor::user::File passwd(passwdFileName, "r");
    if ((passwd)() == NULL)
    {
        log<level::ERR>("Error opening the passwd file");
        elog<InternalFailure>();
    }

    while (true)
    {
        auto r = fgetpwent_r((passwd)(), &pw, buffer.data(), buffer.max_size(),
                             &pwp);
        if ((r != 0) || (pwp == NULL))
        {
            // Any error, break the loop.
            break;
        }
#ifdef ENABLE_ROOT_USER_MGMT
        // Add all users whose UID >= 1000 and < 65534
        // and special UID 0.
        if ((pwp->pw_uid == 0) ||
            ((pwp->pw_uid >= 1000) && (pwp->pw_uid < 65534)))
#else
        // Add all users whose UID >=1000 and < 65534
        if ((pwp->pw_uid >= 1000) && (pwp->pw_uid < 65534))
#endif
        {
            std::string userName(pwp->pw_name);
            userList.emplace_back(userName);

            // ssh doesn't have separate group. Check login shell entry to
            // get all users list which are member of ssh group.
            std::string loginShell(pwp->pw_shell);
            if (loginShell == "/bin/sh")
            {
                sshUsersList.emplace_back(userName);
            }
        }
    }
    endpwent();
    return std::make_pair(std::move(userList), std::move(sshUsersList));
}

size_t UserMgr::getIpmiUsersCount()
{
    std::vector<std::string> userList = getUsersInGroup("ipmi");
    return userList.size();
}

bool UserMgr::isUserEnabled(const std::string &userName)
{
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();
    std::array<char, 4096> buffer{};
    struct spwd spwd;
    struct spwd *resultPtr = nullptr;
    int status = getspnam_r(userName.c_str(), &spwd, buffer.data(),
                            buffer.max_size(), &resultPtr);
    if (!status && (&spwd == resultPtr))
    {
        if (resultPtr->sp_expire >= 0)
        {
            return false; // user locked out
        }
        return true;
    }
    return false; // assume user is disabled for any error.
}

std::vector<std::string> UserMgr::getUsersInGroup(const std::string &groupName)
{
    std::vector<std::string> usersInGroup;
    // Should be more than enough to get the pwd structure.
    std::array<char, 4096> buffer{};
    struct group grp;
    struct group *resultPtr = nullptr;

    int status = getgrnam_r(groupName.c_str(), &grp, buffer.data(),
                            buffer.max_size(), &resultPtr);

    if (!status && (&grp == resultPtr))
    {
        for (; *(grp.gr_mem) != NULL; ++(grp.gr_mem))
        {
            usersInGroup.emplace_back(*(grp.gr_mem));
        }
    }
    else
    {
        log<level::ERR>("Group not found",
                        entry("GROUP=%s", groupName.c_str()));
        // Don't throw error, just return empty userList - fallback
    }
    return usersInGroup;
}

DbusUserObj UserMgr::getPrivilegeMapperObject(void)
{
    DbusUserObj objects;
    try
    {
        std::string basePath = "/xyz/openbmc_project/user/ldap/openldap";
        std::string interface = "xyz.openbmc_project.User.Ldap.Config";

        auto ldapMgmtService =
            getServiceName(std::move(basePath), std::move(interface));
        auto method = bus.new_method_call(
            ldapMgmtService.c_str(), ldapMgrObjBasePath,
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");

        auto reply = bus.call(method);
        reply.read(objects);
    }
    catch (const InternalFailure &e)
    {
        log<level::ERR>("Unable to get the User Service",
                        entry("WHAT=%s", e.what()));
        throw;
    }
    catch (const sdbusplus::exception::SdBusError &e)
    {
        log<level::ERR>(
            "Failed to excute method", entry("METHOD=%s", "GetManagedObjects"),
            entry("PATH=%s", ldapMgrObjBasePath), entry("WHAT=%s", e.what()));
        throw;
    }
    return objects;
}

std::string UserMgr::getLdapGroupName(const std::string &userName)
{
    struct passwd pwd
    {
    };
    struct passwd *pwdPtr = nullptr;
    auto buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (buflen < -1)
    {
        // Use a default size if there is no hard limit suggested by sysconf()
        buflen = 1024;
    }
    std::vector<char> buffer(buflen);
    gid_t gid = 0;

    auto status =
        getpwnam_r(userName.c_str(), &pwd, buffer.data(), buflen, &pwdPtr);
    // On success, getpwnam_r() returns zero, and set *pwdPtr to pwd.
    // If no matching password record was found, these functions return 0
    // and store NULL in *pwdPtr
    if (!status && (&pwd == pwdPtr))
    {
        gid = pwd.pw_gid;
    }
    else
    {
        log<level::ERR>("User does not exist",
                        entry("USER_NAME=%s", userName.c_str()));
        elog<UserNameDoesNotExist>();
    }

    struct group *groups = nullptr;
    std::string ldapGroupName;

    while ((groups = getgrent()) != NULL)
    {
        if (groups->gr_gid == gid)
        {
            ldapGroupName = groups->gr_name;
            break;
        }
    }
    // Call endgrent() to close the group database.
    endgrent();

    return ldapGroupName;
}

std::string UserMgr::getServiceName(std::string &&path, std::string &&intf)
{
    auto mapperCall = bus.new_method_call(objMapperService, objMapperPath,
                                          objMapperInterface, "GetObject");

    mapperCall.append(std::move(path));
    mapperCall.append(std::vector<std::string>({std::move(intf)}));

    auto mapperResponseMsg = bus.call(mapperCall);

    if (mapperResponseMsg.is_method_error())
    {
        log<level::ERR>("Error in mapper call");
        elog<InternalFailure>();
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);

    if (mapperResponse.begin() == mapperResponse.end())
    {
        log<level::ERR>("Invalid response from mapper");
        elog<InternalFailure>();
    }

    return mapperResponse.begin()->first;
}

UserInfoMap UserMgr::getUserInfo(std::string userName)
{
    UserInfoMap userInfo;
    // Check whether the given user is local user or not.
    if (isUserExist(userName) == true)
    {
        const auto &user = usersList[userName];
        userInfo.emplace("UserPrivilege", user.get()->userPrivilege());
        userInfo.emplace("UserGroups", user.get()->userGroups());
        userInfo.emplace("UserEnabled", user.get()->userEnabled());
        userInfo.emplace("UserLockedForFailedAttempt",
                         user.get()->userLockedForFailedAttempt());
        userInfo.emplace("UserPasswordExpired",
                         user.get()->userPasswordExpired());
        userInfo.emplace("RemoteUser", false);
    }
    else
    {
        std::string ldapGroupName = getLdapGroupName(userName);
        if (ldapGroupName.empty())
        {
            log<level::ERR>("Unable to get group name",
                            entry("USER_NAME=%s", userName.c_str()));
            elog<InternalFailure>();
        }

        DbusUserObj objects = getPrivilegeMapperObject();

        std::string privilege;
        std::string groupName;
        std::string ldapConfigPath;

        try
        {
            for (const auto &obj : objects)
            {
                for (const auto &interface : obj.second)
                {
                    if ((interface.first ==
                         "xyz.openbmc_project.Object.Enable"))
                    {
                        for (const auto &property : interface.second)
                        {
                            auto value = std::get<bool>(property.second);
                            if ((property.first == "Enabled") &&
                                (value == true))
                            {
                                ldapConfigPath = obj.first;
                                break;
                            }
                        }
                    }
                }
                if (!ldapConfigPath.empty())
                {
                    break;
                }
            }

            if (ldapConfigPath.empty())
            {
                return userInfo;
            }

            for (const auto &obj : objects)
            {
                for (const auto &interface : obj.second)
                {
                    if ((interface.first ==
                         "xyz.openbmc_project.User.PrivilegeMapperEntry") &&
                        (obj.first.str.find(ldapConfigPath) !=
                         std::string::npos))
                    {

                        for (const auto &property : interface.second)
                        {
                            auto value = std::get<std::string>(property.second);
                            if (property.first == "GroupName")
                            {
                                groupName = value;
                            }
                            else if (property.first == "Privilege")
                            {
                                privilege = value;
                            }
                            if (groupName == ldapGroupName)
                            {
                                userInfo["UserPrivilege"] = privilege;
                            }
                        }
                    }
                }
            }
            auto priv = std::get<std::string>(userInfo["UserPrivilege"]);

            if (priv.empty())
            {
                log<level::ERR>("LDAP group privilege mapping does not exist");
            }
        }
        catch (const std::bad_variant_access &e)
        {
            log<level::ERR>("Error while accessing variant",
                            entry("WHAT=%s", e.what()));
            elog<InternalFailure>();
        }
        userInfo.emplace("RemoteUser", true);
    }

    return userInfo;
}

void UserMgr::initUserObjects(void)
{
    // All user management lock has to be based on /etc/shadow
    phosphor::user::shadow::Lock lock();
    std::vector<std::string> userNameList;
    std::vector<std::string> sshGrpUsersList;
    UserSSHLists userSSHLists = getUserAndSshGrpList();
    userNameList = std::move(userSSHLists.first);
    sshGrpUsersList = std::move(userSSHLists.second);

    if (!userNameList.empty())
    {
        std::map<std::string, std::vector<std::string>> groupLists;
        for (auto &grp : groupsMgr)
        {
            if (grp == grpSsh)
            {
                groupLists.emplace(grp, sshGrpUsersList);
            }
            else
            {
                std::vector<std::string> grpUsersList = getUsersInGroup(grp);
                groupLists.emplace(grp, grpUsersList);
            }
        }
        for (auto &grp : privMgr)
        {
            std::vector<std::string> grpUsersList = getUsersInGroup(grp);
            groupLists.emplace(grp, grpUsersList);
        }

        for (auto &user : userNameList)
        {
            std::vector<std::string> userGroups;
            std::string userPriv;
            for (const auto &grp : groupLists)
            {
                std::vector<std::string> tempGrp = grp.second;
                if (std::find(tempGrp.begin(), tempGrp.end(), user) !=
                    tempGrp.end())
                {
                    if (std::find(privMgr.begin(), privMgr.end(), grp.first) !=
                        privMgr.end())
                    {
                        userPriv = grp.first;
                    }
                    else
                    {
                        userGroups.emplace_back(grp.first);
                    }
                }
            }
            // Add user objects to the Users path.
            auto objPath = std::string(usersObjPath) + "/" + user;
            std::sort(userGroups.begin(), userGroups.end());
            usersList.emplace(user,
                              std::move(std::make_unique<phosphor::user::Users>(
                                  bus, objPath.c_str(), userGroups, userPriv,
                                  isUserEnabled(user), *this)));
        }
    }
}

UserMgr::UserMgr(sdbusplus::bus::bus &bus, const char *path) :
    Ifaces(bus, path, true), bus(bus), path(path)
{
    UserMgrIface::allPrivileges(privMgr);
    std::sort(groupsMgr.begin(), groupsMgr.end());
    UserMgrIface::allGroups(groupsMgr);
    std::string valueStr;
    auto value = minPasswdLength;
    unsigned long tmp = 0;
    if (getPamModuleArgValue(pamCrackLib, minPasswdLenProp, valueStr) !=
        success)
    {
        AccountPolicyIface::minPasswordLength(minPasswdLength);
    }
    else
    {
        try
        {
            tmp = std::stoul(valueStr, nullptr);
            if (tmp > std::numeric_limits<decltype(value)>::max())
            {
                throw std::out_of_range("Out of range");
            }
            value = static_cast<decltype(value)>(tmp);
        }
        catch (const std::exception &e)
        {
            log<level::ERR>("Exception for MinPasswordLength",
                            entry("WHAT=%s", e.what()));
            throw;
        }
        AccountPolicyIface::minPasswordLength(value);
    }
    valueStr.clear();
    if (getPamModuleArgValue(pamPWHistory, remOldPasswdCount, valueStr) !=
        success)
    {
        AccountPolicyIface::rememberOldPasswordTimes(0);
    }
    else
    {
        value = 0;
        try
        {
            tmp = std::stoul(valueStr, nullptr);
            if (tmp > std::numeric_limits<decltype(value)>::max())
            {
                throw std::out_of_range("Out of range");
            }
            value = static_cast<decltype(value)>(tmp);
        }
        catch (const std::exception &e)
        {
            log<level::ERR>("Exception for RememberOldPasswordTimes",
                            entry("WHAT=%s", e.what()));
            throw;
        }
        AccountPolicyIface::rememberOldPasswordTimes(value);
    }
    valueStr.clear();
    if (getPamModuleArgValue(pamTally2, maxFailedAttempt, valueStr) != success)
    {
        AccountPolicyIface::maxLoginAttemptBeforeLockout(0);
    }
    else
    {
        uint16_t value16 = 0;
        try
        {
            tmp = std::stoul(valueStr, nullptr);
            if (tmp > std::numeric_limits<decltype(value16)>::max())
            {
                throw std::out_of_range("Out of range");
            }
            value16 = static_cast<decltype(value16)>(tmp);
        }
        catch (const std::exception &e)
        {
            log<level::ERR>("Exception for MaxLoginAttemptBeforLockout",
                            entry("WHAT=%s", e.what()));
            throw;
        }
        AccountPolicyIface::maxLoginAttemptBeforeLockout(value16);
    }
    valueStr.clear();
    if (getPamModuleArgValue(pamTally2, unlockTimeout, valueStr) != success)
    {
        AccountPolicyIface::accountUnlockTimeout(0);
    }
    else
    {
        uint32_t value32 = 0;
        try
        {
            tmp = std::stoul(valueStr, nullptr);
            if (tmp > std::numeric_limits<decltype(value32)>::max())
            {
                throw std::out_of_range("Out of range");
            }
            value32 = static_cast<decltype(value32)>(tmp);
        }
        catch (const std::exception &e)
        {
            log<level::ERR>("Exception for AccountUnlockTimeout",
                            entry("WHAT=%s", e.what()));
            throw;
        }
        AccountPolicyIface::accountUnlockTimeout(value32);
    }
    initUserObjects();

    // emit the signal
    this->emit_object_added();
}

} // namespace user
} // namespace phosphor
