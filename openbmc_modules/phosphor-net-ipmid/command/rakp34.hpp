#pragma once

#include "comm_module.hpp"
#include "message_handler.hpp"

#include <vector>

namespace command
{

/**
 * @struct RAKP3request
 *
 * IPMI Payload for RAKP Message 3
 */
struct RAKP3request
{
    uint8_t messageTag;
    uint8_t rmcpStatusCode;
    uint16_t reserved;
    uint32_t managedSystemSessionID;
} __attribute__((packed));

/**
 * @struct RAKP4response
 *
 * IPMI Payload for RAKP Message 4
 */
struct RAKP4response
{
    uint8_t messageTag;
    uint8_t rmcpStatusCode;
    uint16_t reserved;
    uint32_t remoteConsoleSessionID;
} __attribute__((packed));

/**
 * @brief RAKP Message 3, RAKP Message 4
 *
 * The session activation process is completed by the remote console and BMC
 * exchanging messages that are signed according to the Authentication Algorithm
 * that was negotiated, and the parameters that were passed in the earlier
 * messages. RAKP Message 3 is the signed message from the remote console to the
 * BMC. After receiving RAKP Message 3, the BMC returns RAKP Message 4 - a
 * signed message from BMC to the remote console.
 *
 * @param[in] inPayload - Request Data for the command
 * @param[in] handler - Reference to the Message Handler
 *
 * @return Response data for the command
 */
std::vector<uint8_t> RAKP34(const std::vector<uint8_t>& inPayload,
                            const message::Handler& handler);

} // namespace command
