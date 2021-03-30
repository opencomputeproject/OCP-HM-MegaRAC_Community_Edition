#include "rakp12.hpp"

#include "comm_module.hpp"
#include "endian.hpp"
#include "guid.hpp"
#include "main.hpp"

#include <openssl/rand.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <phosphor-logging/log.hpp>

using namespace phosphor::logging;

namespace command
{

bool isChannelAccessModeEnabled(const uint8_t accessMode)
{
    return accessMode !=
           static_cast<uint8_t>(ipmi::EChannelAccessMode::disabled);
}

std::vector<uint8_t> RAKP12(const std::vector<uint8_t>& inPayload,
                            const message::Handler& handler)
{
    std::vector<uint8_t> outPayload(sizeof(RAKP2response));
    auto request = reinterpret_cast<const RAKP1request*>(inPayload.data());
    auto response = reinterpret_cast<RAKP2response*>(outPayload.data());

    // Session ID zero is reserved for Session Setup
    if (endian::from_ipmi(request->managedSystemSessionID) ==
        session::sessionZero)
    {
        log<level::INFO>("RAKP12: BMC invalid Session ID");
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::INVALID_SESSION_ID);
        return outPayload;
    }

    std::shared_ptr<session::Session> session;
    try
    {
        session =
            std::get<session::Manager&>(singletonPool)
                .getSession(endian::from_ipmi(request->managedSystemSessionID));
    }
    catch (std::exception& e)
    {
        log<level::ERR>("RAKP12 : session not found",
                        entry("EXCEPTION=%s", e.what()));
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::INVALID_SESSION_ID);
        return outPayload;
    }

    auto rakp1Size =
        sizeof(RAKP1request) - (userNameMaxLen - request->user_name_len);

    // Validate user name length in the message
    if (request->user_name_len > userNameMaxLen ||
        inPayload.size() != rakp1Size)
    {
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::INVALID_NAME_LENGTH);
        return outPayload;
    }

    session->userName.assign(request->user_name, request->user_name_len);

    // Update transaction time
    session->updateLastTransactionTime();

    auto rcSessionID = endian::to_ipmi(session->getRCSessionID());
    auto bmcSessionID = endian::to_ipmi(session->getBMCSessionID());
    auto authAlgo = session->getAuthAlgo();

    /*
     * Generate Key Authentication Code - RAKP 2
     *
     * 1) Remote Console Session ID - 4 bytes
     * 2) Managed System Session ID - 4 bytes
     * 3) Remote Console Random Number - 16 bytes
     * 4) Managed System Random Number - 16 bytes
     * 5) Managed System GUID - 16 bytes
     * 6) Requested Privilege Level - 1 byte
     * 7) User Name Length Byte - 1 byte (0 for 'null' username)
     * 8) User Name - variable (absent for 'null' username)
     */

    std::vector<uint8_t> input;
    input.resize(sizeof(rcSessionID) + sizeof(bmcSessionID) +
                 cipher::rakp_auth::REMOTE_CONSOLE_RANDOM_NUMBER_LEN +
                 cipher::rakp_auth::BMC_RANDOM_NUMBER_LEN + BMC_GUID_LEN +
                 sizeof(request->req_max_privilege_level) +
                 sizeof(request->user_name_len) + session->userName.size());

    auto iter = input.begin();

    // Remote Console Session ID
    std::copy_n(reinterpret_cast<uint8_t*>(&rcSessionID), sizeof(rcSessionID),
                iter);
    std::advance(iter, sizeof(rcSessionID));

    // Managed System Session ID
    std::copy_n(reinterpret_cast<uint8_t*>(&bmcSessionID), sizeof(bmcSessionID),
                iter);
    std::advance(iter, sizeof(bmcSessionID));

    // Copy the Remote Console Random Number from the RAKP1 request to the
    // Authentication Algorithm
    std::copy_n(
        reinterpret_cast<const uint8_t*>(request->remote_console_random_number),
        cipher::rakp_auth::REMOTE_CONSOLE_RANDOM_NUMBER_LEN,
        authAlgo->rcRandomNum.begin());

    std::copy(authAlgo->rcRandomNum.begin(), authAlgo->rcRandomNum.end(), iter);
    std::advance(iter, cipher::rakp_auth::REMOTE_CONSOLE_RANDOM_NUMBER_LEN);

    // Generate the Managed System Random Number
    if (!RAND_bytes(input.data() + sizeof(rcSessionID) + sizeof(bmcSessionID) +
                        cipher::rakp_auth::REMOTE_CONSOLE_RANDOM_NUMBER_LEN,
                    cipher::rakp_auth::BMC_RANDOM_NUMBER_LEN))
    {
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::INSUFFICIENT_RESOURCE);
        return outPayload;
    }
    // As stated in Set Session Privilege Level command in IPMI Spec, when
    // creating a session through Activate command / RAKP 1 message, it must
    // be established with USER privilege as well as all other sessions are
    // initially set to USER privilege, regardless of the requested maximum
    // privilege.
    if (!(static_cast<session::Privilege>(request->req_max_privilege_level &
                                          session::reqMaxPrivMask) >
          session::Privilege::CALLBACK))
    {
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::UNAUTH_ROLE_PRIV);
        return outPayload;
    }
    session->currentPrivilege(static_cast<uint8_t>(session::Privilege::USER));

    session->reqMaxPrivLevel =
        static_cast<session::Privilege>(request->req_max_privilege_level);
    if (request->user_name_len == 0)
    {
        // Bail out, if user name is not specified.
        // Yes, NULL user name is not supported for security reasons.
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::UNAUTH_NAME);
        return outPayload;
    }

    // Perform user name based lookup
    std::string userName(request->user_name, request->user_name_len);
    std::string passwd;
    uint8_t userId = ipmi::ipmiUserGetUserId(userName);
    if (userId == ipmi::invalidUserId)
    {
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::UNAUTH_NAME);
        return outPayload;
    }
    // check user is enabled before proceeding.
    bool userEnabled = false;
    ipmi::ipmiUserCheckEnabled(userId, userEnabled);
    if (!userEnabled)
    {
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::INACTIVE_ROLE);
        return outPayload;
    }
    // Get the user password for RAKP message authenticate
    passwd = ipmi::ipmiUserGetPassword(userName);
    if (passwd.empty())
    {
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::UNAUTH_NAME);
        return outPayload;
    }
    // Check whether user is already locked for failed attempts
    if (!ipmi::ipmiUserPamAuthenticate(userName, passwd))
    {
        log<level::ERR>("Authentication failed - user already locked out",
                        entry("USER-ID=%d", static_cast<uint8_t>(userId)));

        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::UNAUTH_NAME);
        return outPayload;
    }

    uint8_t chNum = static_cast<uint8_t>(getInterfaceIndex());
    // Get channel based access information
    if ((ipmi::ipmiUserGetPrivilegeAccess(
             userId, chNum, session->sessionUserPrivAccess) != IPMI_CC_OK) ||
        (ipmi::getChannelAccessData(chNum, session->sessionChannelAccess) !=
         IPMI_CC_OK))
    {
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::INACTIVE_ROLE);
        return outPayload;
    }
    if (!isChannelAccessModeEnabled(session->sessionChannelAccess.accessMode))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Channel access mode disabled.");
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::INACTIVE_ROLE);
        return outPayload;
    }
    if (session->sessionUserPrivAccess.privilege >
        static_cast<uint8_t>(session::Privilege::OEM))
    {
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::INACTIVE_ROLE);
        return outPayload;
    }
    session->channelNum(chNum);
    session->userID(userId);
    // minimum privilege of Channel / User / session::privilege::USER
    // has to be used as session current privilege level
    uint8_t minPriv = 0;
    if (session->sessionChannelAccess.privLimit <
        session->sessionUserPrivAccess.privilege)
    {
        minPriv = session->sessionChannelAccess.privLimit;
    }
    else
    {
        minPriv = session->sessionUserPrivAccess.privilege;
    }
    if (session->currentPrivilege() > minPriv)
    {
        session->currentPrivilege(static_cast<uint8_t>(minPriv));
    }
    // For username / privilege lookup, fail with UNAUTH_NAME, if requested
    // max privilege does not match user privilege
    if (((request->req_max_privilege_level & userNameOnlyLookupMask) ==
         userNamePrivLookup) &&
        ((request->req_max_privilege_level & session::reqMaxPrivMask) !=
         session->sessionUserPrivAccess.privilege))
    {
        log<level::INFO>(
            "Username/Privilege lookup failed for requested privilege");
        response->rmcpStatusCode =
            static_cast<uint8_t>(RAKP_ReturnCode::UNAUTH_NAME);
        return outPayload;
    }

    std::fill(authAlgo->userKey.data(),
              authAlgo->userKey.data() + authAlgo->userKey.size(), 0);
    std::copy_n(passwd.c_str(), passwd.size(), authAlgo->userKey.data());

    // Copy the Managed System Random Number to the Authentication Algorithm
    std::copy_n(iter, cipher::rakp_auth::BMC_RANDOM_NUMBER_LEN,
                authAlgo->bmcRandomNum.begin());
    std::advance(iter, cipher::rakp_auth::BMC_RANDOM_NUMBER_LEN);

    // Managed System GUID
    std::copy_n(cache::guid.data(), cache::guid.size(), iter);
    std::advance(iter, BMC_GUID_LEN);

    // Requested Privilege Level
    std::copy_n(&(request->req_max_privilege_level),
                sizeof(request->req_max_privilege_level), iter);
    std::advance(iter, sizeof(request->req_max_privilege_level));

    // User Name Length Byte
    std::copy_n(&(request->user_name_len), sizeof(request->user_name_len),
                iter);
    std::advance(iter, sizeof(request->user_name_len));

    std::copy_n(session->userName.data(), session->userName.size(), iter);

    // Generate Key Exchange Authentication Code - RAKP2
    auto output = authAlgo->generateHMAC(input);

    response->messageTag = request->messageTag;
    response->rmcpStatusCode = static_cast<uint8_t>(RAKP_ReturnCode::NO_ERROR);
    response->reserved = 0;
    response->remoteConsoleSessionID = rcSessionID;

    // Copy Managed System Random Number to the Response
    std::copy(authAlgo->bmcRandomNum.begin(), authAlgo->bmcRandomNum.end(),
              response->managed_system_random_number);

    // Copy System GUID to the Response
    std::copy_n(cache::guid.data(), cache::guid.size(),
                response->managed_system_guid);

    // Insert the HMAC output into the payload
    outPayload.insert(outPayload.end(), output.begin(), output.end());
    return outPayload;
}

} // namespace command
