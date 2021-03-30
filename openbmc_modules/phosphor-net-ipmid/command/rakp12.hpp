#pragma once

#include "comm_module.hpp"
#include "message_handler.hpp"

#include <cstddef>
#include <vector>

namespace command
{

constexpr size_t userNameMaxLen = 16;

constexpr uint8_t userNameOnlyLookupMask = 0x10;
constexpr uint8_t userNameOnlyLookup = 0x10;
constexpr uint8_t userNamePrivLookup = 0x0;

/**
 * @struct RAKP1request
 *
 * IPMI Payload for RAKP Message 1
 */
struct RAKP1request
{
    uint8_t messageTag;
    uint8_t reserved1;
    uint16_t reserved2;
    uint32_t managedSystemSessionID;
    uint8_t remote_console_random_number[16];
    uint8_t req_max_privilege_level;
    uint16_t reserved3;
    uint8_t user_name_len;
    char user_name[userNameMaxLen];
} __attribute__((packed));

/**
 * @struct RAKP2response
 *
 * IPMI Payload for RAKP Message 2
 */
struct RAKP2response
{
    uint8_t messageTag;
    uint8_t rmcpStatusCode;
    uint16_t reserved;
    uint32_t remoteConsoleSessionID;
    uint8_t managed_system_random_number[16];
    uint8_t managed_system_guid[16];
} __attribute__((packed));

/**
 * @brief RAKP Message 1, RAKP Message 2
 *
 * These messages are used to exchange random number and identification
 * information between the BMC and the remote console that are, in effect,
 * mutual challenges for a challenge/response. (Unlike IPMI v1.5, the v2.0/RMCP+
 * challenge/response is symmetric. I.e. the remote console and BMC both issues
 * challenges,and both need to provide valid responses for the session to be
 * activated.)
 *
 * The remote console request (RAKP Message 1) passes a random number and
 * username/privilege information that the BMC will later use to ‘sign’ a
 * response message based on key information associated with the user and the
 * Authentication Algorithm negotiated in the Open Session Request/Response
 * exchange. The BMC responds with RAKP Message 2 and passes a random number and
 * GUID (globally unique ID) for the managed system that the remote console
 * uses according the Authentication Algorithm to sign a response back to the
 * BMC.
 *
 * @param[in] inPayload - Request Data for the command
 * @param[in] handler - Reference to the Message Handler
 *
 * @return Response data for the command
 */
std::vector<uint8_t> RAKP12(const std::vector<uint8_t>& inPayload,
                            const message::Handler& handler);

} // namespace command
