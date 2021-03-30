#include "payload_cmds.hpp"

#include "main.hpp"
#include "sol/sol_manager.hpp"
#include "sol_cmds.hpp"

#include <ipmid/api.h>

#include <ipmid/api-types.hpp>
#include <phosphor-logging/log.hpp>

namespace sol
{

namespace command
{

using namespace phosphor::logging;

std::vector<uint8_t> activatePayload(const std::vector<uint8_t>& inPayload,
                                     const message::Handler& handler)
{
    std::vector<uint8_t> outPayload(sizeof(ActivatePayloadResponse));
    auto request =
        reinterpret_cast<const ActivatePayloadRequest*>(inPayload.data());
    auto response =
        reinterpret_cast<ActivatePayloadResponse*>(outPayload.data());

    if (inPayload.size() != sizeof(ActivatePayloadRequest))
    {
        response->completionCode = ipmi::ccReqDataLenInvalid;
        outPayload.resize(sizeof(response->completionCode));
        return outPayload;
    }

    response->completionCode = IPMI_CC_OK;

    // SOL is the payload currently supported for activation.
    if (static_cast<uint8_t>(message::PayloadType::SOL) != request->payloadType)
    {
        response->completionCode = IPMI_CC_INVALID_FIELD_REQUEST;
        return outPayload;
    }

    if (!std::get<sol::Manager&>(singletonPool).enable)
    {
        response->completionCode = IPMI_CC_PAYLOAD_TYPE_DISABLED;
        return outPayload;
    }

    // Only one instance of SOL is currently supported.
    if (request->payloadInstance != 1)
    {
        response->completionCode = IPMI_CC_INVALID_FIELD_REQUEST;
        return outPayload;
    }

    auto session = std::get<session::Manager&>(singletonPool)
                       .getSession(handler.sessionID);

    if (!request->encryption && session->isCryptAlgoEnabled())
    {
        response->completionCode = IPMI_CC_PAYLOAD_WITHOUT_ENCRYPTION;
        return outPayload;
    }

    // Is SOL Payload enabled for this user & channel.
    auto userId = ipmi::ipmiUserGetUserId(session->userName);
    ipmi::PayloadAccess payloadAccess = {};
    if ((ipmi::ipmiUserGetUserPayloadAccess(session->channelNum(), userId,
                                            payloadAccess) != IPMI_CC_OK) ||
        !(payloadAccess.stdPayloadEnables1[static_cast<uint8_t>(
            message::PayloadType::SOL)]))
    {
        response->completionCode = IPMI_CC_PAYLOAD_TYPE_DISABLED;
        return outPayload;
    }

    auto status = std::get<sol::Manager&>(singletonPool)
                      .isPayloadActive(request->payloadInstance);
    if (status)
    {
        response->completionCode = IPMI_CC_PAYLOAD_ALREADY_ACTIVE;
        return outPayload;
    }

    // Set the current command's socket channel to the session
    handler.setChannelInSession();

    // Start the SOL payload
    try
    {
        std::get<sol::Manager&>(singletonPool)
            .startPayloadInstance(request->payloadInstance, handler.sessionID);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(e.what());
        response->completionCode = IPMI_CC_UNSPECIFIED_ERROR;
        return outPayload;
    }

    response->inPayloadSize = endian::to_ipmi<uint16_t>(MAX_PAYLOAD_SIZE);
    response->outPayloadSize = endian::to_ipmi<uint16_t>(MAX_PAYLOAD_SIZE);
    response->portNum = endian::to_ipmi<uint16_t>(IPMI_STD_PORT);

    // VLAN addressing is not used
    response->vlanNum = 0xFFFF;

    return outPayload;
}

std::vector<uint8_t> deactivatePayload(const std::vector<uint8_t>& inPayload,
                                       const message::Handler& handler)
{
    std::vector<uint8_t> outPayload(sizeof(DeactivatePayloadResponse));
    auto request =
        reinterpret_cast<const DeactivatePayloadRequest*>(inPayload.data());
    auto response =
        reinterpret_cast<DeactivatePayloadResponse*>(outPayload.data());

    response->completionCode = IPMI_CC_OK;

    if (inPayload.size() != sizeof(DeactivatePayloadRequest))
    {
        response->completionCode = IPMI_CC_REQ_DATA_LEN_INVALID;
        return outPayload;
    }

    // SOL is the payload currently supported for deactivation
    if (static_cast<uint8_t>(message::PayloadType::SOL) != request->payloadType)
    {
        response->completionCode = IPMI_CC_INVALID_FIELD_REQUEST;
        return outPayload;
    }

    // Only one instance of SOL is supported
    if (request->payloadInstance != 1)
    {
        response->completionCode = IPMI_CC_INVALID_FIELD_REQUEST;
        return outPayload;
    }

    auto status = std::get<sol::Manager&>(singletonPool)
                      .isPayloadActive(request->payloadInstance);
    if (!status)
    {
        response->completionCode = IPMI_CC_PAYLOAD_DEACTIVATED;
        return outPayload;
    }

    try
    {
        auto& context = std::get<sol::Manager&>(singletonPool)
                            .getContext(request->payloadInstance);
        auto sessionID = context.sessionID;

        std::get<sol::Manager&>(singletonPool)
            .stopPayloadInstance(request->payloadInstance);

        try
        {
            activating(request->payloadInstance, sessionID);
        }
        catch (std::exception& e)
        {
            log<level::INFO>(e.what());
            /*
             * In case session has been closed (like in the case of inactivity
             * timeout), then activating function would throw an exception,
             * since sessionID is not found. IPMI success completion code is
             * returned, since the session is closed.
             */
            return outPayload;
        }
    }
    catch (std::exception& e)
    {
        log<level::ERR>(e.what());
        response->completionCode = IPMI_CC_UNSPECIFIED_ERROR;
        return outPayload;
    }

    return outPayload;
}

std::vector<uint8_t> getPayloadStatus(const std::vector<uint8_t>& inPayload,
                                      const message::Handler& handler)
{
    std::vector<uint8_t> outPayload(sizeof(GetPayloadStatusResponse));
    auto request =
        reinterpret_cast<const GetPayloadStatusRequest*>(inPayload.data());
    auto response =
        reinterpret_cast<GetPayloadStatusResponse*>(outPayload.data());

    // SOL is the payload currently supported for payload status
    if (static_cast<uint8_t>(message::PayloadType::SOL) != request->payloadType)
    {
        response->completionCode = IPMI_CC_UNSPECIFIED_ERROR;
        return outPayload;
    }

    response->completionCode = IPMI_CC_OK;

    constexpr size_t maxSolPayloadInstances = 1;
    response->capacity = maxSolPayloadInstances;

    // Currently we support only one SOL session
    response->instance1 =
        std::get<sol::Manager&>(singletonPool).isPayloadActive(1);

    return outPayload;
}

std::vector<uint8_t> getPayloadInfo(const std::vector<uint8_t>& inPayload,
                                    const message::Handler& handler)
{
    std::vector<uint8_t> outPayload(sizeof(GetPayloadInfoResponse));
    auto request =
        reinterpret_cast<const GetPayloadInfoRequest*>(inPayload.data());
    auto response =
        reinterpret_cast<GetPayloadInfoResponse*>(outPayload.data());

    if (inPayload.size() != sizeof(GetPayloadInfoRequest))
    {
        response->completionCode = IPMI_CC_REQ_DATA_LEN_INVALID;
        return outPayload;
    }
    // SOL is the payload currently supported for payload status & only one
    // instance of SOL is supported.
    if (static_cast<uint8_t>(message::PayloadType::SOL) !=
            request->payloadType ||
        request->payloadInstance != 1)
    {
        response->completionCode = IPMI_CC_INVALID_FIELD_REQUEST;
        return outPayload;
    }
    auto status = std::get<sol::Manager&>(singletonPool)
                      .isPayloadActive(request->payloadInstance);

    if (status)
    {
        auto& context = std::get<sol::Manager&>(singletonPool)
                            .getContext(request->payloadInstance);
        response->sessionID = context.sessionID;
    }
    else
    {
        // No active payload - return session id as 0
        response->sessionID = 0;
    }
    response->completionCode = IPMI_CC_OK;
    return outPayload;
}

} // namespace command

} // namespace sol
