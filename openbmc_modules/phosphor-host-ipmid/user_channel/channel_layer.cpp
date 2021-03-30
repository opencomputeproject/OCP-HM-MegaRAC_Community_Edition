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

#include "channel_layer.hpp"

#include "channel_mgmt.hpp"
#include "cipher_mgmt.hpp"

#include <phosphor-logging/log.hpp>

namespace ipmi
{

bool doesDeviceExist(const uint8_t chNum)
{
    // TODO: This is not the reliable way to find the device
    // associated with ethernet interface as the channel number to
    // eth association is not done. Need to revisit later
    struct stat fileStat = {0};
    std::string devName("/sys/class/net/" + getChannelName(chNum));

    if (stat(devName.data(), &fileStat) != 0)
    {
        phosphor::logging::log<phosphor::logging::level::DEBUG>(
            "Ethernet device not found");
        return false;
    }

    return true;
}

bool isValidPrivLimit(const uint8_t privLimit)
{
    // Callback privilege is deprecated in OpenBMC
    // At present, "OEM Privilege" is not used in OpenBMC
    return ((privLimit > PRIVILEGE_CALLBACK) && (privLimit < PRIVILEGE_OEM));
}

bool isValidAccessMode(const uint8_t accessMode)
{
    return (
        (accessMode >= static_cast<uint8_t>(EChannelAccessMode::disabled)) &&
        (accessMode <= static_cast<uint8_t>(EChannelAccessMode::shared)));
}

bool isValidChannel(const uint8_t chNum)
{
    return getChannelConfigObject().isValidChannel(chNum);
}

bool isValidAuthType(const uint8_t chNum, const EAuthType& authType)
{
    return getChannelConfigObject().isValidAuthType(chNum, authType);
}

EChannelSessSupported getChannelSessionSupport(const uint8_t chNum)
{
    return getChannelConfigObject().getChannelSessionSupport(chNum);
}

int getChannelActiveSessions(const uint8_t chNum)
{
    return getChannelConfigObject().getChannelActiveSessions(chNum);
}

size_t getChannelMaxTransferSize(uint8_t chNum)
{
    return getChannelConfigObject().getChannelMaxTransferSize(chNum);
}

Cc ipmiChannelInit()
{
    getChannelConfigObject();
    getCipherConfigObject(csPrivFileName, csPrivDefaultFileName);
    return ccSuccess;
}

Cc getChannelInfo(const uint8_t chNum, ChannelInfo& chInfo)
{
    return getChannelConfigObject().getChannelInfo(chNum, chInfo);
}

Cc getChannelAccessData(const uint8_t chNum, ChannelAccess& chAccessData)
{
    return getChannelConfigObject().getChannelAccessData(chNum, chAccessData);
}

Cc setChannelAccessData(const uint8_t chNum, const ChannelAccess& chAccessData,
                        const uint8_t setFlag)
{
    return getChannelConfigObject().setChannelAccessData(chNum, chAccessData,
                                                         setFlag);
}

Cc getChannelAccessPersistData(const uint8_t chNum, ChannelAccess& chAccessData)
{
    return getChannelConfigObject().getChannelAccessPersistData(chNum,
                                                                chAccessData);
}

Cc setChannelAccessPersistData(const uint8_t chNum,
                               const ChannelAccess& chAccessData,
                               const uint8_t setFlag)
{
    return getChannelConfigObject().setChannelAccessPersistData(
        chNum, chAccessData, setFlag);
}

Cc getChannelAuthTypeSupported(const uint8_t chNum, uint8_t& authTypeSupported)
{
    return getChannelConfigObject().getChannelAuthTypeSupported(
        chNum, authTypeSupported);
}

Cc getChannelEnabledAuthType(const uint8_t chNum, const uint8_t priv,
                             EAuthType& authType)
{
    return getChannelConfigObject().getChannelEnabledAuthType(chNum, priv,
                                                              authType);
}

std::string getChannelName(const uint8_t chNum)
{
    return getChannelConfigObject().getChannelName(chNum);
}

uint8_t getChannelByName(const std::string& chName)
{
    return getChannelConfigObject().getChannelByName(chName);
}

bool isValidPayloadType(const PayloadType payloadType)
{
    return (
        payloadType == PayloadType::IPMI || payloadType == PayloadType::SOL ||
        payloadType == PayloadType::OPEN_SESSION_REQUEST ||
        payloadType == PayloadType::OPEN_SESSION_RESPONSE ||
        payloadType == PayloadType::RAKP1 ||
        payloadType == PayloadType::RAKP2 ||
        payloadType == PayloadType::RAKP3 || payloadType == PayloadType::RAKP4);
}
} // namespace ipmi
