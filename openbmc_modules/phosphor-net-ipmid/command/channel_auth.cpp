#include "channel_auth.hpp"

#include <ipmid/api.h>

#include <user_channel/channel_layer.hpp>
#include <user_channel/user_layer.hpp>

namespace command
{

std::vector<uint8_t>
    GetChannelCapabilities(const std::vector<uint8_t>& inPayload,
                           const message::Handler& handler)
{
    auto request =
        reinterpret_cast<const GetChannelCapabilitiesReq*>(inPayload.data());
    if (inPayload.size() != sizeof(*request))
    {
        std::vector<uint8_t> errorPayload{IPMI_CC_REQ_DATA_LEN_INVALID};
        return errorPayload;
    }
    constexpr unsigned int channelMask = 0x0f;
    uint8_t chNum = ipmi::convertCurrentChannelNum(
        request->channelNumber & channelMask, getInterfaceIndex());

    if (!ipmi::isValidChannel(chNum) ||
        (ipmi::EChannelSessSupported::none ==
         ipmi::getChannelSessionSupport(chNum)) ||
        !ipmi::isValidPrivLimit(request->reqMaxPrivLevel))
    {
        std::vector<uint8_t> errorPayload{IPMI_CC_INVALID_FIELD_REQUEST};
        return errorPayload;
    }

    std::vector<uint8_t> outPayload(sizeof(GetChannelCapabilitiesResp));
    auto response =
        reinterpret_cast<GetChannelCapabilitiesResp*>(outPayload.data());

    // A canned response, since there is no user and channel management.
    response->completionCode = IPMI_CC_OK;

    response->channelNumber = chNum;

    response->ipmiVersion = 1; // IPMI v2.0 extended capabilities available.
    response->reserved1 = 0;
    response->oem = 0;
    response->straightKey = 0;
    response->reserved2 = 0;
    response->md5 = 0;
    response->md2 = 0;

    response->reserved3 = 0;
    response->KGStatus = 0;       // KG is set to default
    response->perMessageAuth = 0; // Per-message Authentication is enabled
    response->userAuth = 0;       // User Level Authentication is enabled
    uint8_t maxChUsers = 0;
    uint8_t enabledUsers = 0;
    uint8_t fixedUsers = 0;
    ipmi::ipmiUserGetAllCounts(maxChUsers, enabledUsers, fixedUsers);

    response->nonNullUsers = enabledUsers > 0 ? 1 : 0; // Non-null usernames
    response->nullUsers = 0;      // Null usernames disabled
    response->anonymousLogin = 0; // Anonymous Login disabled

    response->reserved4 = 0;
    response->extCapabilities = 0x2; // Channel supports IPMI v2.0 connections

    response->oemID[0] = 0;
    response->oemID[1] = 0;
    response->oemID[2] = 0;
    response->oemAuxillary = 0;
    return outPayload;
}

} // namespace command
