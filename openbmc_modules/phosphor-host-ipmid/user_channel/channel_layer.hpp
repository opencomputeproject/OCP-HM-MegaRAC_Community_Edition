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
#include <array>
#include <ipmid/api.hpp>
#include <string>

namespace ipmi
{

static constexpr uint8_t maxIpmiChannels = 16;
static constexpr uint8_t currentChNum = 0xE;
static constexpr uint8_t invalidChannel = 0xff;
static constexpr const uint8_t ccActionNotSupportedForChannel = 0x82;
static constexpr const uint8_t ccAccessModeNotSupportedForChannel = 0x83;

/**
 * @array of privilege levels
 */
extern const std::array<std::string, PRIVILEGE_OEM + 1> privList;

/**
 * @enum Channel Protocol Type (refer spec sec 6.4)
 */
enum class EChannelProtocolType : uint8_t
{
    na = 0x00,
    ipmbV10 = 0x01,
    icmbV11 = 0x02,
    reserved = 0x03,
    ipmiSmbus = 0x04,
    kcs = 0x05,
    smic = 0x06,
    bt10 = 0x07,
    bt15 = 0x08,
    tMode = 0x09,
    oem = 0x1C,
};

/**
 * @enum Channel Medium Type (refer spec sec 6.5)
 */
enum class EChannelMediumType : uint8_t
{
    reserved = 0x00,
    ipmb = 0x01,
    icmbV10 = 0x02,
    icmbV09 = 0x03,
    lan8032 = 0x04,
    serial = 0x05,
    otherLan = 0x06,
    pciSmbus = 0x07,
    smbusV11 = 0x08,
    smbusV20 = 0x09,
    usbV1x = 0x0A,
    usbV2x = 0x0B,
    systemInterface = 0x0C,
    oem = 0x60,
    unknown = 0x82,
};

/**
 * @enum Channel Session Type (refer spec sec 22.24 -
 * response data byte 5)
 */
enum class EChannelSessSupported : uint8_t
{
    none = 0,
    single = 1,
    multi = 2,
    any = 3,
};

/**
 * @enum Channel Access Mode (refer spec sec 6.6)
 */
enum class EChannelAccessMode : uint8_t
{
    disabled = 0,
    preboot = 1,
    alwaysAvail = 2,
    shared = 3,
};

/**
 * @enum Authentication Types (refer spec sec 13.6 - IPMI
 * Session Header)
 */
enum class EAuthType : uint8_t
{
    none = (1 << 0x0),
    md2 = (1 << 0x1),
    md5 = (1 << 0x2),
    reserved = (1 << 0x3),
    straightPasswd = (1 << 0x4),
    oem = (1 << 0x5),
};

// TODO: Remove duplicate 'PayloadType' definition from netipmid's message.hpp
// to phosphor-ipmi-host/include
/**
 * @enum Payload Types (refer spec sec 13.27.3)
 */
enum class PayloadType : uint8_t
{
    IPMI = 0x00,
    SOL = 0x01,
    OPEN_SESSION_REQUEST = 0x10,
    OPEN_SESSION_RESPONSE = 0x11,
    RAKP1 = 0x12,
    RAKP2 = 0x13,
    RAKP3 = 0x14,
    RAKP4 = 0x15,
    INVALID = 0xFF,
};

/**
 * @enum Access mode for channel access set/get (refer spec
 * sec 22.22 - request byte 2[7:6])
 */
typedef enum
{
    doNotSet = 0x00,
    nvData = 0x01,
    activeData = 0x02,
    reserved = 0x03,
} EChannelActionType;

/**
 * @enum Access set flag to determine changes that has to be updated
 * in channel access data configuration.
 */
enum AccessSetFlag
{
    setAccessMode = (1 << 0),
    setUserAuthEnabled = (1 << 1),
    setMsgAuthEnabled = (1 << 2),
    setAlertingEnabled = (1 << 3),
    setPrivLimit = (1 << 4),
};

/** @struct ChannelAccess
 *
 *  Structure to store channel access related information, defined in IPMI
 * specification and used in Get / Set channel access (refer spec sec 22.22
 * & 22.23)
 */
struct ChannelAccess
{
    uint8_t accessMode;
    bool userAuthDisabled;
    bool perMsgAuthDisabled;
    bool alertingDisabled;
    uint8_t privLimit;
};

/** @struct ChannelInfo
 *
 *  Structure to store data about channel information, which identifies each
 *  channel type and information as defined in IPMI specification. (refer spec
 * sec 22.22 & 22.23)
 */
struct ChannelInfo
{
    uint8_t mediumType;
    uint8_t protocolType;
    uint8_t sessionSupported;
    bool isIpmi; // Is session IPMI
    // This is used in Get LAN Configuration parameter.
    // This holds the supported AuthTypes for a given channel.
    uint8_t authTypeSupported;
};

/** @brief determines valid channel
 *
 *  @param[in] chNum- channel number
 *
 *  @return true if valid, false otherwise
 */
bool isValidChannel(const uint8_t chNum);

/** @brief determines whether channel device exist
 *
 *  @param[in] chNum - channel number
 *
 *  @return true if valid, false otherwise
 */
bool doesDeviceExist(const uint8_t chNum);

/** @brief determines whether privilege limit is valid
 *
 *  @param[in] privLimit - Privilege limit
 *
 *  @return true if valid, false otherwise
 */
bool isValidPrivLimit(const uint8_t privLimit);

/** @brief determines whether access mode  is valid
 *
 *  @param[in] accessMode - Access mode
 *
 *  @return true if valid, false otherwise
 */
bool isValidAccessMode(const uint8_t accessMode);

/** @brief determines valid authentication type based on channel number
 *
 *  @param[in] chNum - channel number
 *  @param[in] authType - authentication type
 *
 *  @return true if valid, false otherwise
 */
bool isValidAuthType(const uint8_t chNum, const EAuthType& authType);

/** @brief determines supported session type of a channel
 *
 *  @param[in] chNum - channel number
 *
 *  @return EChannelSessSupported - supported session type
 */
EChannelSessSupported getChannelSessionSupport(const uint8_t chNum);

/** @brief determines number of active sessions on a channel
 *
 *  @param[in] chNum - channel number
 *
 *  @return numer of active sessions
 */
int getChannelActiveSessions(const uint8_t chNum);

/** @brief determines maximum transfer size for a channel
 *
 *  @param[in] chNum - channel number
 *
 *  @return maximum bytes that can be transferred on this channel
 */
size_t getChannelMaxTransferSize(uint8_t chNum);

/** @brief initializes channel management
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc ipmiChannelInit();

/** @brief provides channel info details
 *
 *  @param[in] chNum - channel number
 *  @param[out] chInfo - channel info details
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc getChannelInfo(const uint8_t chNum, ChannelInfo& chInfo);

/** @brief provides channel access data
 *
 *  @param[in] chNum - channel number
 *  @param[out] chAccessData -channel access data
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc getChannelAccessData(const uint8_t chNum, ChannelAccess& chAccessData);

/** @brief provides function to convert current channel number (0xE)
 *
 *  @param[in] chNum - channel number as requested in commands.
 *  @param[in] devChannel - channel number as provided by device (not 0xE)
 *
 *  @return same channel number or proper channel number for current channel
 * number (0xE).
 */
static inline uint8_t convertCurrentChannelNum(const uint8_t chNum,
                                               const uint8_t devChannel)
{
    if (chNum == currentChNum)
    {
        return devChannel;
    }
    return chNum;
}

/** @brief to set channel access data
 *
 *  @param[in] chNum - channel number
 *  @param[in] chAccessData - channel access data
 *  @param[in] setFlag - flag to indicate updatable fields
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc setChannelAccessData(const uint8_t chNum, const ChannelAccess& chAccessData,
                        const uint8_t setFlag);

/** @brief to get channel access data persistent data
 *
 *  @param[in] chNum - channel number
 *  @param[out] chAccessData - channel access data
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc getChannelAccessPersistData(const uint8_t chNum,
                               ChannelAccess& chAccessData);

/** @brief to set channel access data persistent data
 *
 *  @param[in] chNum - channel number
 *  @param[in] chAccessData - channel access data
 *  @param[in] setFlag - flag to indicate updatable fields
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc setChannelAccessPersistData(const uint8_t chNum,
                               const ChannelAccess& chAccessData,
                               const uint8_t setFlag);

/** @brief provides supported authentication type for the channel
 *
 *  @param[in] chNum - channel number
 *  @param[out] authTypeSupported - supported authentication type
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc getChannelAuthTypeSupported(const uint8_t chNum, uint8_t& authTypeSupported);

/** @brief provides enabled authentication type for the channel
 *
 *  @param[in] chNum - channel number
 *  @param[in] priv - privilege
 *  @param[out] authType - enabled authentication type
 *
 *  @return ccSuccess for success, others for failure.
 */
Cc getChannelEnabledAuthType(const uint8_t chNum, const uint8_t priv,
                             EAuthType& authType);

/** @brief Retrieves the LAN channel name from the IPMI channel number
 *
 *  @param[in] chNum - IPMI channel number
 *
 *  @return the LAN channel name (i.e. eth0)
 */
std::string getChannelName(const uint8_t chNum);

/** @brief Retrieves the LAN channel number from the IPMI channel name
 *
 *  @param[in] chName - IPMI channel name (i.e. eth0)
 *
 *  @return the LAN channel number
 */
uint8_t getChannelByName(const std::string& chName);

/** @brief determines whether payload type is valid
 *
 *	@param[in] payload type - Payload Type
 *
 *	@return true if valid, false otherwise
 */
bool isValidPayloadType(const PayloadType payloadType);

} // namespace ipmi
