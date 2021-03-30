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

#include "apphandler.hpp"
#include "channel_layer.hpp"

#include <ipmid/api.hpp>
#include <phosphor-logging/log.hpp>
#include <regex>

using namespace phosphor::logging;

namespace ipmi
{

/** @brief implements the set channel access command
 *  @ param ctx - context pointer
 *  @ param channel - channel number
 *  @ param reserved - skip 4 bits
 *  @ param accessMode - access mode for IPMI messaging
 *  @ param usrAuth - user level authentication (enable/disable)
 *  @ param msgAuth - per message authentication (enable/disable)
 *  @ param alertDisabled - PEF alerting (enable/disable)
 *  @ param chanAccess - channel access
 *  @ param channelPrivLimit - channel privilege limit
 *  @ param reserved - skip 3 bits
 *  @ param channelPrivMode - channel priviledge mode
 *
 *  @ returns IPMI completion code
 **/
RspType<> ipmiSetChannelAccess(Context::ptr ctx, uint4_t channel,
                               uint4_t reserved1, uint3_t accessMode,
                               bool usrAuth, bool msgAuth, bool alertDisabled,
                               uint2_t chanAccess, uint4_t channelPrivLimit,
                               uint2_t reserved2, uint2_t channelPrivMode)
{
    if (reserved1 || reserved2 ||
        !isValidPrivLimit(static_cast<uint8_t>(channelPrivLimit)))
    {
        log<level::DEBUG>("Set channel access - Invalid field in request");
        return responseInvalidFieldRequest();
    }

    const uint8_t chNum =
        convertCurrentChannelNum(static_cast<uint8_t>(channel), ctx->channel);
    if ((getChannelSessionSupport(chNum) == EChannelSessSupported::none) ||
        (!isValidChannel(chNum)))
    {
        log<level::DEBUG>("Set channel access - No support on channel");
        return response(ccActionNotSupportedForChannel);
    }

    ChannelAccess chActData;
    ChannelAccess chNVData;
    uint8_t setActFlag = 0;
    uint8_t setNVFlag = 0;
    Cc compCode;

    // cannot static cast directly from uint2_t to enum; must go via int
    uint8_t channelAccessAction = static_cast<uint8_t>(chanAccess);
    switch (static_cast<EChannelActionType>(channelAccessAction))
    {
        case doNotSet:
            break;
        case nvData:
            chNVData.accessMode = static_cast<uint8_t>(accessMode);
            chNVData.userAuthDisabled = usrAuth;
            chNVData.perMsgAuthDisabled = msgAuth;
            chNVData.alertingDisabled = alertDisabled;
            setNVFlag |= (setAccessMode | setUserAuthEnabled |
                          setMsgAuthEnabled | setAlertingEnabled);
            break;

        case activeData:
            chActData.accessMode = static_cast<uint8_t>(accessMode);
            chActData.userAuthDisabled = usrAuth;
            chActData.perMsgAuthDisabled = msgAuth;
            chActData.alertingDisabled = alertDisabled;
            setActFlag |= (setAccessMode | setUserAuthEnabled |
                           setMsgAuthEnabled | setAlertingEnabled);
            break;

        case reserved:
        default:
            log<level::DEBUG>("Set channel access - Invalid access set mode");
            return response(ccAccessModeNotSupportedForChannel);
    }

    // cannot static cast directly from uint2_t to enum; must go via int
    uint8_t channelPrivAction = static_cast<uint8_t>(channelPrivMode);
    switch (static_cast<EChannelActionType>(channelPrivAction))
    {
        case doNotSet:
            break;
        case nvData:
            chNVData.privLimit = static_cast<uint8_t>(channelPrivLimit);
            setNVFlag |= setPrivLimit;
            break;
        case activeData:
            chActData.privLimit = static_cast<uint8_t>(channelPrivLimit);

            setActFlag |= setPrivLimit;
            break;
        case reserved:
        default:
            log<level::DEBUG>("Set channel access - Invalid access priv mode");
            return response(ccAccessModeNotSupportedForChannel);
    }

    if (setNVFlag != 0)
    {
        compCode = setChannelAccessPersistData(chNum, chNVData, setNVFlag);
        if (compCode != ccSuccess)
        {
            log<level::DEBUG>("Set channel access - Failed to set access data");
            return response(compCode);
        }
    }

    if (setActFlag != 0)
    {
        compCode = setChannelAccessData(chNum, chActData, setActFlag);
        if (compCode != ccSuccess)
        {
            log<level::DEBUG>("Set channel access - Failed to set access data");
            return response(compCode);
        }
    }

    return responseSuccess();
}

/** @brief implements the get channel access command
 *  @ param ctx - context pointer
 *  @ param channel - channel number
 *  @ param reserved1 - skip 4 bits
 *  @ param reserved2 - skip 6 bits
 *  @ param accessMode - get access mode
 *
 *  @returns ipmi completion code plus response data
 *  - accessMode - get access mode
 *  - usrAuthDisabled - user level authentication status
 *  - msgAuthDisabled - message level authentication status
 *  - alertDisabled - alerting status
 *  - reserved - skip 2 bits
 *  - privLimit - channel privilege limit
 *  - reserved - skip 4 bits
 * */
ipmi ::RspType<uint3_t, // access mode,
               bool,    // user authentication status,
               bool,    // message authentication status,
               bool,    // alerting status,
               uint2_t, // reserved,

               uint4_t, // channel privilege,
               uint4_t  // reserved
               >
    ipmiGetChannelAccess(Context::ptr ctx, uint4_t channel, uint4_t reserved1,
                         uint6_t reserved2, uint2_t accessSetMode)
{
    if (reserved1 || reserved2)
    {
        log<level::DEBUG>("Get channel access - Invalid field in request");
        return responseInvalidFieldRequest();
    }

    if ((accessSetMode == doNotSet) || (accessSetMode == reserved))
    {
        log<level::DEBUG>("Get channel access - Invalid Access mode");
        return responseInvalidFieldRequest();
    }

    const uint8_t chNum =
        convertCurrentChannelNum(static_cast<uint8_t>(channel), ctx->channel);

    if ((getChannelSessionSupport(chNum) == EChannelSessSupported::none) ||
        (!isValidChannel(chNum)))
    {
        log<level::DEBUG>("Get channel access - No support on channel");
        return response(ccActionNotSupportedForChannel);
    }

    ChannelAccess chAccess;

    Cc compCode;

    if (accessSetMode == nvData)
    {
        compCode = getChannelAccessPersistData(chNum, chAccess);
    }
    else if (accessSetMode == activeData)
    {
        compCode = getChannelAccessData(chNum, chAccess);
    }

    if (compCode != ccSuccess)
    {
        return response(compCode);
    }

    constexpr uint2_t reservedOut1 = 0;
    constexpr uint4_t reservedOut2 = 0;

    return responseSuccess(
        static_cast<uint3_t>(chAccess.accessMode), chAccess.userAuthDisabled,
        chAccess.perMsgAuthDisabled, chAccess.alertingDisabled, reservedOut1,
        static_cast<uint4_t>(chAccess.privLimit), reservedOut2);
}

/** @brief implements the get channel info command
 *  @ param ctx - context pointer
 *  @ param channel - channel number
 *  @ param reserved - skip 4 bits
 *
 *  @returns ipmi completion code plus response data
 *  - chNum - the channel number for this request
 *  - mediumType - see Table 6-3, Channel Medium Type Numbers
 *  - protocolType - Table 6-2, Channel Protocol Type Numbers
 *  - activeSessionCount - number of active sessions
 *  - sessionType - channel support for sessions
 *  - vendorId - vendor for this channel protocol (IPMI - 7154)
 *  - auxChInfo - auxiliary info for channel
 * */
RspType<uint4_t,  // chNum
        uint4_t,  // reserved
        uint7_t,  // mediumType
        bool,     // reserved
        uint5_t,  // protocolType
        uint3_t,  // reserved
        uint6_t,  // activeSessionCount
        uint2_t,  // sessionType
        uint24_t, // Vendor IANA
        uint16_t  // aux info
        >
    ipmiGetChannelInfo(Context::ptr ctx, uint4_t channel, uint4_t reserved)
{
    if (reserved)
    {
        log<level::DEBUG>("Get channel access - Invalid field in request");
        return responseInvalidFieldRequest();
    }

    uint8_t chNum =
        convertCurrentChannelNum(static_cast<uint8_t>(channel), ctx->channel);
    if (!isValidChannel(chNum))
    {
        log<level::DEBUG>("Get channel Info - No support on channel");
        return responseInvalidFieldRequest();
    }

    ChannelInfo chInfo;
    Cc compCode = getChannelInfo(chNum, chInfo);
    if (compCode != ccSuccess)
    {
        log<level::ERR>("Failed to get channel info",
                        entry("CHANNEL=%x", chNum),
                        entry("ERRNO=%x", compCode));
        return response(compCode);
    }

    constexpr uint4_t reserved1 = 0;
    constexpr bool reserved2 = false;
    constexpr uint3_t reserved3 = 0;
    uint8_t mediumType = chInfo.mediumType;
    uint8_t protocolType = chInfo.protocolType;
    uint2_t sessionType = chInfo.sessionSupported;
    uint6_t activeSessionCount = getChannelActiveSessions(chNum);
    // IPMI Spec: The IPMI Enterprise Number is: 7154 (decimal)
    constexpr uint24_t vendorId = 7154;
    constexpr uint16_t auxChInfo = 0;

    return responseSuccess(chNum, reserved1, mediumType, reserved2,
                           protocolType, reserved3, activeSessionCount,
                           sessionType, vendorId, auxChInfo);
}

namespace
{
constexpr uint16_t standardPayloadBit(PayloadType p)
{
    return (1 << static_cast<size_t>(p));
}

constexpr uint16_t sessionPayloadBit(PayloadType p)
{
    constexpr size_t sessionShift =
        static_cast<size_t>(PayloadType::OPEN_SESSION_REQUEST);
    return ((1 << static_cast<size_t>(p)) >> sessionShift);
}
} // namespace

/** @brief implements get channel payload support command
 *  @ param ctx - ipmi context pointer
 *  @ param chNum - channel number
 *  @ param reserved - skip 4 bits
 *
 *  @ returns IPMI completion code plus response data
 *  - stdPayloadType - bitmask of supported standard payload types
 *  - sessSetupPayloadType - bitmask of supported session setup payload types
 *  - OEMPayloadType - bitmask of supported OEM payload types
 *  - reserved - 2 bytes of 0
 **/
RspType<uint16_t, // stdPayloadType
        uint16_t, // sessSetupPayloadType
        uint16_t, // OEMPayloadType
        uint16_t  // reserved
        >
    ipmiGetChannelPayloadSupport(Context::ptr ctx, uint4_t channel,
                                 uint4_t reserved)
{
    uint8_t chNum =
        convertCurrentChannelNum(static_cast<uint8_t>(channel), ctx->channel);

    if (!doesDeviceExist(chNum) || !isValidChannel(chNum) || reserved)
    {
        log<level::DEBUG>("Get channel payload - Invalid field in request");
        return responseInvalidFieldRequest();
    }

    // Session support is available in active LAN channels.
    if (getChannelSessionSupport(chNum) == EChannelSessSupported::none)
    {
        log<level::DEBUG>("Get channel payload - No support on channel");
        return response(ccActionNotSupportedForChannel);
    }
    constexpr uint16_t stdPayloadType = standardPayloadBit(PayloadType::IPMI) |
                                        standardPayloadBit(PayloadType::SOL);
    constexpr uint16_t sessSetupPayloadType =
        sessionPayloadBit(PayloadType::OPEN_SESSION_REQUEST) |
        sessionPayloadBit(PayloadType::OPEN_SESSION_RESPONSE) |
        sessionPayloadBit(PayloadType::RAKP1) |
        sessionPayloadBit(PayloadType::RAKP2) |
        sessionPayloadBit(PayloadType::RAKP3) |
        sessionPayloadBit(PayloadType::RAKP4);
    constexpr uint16_t OEMPayloadType = 0;
    constexpr uint16_t rspRsvd = 0;
    return responseSuccess(stdPayloadType, sessSetupPayloadType, OEMPayloadType,
                           rspRsvd);
}

/** @brief implements the get channel payload version command
 *  @param ctx - IPMI context pointer (for channel)
 *  @param chNum - channel number to get info about
 *  @param reserved - skip 4 bits
 *  @param payloadTypeNum - to get payload type info

 *  @returns IPMI completion code plus response data
 *   - formatVersion - BCD encoded format version info
 */

RspType<uint8_t> // formatVersion
    ipmiGetChannelPayloadVersion(Context::ptr ctx, uint4_t chNum,
                                 uint4_t reserved, uint8_t payloadTypeNum)
{
    uint8_t channel =
        convertCurrentChannelNum(static_cast<uint8_t>(chNum), ctx->channel);

    if (reserved || !isValidChannel(channel))
    {
        log<level::DEBUG>(
            "Get channel payload version - Invalid field in request");
        return responseInvalidFieldRequest();
    }

    if (getChannelSessionSupport(channel) == EChannelSessSupported::none)
    {
        log<level::DEBUG>(
            "Get channel payload version - No support on channel");
        return response(ccActionNotSupportedForChannel);
    }

    if (!isValidPayloadType(static_cast<PayloadType>(payloadTypeNum)))
    {
        log<level::ERR>(
            "Get channel payload version - Payload type unavailable");

        constexpr uint8_t payloadTypeNotSupported = 0x80;
        return response(payloadTypeNotSupported);
    }

    // BCD encoded version representation - 1.0
    constexpr uint8_t formatVersion = 0x10;

    return responseSuccess(formatVersion);
}

void registerChannelFunctions() __attribute__((constructor));
void registerChannelFunctions()
{
    ipmiChannelInit();

    registerHandler(prioOpenBmcBase, netFnApp, app::cmdSetChannelAccess,
                    Privilege::Admin, ipmiSetChannelAccess);

    registerHandler(prioOpenBmcBase, netFnApp, app::cmdGetChannelAccess,
                    Privilege::User, ipmiGetChannelAccess);

    registerHandler(prioOpenBmcBase, netFnApp, app::cmdGetChannelInfoCommand,
                    Privilege::User, ipmiGetChannelInfo);

    registerHandler(prioOpenBmcBase, netFnApp, app::cmdGetChannelPayloadSupport,
                    Privilege::User, ipmiGetChannelPayloadSupport);

    registerHandler(prioOpenBmcBase, netFnApp, app::cmdGetChannelPayloadVersion,
                    Privilege::User, ipmiGetChannelPayloadVersion);
}

} // namespace ipmi
