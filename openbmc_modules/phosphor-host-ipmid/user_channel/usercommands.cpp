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

#include "usercommands.hpp"

#include "apphandler.hpp"
#include "channel_layer.hpp"
#include "user_layer.hpp"

#include <security/pam_appl.h>

#include <ipmid/api.hpp>
#include <phosphor-logging/log.hpp>
#include <regex>

namespace ipmi
{

using namespace phosphor::logging;

static constexpr uint8_t disableUser = 0x00;
static constexpr uint8_t enableUser = 0x01;
static constexpr uint8_t setPassword = 0x02;
static constexpr uint8_t testPassword = 0x03;
static constexpr uint8_t passwordKeySize20 = 1;
static constexpr uint8_t passwordKeySize16 = 0;
static constexpr uint8_t enableOperation = 0x00;
static constexpr uint8_t disableOperation = 0x01;

/** @struct SetUserNameReq
 *
 *  Structure for set user name request command (refer spec sec 22.28)
 */
struct SetUserNameReq
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t userId : 6;
    uint8_t reserved1 : 2;
#endif
#if BYTE_ORDER == BIG_ENDIAN
    uint8_t reserved1 : 2;
    uint8_t userId : 6;
#endif
    uint8_t userName[16];
} __attribute__((packed));

/** @struct GetUserNameReq
 *
 *  Structure for get user name request command (refer spec sec 22.29)
 */
struct GetUserNameReq
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t userId : 6;
    uint8_t reserved1 : 2;
#endif
#if BYTE_ORDER == BIG_ENDIAN
    uint8_t reserved1 : 2;
    uint8_t userId : 6;
#endif
} __attribute__((packed));

/** @struct GetUserNameResp
 *
 *  Structure for get user name response command (refer spec sec 22.29)
 */
struct GetUserNameResp
{
    uint8_t userName[16];
} __attribute__((packed));

/** @struct SetUserPasswordReq
 *
 *  Structure for set user password request command (refer spec sec 22.30)
 */
struct SetUserPasswordReq
{
#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t userId : 6;
    uint8_t reserved1 : 1;
    uint8_t ipmi20 : 1;
    uint8_t operation : 2;
    uint8_t reserved2 : 6;
#endif
#if BYTE_ORDER == BIG_ENDIAN
    uint8_t ipmi20 : 1;
    uint8_t reserved1 : 1;
    uint8_t userId : 6;
    uint8_t reserved2 : 6;
    uint8_t operation : 2;
#endif
    uint8_t userPassword[maxIpmi20PasswordSize];
} __attribute__((packed));

/** @brief implements the set user access command
 *  @param ctx - IPMI context pointer (for channel)
 *  @param channel - channel number
 *  @param ipmiEnabled - indicates ipmi messaging state
 *  @param linkAuthEnabled - indicates link authentication state
 *  @param accessCallback - indicates callback state
 *  @param bitsUpdate - indicates update request
 *  @param userId - user id
 *  @param reserved1 - skip 2 bits
 *  @param privilege - user privilege
 *  @param reserved2 - skip 4 bits
 *  @param sessionLimit - optional - unused for now
 *
 *  @returns ipmi completion code
 */
ipmi::RspType<> ipmiSetUserAccess(ipmi::Context::ptr ctx, uint4_t channel,
                                  uint1_t ipmiEnabled, uint1_t linkAuthEnabled,
                                  uint1_t accessCallback, uint1_t bitsUpdate,

                                  uint6_t userId, uint2_t reserved1,

                                  uint4_t privilege, uint4_t reserved2,

                                  std::optional<uint8_t> sessionLimit)
{
    uint8_t sessLimit = sessionLimit.value_or(0);
    if (reserved1 || reserved2 || sessLimit ||
        !ipmiUserIsValidPrivilege(static_cast<uint8_t>(privilege)))
    {
        log<level::DEBUG>("Set user access - Invalid field in request");
        return ipmi::responseInvalidFieldRequest();
    }

    uint8_t chNum =
        convertCurrentChannelNum(static_cast<uint8_t>(channel), ctx->channel);
    if (!isValidChannel(chNum))
    {
        log<level::DEBUG>("Set user access - Invalid channel request");
        return ipmi::response(invalidChannel);
    }
    if (getChannelSessionSupport(chNum) == EChannelSessSupported::none)
    {
        log<level::DEBUG>("Set user access - No support on channel");
        return ipmi::response(ccActionNotSupportedForChannel);
    }
    if (!ipmiUserIsValidUserId(static_cast<uint8_t>(userId)))
    {
        log<level::DEBUG>("Set user access - Parameter out of range");
        return ipmi::responseParmOutOfRange();
    }

    PrivAccess privAccess = {0};
    if (bitsUpdate)
    {
        privAccess.ipmiEnabled = static_cast<uint8_t>(ipmiEnabled);
        privAccess.linkAuthEnabled = static_cast<uint8_t>(linkAuthEnabled);
        privAccess.accessCallback = static_cast<uint8_t>(accessCallback);
    }
    privAccess.privilege = static_cast<uint8_t>(privilege);
    return ipmi::response(
        ipmiUserSetPrivilegeAccess(static_cast<uint8_t>(userId), chNum,
                                   privAccess, static_cast<bool>(bitsUpdate)));
}

/** @brief implements the set user access command
 *  @param ctx - IPMI context pointer (for channel)
 *  @param channel - channel number
 *  @param reserved1 - skip 4 bits
 *  @param userId - user id
 *  @param reserved2 - skip 2 bits
 *
 *  @returns ipmi completion code plus response data
 *   - maxChUsers - max channel users
 *   - reserved1 - skip 2 bits
 *   - enabledUsers - enabled users count
 *   - enabledStatus - enabled status
 *   - fixedUsers - fixed users count
 *   - reserved2 - skip 2 bits
 *   - privilege - user privilege
 *   - ipmiEnabled - ipmi messaging state
 *   - linkAuthEnabled - link authenticatin state
 *   - accessCallback - callback state
 *   - reserved - skip 1 bit
 */
ipmi::RspType<uint6_t, // max channel users
              uint2_t, // reserved1

              uint6_t, // enabled users count
              uint2_t, // enabled status

              uint6_t, // fixed users count
              uint2_t, // reserved2

              uint4_t, // privilege
              uint1_t, // ipmi messaging state
              uint1_t, // link authentication state
              uint1_t, // access callback state
              uint1_t  // reserved3
              >
    ipmiGetUserAccess(ipmi::Context::ptr ctx, uint4_t channel,
                      uint4_t reserved1,

                      uint6_t userId, uint2_t reserved2)
{
    uint8_t chNum =
        convertCurrentChannelNum(static_cast<uint8_t>(channel), ctx->channel);

    if (reserved1 || reserved2 || !isValidChannel(chNum))
    {
        log<level::DEBUG>("Get user access - Invalid field in request");
        return ipmi::responseInvalidFieldRequest();
    }

    if (getChannelSessionSupport(chNum) == EChannelSessSupported::none)
    {
        log<level::DEBUG>("Get user access - No support on channel");
        return ipmi::response(ccActionNotSupportedForChannel);
    }
    if (!ipmiUserIsValidUserId(static_cast<uint8_t>(userId)))
    {
        log<level::DEBUG>("Get user access - Parameter out of range");
        return ipmi::responseParmOutOfRange();
    }

    uint8_t maxChUsers = 0, enabledUsers = 0, fixedUsers = 0;
    ipmi::Cc retStatus;
    retStatus = ipmiUserGetAllCounts(maxChUsers, enabledUsers, fixedUsers);
    if (retStatus != ccSuccess)
    {
        return ipmi::response(retStatus);
    }

    bool enabledState = false;
    retStatus =
        ipmiUserCheckEnabled(static_cast<uint8_t>(userId), enabledState);
    if (retStatus != ccSuccess)
    {
        return ipmi::response(retStatus);
    }

    uint2_t enabledStatus = enabledState ? userIdEnabledViaSetPassword
                                         : userIdDisabledViaSetPassword;
    PrivAccess privAccess{};
    retStatus = ipmiUserGetPrivilegeAccess(static_cast<uint8_t>(userId), chNum,
                                           privAccess);
    if (retStatus != ccSuccess)
    {
        return ipmi::response(retStatus);
    }
    constexpr uint2_t res2Bits = 0;
    return ipmi::responseSuccess(
        static_cast<uint6_t>(maxChUsers), res2Bits,

        static_cast<uint6_t>(enabledUsers), enabledStatus,

        static_cast<uint6_t>(fixedUsers), res2Bits,

        static_cast<uint4_t>(privAccess.privilege),
        static_cast<uint1_t>(privAccess.ipmiEnabled),
        static_cast<uint1_t>(privAccess.linkAuthEnabled),
        static_cast<uint1_t>(privAccess.accessCallback),
        static_cast<uint1_t>(privAccess.reserved));
}

Cc ipmiSetUserName(ipmi_netfn_t netfn, ipmi_cmd_t cmd, ipmi_request_t request,
                   ipmi_response_t response, ipmi_data_len_t dataLen,
                   ipmi_context_t context)
{
    const SetUserNameReq* req = static_cast<SetUserNameReq*>(request);
    size_t reqLength = *dataLen;
    *dataLen = 0;

    if (reqLength != sizeof(*req))
    {
        log<level::DEBUG>("Set user name - Invalid Length");
        return ccReqDataLenInvalid;
    }
    if (req->reserved1)
    {
        return ccInvalidFieldRequest;
    }
    if (!ipmiUserIsValidUserId(req->userId))
    {
        log<level::DEBUG>("Set user name - Invalid user id");
        return ccParmOutOfRange;
    }

    return ipmiUserSetUserName(req->userId,
                               reinterpret_cast<const char*>(req->userName));
}

/** @brief implementes the get user name command
 *  @param[in] netfn - specifies netfn.
 *  @param[in] cmd   - specifies cmd number.
 *  @param[in] request - pointer to request data.
 *  @param[in, out] dataLen - specifies request data length, and returns
 * response data length.
 *  @param[in] context - ipmi context.
 *  @returns ipmi completion code.
 */
Cc ipmiGetUserName(ipmi_netfn_t netfn, ipmi_cmd_t cmd, ipmi_request_t request,
                   ipmi_response_t response, ipmi_data_len_t dataLen,
                   ipmi_context_t context)
{
    const GetUserNameReq* req = static_cast<GetUserNameReq*>(request);
    size_t reqLength = *dataLen;

    *dataLen = 0;

    if (reqLength != sizeof(*req))
    {
        log<level::DEBUG>("Get user name - Invalid Length");
        return ccReqDataLenInvalid;
    }

    std::string userName;
    if (ipmiUserGetUserName(req->userId, userName) != ccSuccess)
    { // Invalid User ID
        log<level::DEBUG>("User Name not found",
                          entry("USER-ID=%d", (uint8_t)req->userId));
        return ccParmOutOfRange;
    }
    GetUserNameResp* resp = static_cast<GetUserNameResp*>(response);
    std::fill(reinterpret_cast<uint8_t*>(resp),
              reinterpret_cast<uint8_t*>(resp) + sizeof(*resp), 0);
    userName.copy(reinterpret_cast<char*>(resp->userName),
                  sizeof(resp->userName), 0);
    *dataLen = sizeof(*resp);

    return ccSuccess;
}

/** @brief implementes the set user password command
 *  @param[in] netfn - specifies netfn.
 *  @param[in] cmd   - specifies cmd number.
 *  @param[in] request - pointer to request data.
 *  @param[in, out] dataLen - specifies request data length, and returns
 * response data length.
 *  @param[in] context - ipmi context.
 *  @returns ipmi completion code.
 */
Cc ipmiSetUserPassword(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                       ipmi_request_t request, ipmi_response_t response,
                       ipmi_data_len_t dataLen, ipmi_context_t context)
{
    const SetUserPasswordReq* req = static_cast<SetUserPasswordReq*>(request);
    size_t reqLength = *dataLen;
    // subtract 2 bytes header to know the password length - including NULL
    uint8_t passwordLength = *dataLen - 2;
    *dataLen = 0;

    // verify input length based on operation. Required password size is 20
    // bytes as  we support only IPMI 2.0, but in order to be compatible with
    // tools, accept 16 bytes of password size too.
    if (reqLength < 2 ||
        // If enable / disable user, reqLength has to be >=2 & <= 22
        ((req->operation == disableUser || req->operation == enableUser) &&
         ((reqLength < 2) || (reqLength > sizeof(SetUserPasswordReq)))))
    {
        log<level::DEBUG>("Invalid Length");
        return ccReqDataLenInvalid;
    }
    // If set / test password then password length has to be 16 or 20 bytes
    // based on the password size bit.
    if (((req->operation == setPassword) || (req->operation == testPassword)) &&
        (((req->ipmi20 == passwordKeySize20) &&
          (passwordLength != maxIpmi20PasswordSize)) ||
         ((req->ipmi20 == passwordKeySize16) &&
          (passwordLength != maxIpmi15PasswordSize))))
    {
        log<level::DEBUG>("Invalid Length");
        return ccReqDataLenInvalid;
    }

    std::string userName;
    if (ipmiUserGetUserName(req->userId, userName) != ccSuccess)
    {
        log<level::DEBUG>("User Name not found",
                          entry("USER-ID=%d", (uint8_t)req->userId));
        return ccParmOutOfRange;
    }
    if (req->operation == setPassword)
    {
        return ipmiUserSetUserPassword(
            req->userId, reinterpret_cast<const char*>(req->userPassword));
    }
    else if (req->operation == enableUser || req->operation == disableUser)
    {
        return ipmiUserUpdateEnabledState(req->userId,
                                          static_cast<bool>(req->operation));
    }
    else if (req->operation == testPassword)
    {
        auto password = ipmiUserGetPassword(userName);
        std::string testPassword(
            reinterpret_cast<const char*>(req->userPassword), 0,
            passwordLength);
        // Note: For security reasons password size won't be compared and
        // wrong password size completion code will not be returned if size
        // doesn't match as specified in IPMI specification.
        if (password != testPassword)
        {
            log<level::DEBUG>("Test password failed",
                              entry("USER-ID=%d", (uint8_t)req->userId));
            return static_cast<Cc>(
                IPMISetPasswordReturnCodes::ipmiCCPasswdFailMismatch);
        }
        return ccSuccess;
    }
    return ccInvalidFieldRequest;
}

/** @brief implements the get channel authentication command
 *  @param ctx - IPMI context pointer (for channel)
 *  @param extData - get IPMI 2.0 extended data
 *  @param reserved1 - skip 3 bits
 *  @param chNum - channel number to get info about
 *  @param reserved2 - skip 4 bits
 *  @param privLevel - requested privilege level

 *  @returns ipmi completion code plus response data
 *   - channel number
 *   - rmcpAuthTypes - RMCP auth types (IPMI 1.5)
 *   - reserved1
 *   - extDataSupport - true for IPMI 2.0 extensions
 *   - anonymousLogin - true for anonymous login enabled
 *   - nullUsers - true for null user names enabled
 *   - nonNullUsers - true for non-null usernames enabled
 *   - userAuth - false for user authentication enabled
 *   - perMessageAuth - false for per message authentication enabled
 *   - KGStatus - true for Kg required for authentication
 *   - reserved2
 *   - rmcp - RMCP (IPMI 1.5) connection support
 *   - rmcpp - RMCP+ (IPMI 2.0) connection support
 *   - reserved3
 *   - oemID - OEM IANA of any OEM auth support
 *   - oemAuxillary - OEM data for auth
 */
ipmi::RspType<uint8_t,  // channel number
              uint6_t,  // rmcpAuthTypes
              bool,     // reserved1
              bool,     // extDataSupport
              bool,     // anonymousLogin
              bool,     // nullUsers
              bool,     // nonNullUsers
              bool,     // userAuth
              bool,     // perMessageAuth
              bool,     // KGStatus
              uint2_t,  // reserved2
              bool,     // rmcp
              bool,     // rmcpp
              uint6_t,  // reserved3
              uint24_t, // oemID
              uint8_t   // oemAuxillary
              >
    ipmiGetChannelAuthenticationCapabilities(ipmi::Context::ptr ctx,
                                             uint4_t chNum, uint3_t reserved1,
                                             bool extData, uint4_t privLevel,
                                             uint4_t reserved2)
{
    uint8_t channel =
        convertCurrentChannelNum(static_cast<uint8_t>(chNum), ctx->channel);

    if (reserved1 || reserved2 || !isValidChannel(channel) ||
        !isValidPrivLimit(static_cast<uint8_t>(privLevel)))
    {
        log<level::DEBUG>(
            "Get channel auth capabilities - Invalid field in request");
        return ipmi::responseInvalidFieldRequest();
    }

    if (getChannelSessionSupport(channel) == EChannelSessSupported::none)
    {
        log<level::DEBUG>(
            "Get channel auth capabilities - No support on channel");
        return ipmi::response(ccActionNotSupportedForChannel);
    }

    constexpr bool extDataSupport = true; // true for IPMI 2.0 extensions
    constexpr bool reserved3 = false;
    constexpr uint6_t rmcpAuthTypes = 0; // IPMI 1.5 auth types - not supported
    constexpr uint2_t reserved4 = 0;
    constexpr bool KGStatus = false;       // Not supporting now.
    constexpr bool perMessageAuth = false; // Per message auth - enabled
    constexpr bool userAuth = false;       // User authentication - enabled
    constexpr bool nullUsers = false;      // Null user names - not supported
    constexpr bool anonymousLogin = false; // Anonymous login - not supported
    constexpr uint6_t reserved5 = 0;
    constexpr bool rmcpp = true; // IPMI 2.0 - supported
    constexpr bool rmcp = false; // IPMI 1.5 - not supported
    constexpr uint24_t oemID = 0;
    constexpr uint8_t oemAuxillary = 0;

    bool nonNullUsers = 0;
    uint8_t maxChUsers = 0, enabledUsers = 0, fixedUsers = 0;
    ipmi::ipmiUserGetAllCounts(maxChUsers, enabledUsers, fixedUsers);
    nonNullUsers = enabledUsers > 0;

    return ipmi::responseSuccess(
        channel, rmcpAuthTypes, reserved3, extDataSupport, anonymousLogin,
        nullUsers, nonNullUsers, userAuth, perMessageAuth, KGStatus, reserved4,
        rmcp, rmcpp, reserved5, oemID, oemAuxillary);
}

/** @brief implements the set user payload access command.
 *  @param ctx - IPMI context pointer (for channel)
 *  @param channel - channel number (4 bits)
 *  @param reserved1 - skip 4 bits
 *  @param userId - user id (6 bits)
 *  @param operation - access ENABLE /DISABLE. (2 bits)
 *  @param stdPayload0 - IPMI - reserved. (1 bit)
 *  @param stdPayload1 - SOL.             (1 bit)
 *  @param stdPayload2 -                  (1 bit)
 *  @param stdPayload3 -                  (1 bit)
 *  @param stdPayload4 -                  (1 bit)
 *  @param stdPayload5 -                  (1 bit)
 *  @param stdPayload6 -                  (1 bit)
 *  @param stdPayload7 -                  (1 bit)
 *  @param stdPayloadEnables2Reserved -   (8 bits)
 *  @param oemPayload0 -                  (1 bit)
 *  @param oemPayload1 -                  (1 bit)
 *  @param oemPayload2 -                  (1 bit)
 *  @param oemPayload3 -                  (1 bit)
 *  @param oemPayload4 -                  (1 bit)
 *  @param oemPayload5 -                  (1 bit)
 *  @param oemPayload6 -                  (1 bit)
 *  @param oemPayload7 -                  (1 bit)
 *  @param oemPayloadEnables2Reserved -   (8 bits)
 *
 *  @returns IPMI completion code
 */
ipmi::RspType<> ipmiSetUserPayloadAccess(
    ipmi::Context::ptr ctx,

    uint4_t channel, uint4_t reserved,

    uint6_t userId, uint2_t operation,

    bool stdPayload0ipmiReserved, bool stdPayload1SOL, bool stdPayload2,
    bool stdPayload3, bool stdPayload4, bool stdPayload5, bool stdPayload6,
    bool stdPayload7,

    uint8_t stdPayloadEnables2Reserved,

    bool oemPayload0, bool oemPayload1, bool oemPayload2, bool oemPayload3,
    bool oemPayload4, bool oemPayload5, bool oemPayload6, bool oemPayload7,

    uint8_t oemPayloadEnables2Reserved)
{
    auto chNum =
        convertCurrentChannelNum(static_cast<uint8_t>(channel), ctx->channel);
    // Validate the reserved args. Only SOL payload is supported as on date.
    if (reserved || stdPayload0ipmiReserved || stdPayload2 || stdPayload3 ||
        stdPayload4 || stdPayload5 || stdPayload6 || stdPayload7 ||
        oemPayload0 || oemPayload1 || oemPayload2 || oemPayload3 ||
        oemPayload4 || oemPayload5 || oemPayload6 || oemPayload7 ||
        stdPayloadEnables2Reserved || oemPayloadEnables2Reserved ||
        !isValidChannel(chNum))
    {
        return ipmi::responseInvalidFieldRequest();
    }

    if ((operation != enableOperation && operation != disableOperation))
    {
        return ipmi::responseInvalidFieldRequest();
    }
    if (getChannelSessionSupport(chNum) == EChannelSessSupported::none)
    {
        return ipmi::response(ccActionNotSupportedForChannel);
    }
    if (!ipmiUserIsValidUserId(static_cast<uint8_t>(userId)))
    {
        return ipmi::responseParmOutOfRange();
    }

    PayloadAccess payloadAccess = {0};
    payloadAccess.stdPayloadEnables1[1] = stdPayload1SOL;

    return ipmi::response(ipmiUserSetUserPayloadAccess(
        chNum, static_cast<uint8_t>(operation), static_cast<uint8_t>(userId),
        payloadAccess));
}

/** @brief implements the get user payload access command
 *  This command returns information about user payload enable settings
 *  that were set using the 'Set User Payload Access' Command.
 *
 *  @param ctx - IPMI context pointer (for channel)
 *  @param channel - channel number
 *  @param reserved1 - skip 4 bits
 *  @param userId - user id
 *  @param reserved2 - skip 2 bits
 *
 *  @returns IPMI completion code plus response data
 *   - stdPayload0ipmiReserved - IPMI payload (reserved).
 *   - stdPayload1SOL - SOL payload
 *   - stdPayload2
 *   - stdPayload3
 *   - stdPayload4
 *   - stdPayload5
 *   - stdPayload6
 *   - stdPayload7

 *   - stdPayloadEnables2Reserved - Reserved.

 *   - oemPayload0
 *   - oemPayload1
 *   - oemPayload2
 *   - oemPayload3
 *   - oemPayload4
 *   - oemPayload5
 *   - oemPayload6
 *   - oemPayload7

 *  - oemPayloadEnables2Reserved - Reserved
 */
ipmi::RspType<bool, // stdPayload0ipmiReserved
              bool, // stdPayload1SOL
              bool, // stdPayload2
              bool, // stdPayload3
              bool, // stdPayload4
              bool, // stdPayload5
              bool, // stdPayload6
              bool, // stdPayload7

              uint8_t, // stdPayloadEnables2Reserved

              bool, // oemPayload0
              bool, // oemPayload1
              bool, // oemPayload2
              bool, // oemPayload3
              bool, // oemPayload4
              bool, // oemPayload5
              bool, // oemPayload6
              bool, // oemPayload7

              uint8_t // oemPayloadEnables2Reserved
              >
    ipmiGetUserPayloadAccess(ipmi::Context::ptr ctx,

                             uint4_t channel, uint4_t reserved1,

                             uint6_t userId, uint2_t reserved2)
{
    uint8_t chNum =
        convertCurrentChannelNum(static_cast<uint8_t>(channel), ctx->channel);

    if (reserved1 || reserved2 || !isValidChannel(chNum))
    {
        return ipmi::responseInvalidFieldRequest();
    }
    if (getChannelSessionSupport(chNum) == EChannelSessSupported::none)
    {
        return ipmi::response(ccActionNotSupportedForChannel);
    }
    if (!ipmiUserIsValidUserId(static_cast<uint8_t>(userId)))
    {
        return ipmi::responseParmOutOfRange();
    }

    ipmi::Cc retStatus;
    PayloadAccess payloadAccess = {};
    retStatus = ipmiUserGetUserPayloadAccess(
        chNum, static_cast<uint8_t>(userId), payloadAccess);
    if (retStatus != ccSuccess)
    {
        return ipmi::response(retStatus);
    }
    constexpr uint8_t res8bits = 0;
    return ipmi::responseSuccess(payloadAccess.stdPayloadEnables1.test(0),
                                 payloadAccess.stdPayloadEnables1.test(1),
                                 payloadAccess.stdPayloadEnables1.test(2),
                                 payloadAccess.stdPayloadEnables1.test(3),
                                 payloadAccess.stdPayloadEnables1.test(4),
                                 payloadAccess.stdPayloadEnables1.test(5),
                                 payloadAccess.stdPayloadEnables1.test(6),
                                 payloadAccess.stdPayloadEnables1.test(7),

                                 res8bits,

                                 payloadAccess.oemPayloadEnables1.test(0),
                                 payloadAccess.oemPayloadEnables1.test(1),
                                 payloadAccess.oemPayloadEnables1.test(2),
                                 payloadAccess.oemPayloadEnables1.test(3),
                                 payloadAccess.oemPayloadEnables1.test(4),
                                 payloadAccess.oemPayloadEnables1.test(5),
                                 payloadAccess.oemPayloadEnables1.test(6),
                                 payloadAccess.oemPayloadEnables1.test(7),

                                 res8bits);
}

void registerUserIpmiFunctions() __attribute__((constructor));
void registerUserIpmiFunctions()
{
    post_work([]() { ipmiUserInit(); });
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnApp,
                          ipmi::app::cmdSetUserAccessCommand,
                          ipmi::Privilege::Admin, ipmiSetUserAccess);

    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnApp,
                          ipmi::app::cmdGetUserAccessCommand,
                          ipmi::Privilege::Operator, ipmiGetUserAccess);

    ipmi_register_callback(NETFUN_APP, IPMI_CMD_GET_USER_NAME, NULL,
                           ipmiGetUserName, PRIVILEGE_OPERATOR);

    ipmi_register_callback(NETFUN_APP, IPMI_CMD_SET_USER_NAME, NULL,
                           ipmiSetUserName, PRIVILEGE_ADMIN);

    ipmi_register_callback(NETFUN_APP, IPMI_CMD_SET_USER_PASSWORD, NULL,
                           ipmiSetUserPassword, PRIVILEGE_ADMIN);

    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnApp,
                          ipmi::app::cmdGetChannelAuthCapabilities,
                          ipmi::Privilege::Callback,
                          ipmiGetChannelAuthenticationCapabilities);

    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnApp,
                          ipmi::app::cmdSetUserPayloadAccess,
                          ipmi::Privilege::Admin, ipmiSetUserPayloadAccess);

    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnApp,
                          ipmi::app::cmdGetUserPayloadAccess,
                          ipmi::Privilege::Operator, ipmiGetUserPayloadAccess);

    return;
}
} // namespace ipmi
