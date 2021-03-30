#pragma once

#include "message_handler.hpp"

#include <vector>

namespace command
{

/**
 * @struct GetChannelCapabilitiesReq
 *
 * IPMI Request data for Get Channel Authentication Capabilities command
 */
struct GetChannelCapabilitiesReq
{
    uint8_t channelNumber;
    uint8_t reqMaxPrivLevel;
} __attribute__((packed));

/**
 * @struct GetChannelCapabilitiesResp
 *
 * IPMI Response data for Get Channel Authentication Capabilities command
 */
struct GetChannelCapabilitiesResp
{
    uint8_t completionCode; // Completion Code

    uint8_t channelNumber; // Channel number that the request was
    // received on

#if BYTE_ORDER == LITTLE_ENDIAN
    uint8_t none : 1;
    uint8_t md2 : 1;
    uint8_t md5 : 1;
    uint8_t reserved2 : 1;
    uint8_t straightKey : 1; // Straight password/key support
    // Support OEM identified by the IANA OEM ID in RMCP+ ping response
    uint8_t oem : 1;
    uint8_t reserved1 : 1;
    uint8_t ipmiVersion : 1; // 0b = IPMIV1.5 support only, 1B = IPMI V2.0
    // support
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t ipmiVersion : 1; // 0b = IPMIV1.5 support only, 1B = IPMI V2.0
    // support
    uint8_t reserved1 : 1;
    // Support OEM identified by the IANA OEM ID in RMCP+ ping response
    uint8_t oem : 1;
    uint8_t straightKey : 1; // Straight password/key support
    uint8_t reserved2 : 1;
    uint8_t md5 : 1;
    uint8_t md2 : 1;
    uint8_t none : 1;
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
    // Anonymous login status for anonymous login enabled/disabled
    uint8_t anonymousLogin : 1;
    // Anonymous login status for null usernames enabled/disabled
    uint8_t nullUsers : 1;
    // Anonymous login status for non-null usernames enabled/disabled
    uint8_t nonNullUsers : 1;
    uint8_t userAuth : 1;       // User level authentication status
    uint8_t perMessageAuth : 1; // Per-message authentication support
    // Two key login status . only for IPMI V2.0 RMCP+ RAKP
    uint8_t KGStatus : 1;
    uint8_t reserved3 : 2;
#endif

#if BYTE_ORDER == BIG_ENDIAN
    uint8_t reserved3 : 2;
    // Two key login status . only for IPMI V2.0 RMCP+ RAKP
    uint8_t KGStatus : 1;
    uint8_t perMessageAuth : 1; // Per-message authentication support
    uint8_t userAuth : 1;       // User level authentication status
    // Anonymous login status for non-null usernames enabled/disabled
    uint8_t nonNullUsers : 1;
    // Anonymous login status for null usernames enabled/disabled
    uint8_t nullUsers : 1;
    // Anonymous login status for anonymous login enabled/disabled
    uint8_t anonymousLogin : 1;
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
    // Extended capabilities will be present only if IPMI version is V2.0
    uint8_t extCapabilities : 2; // Channel support for IPMI V2.0 connections
    uint8_t reserved4 : 6;
#endif

#if BYTE_ORDER == BIG_ENDIAN
    // Extended capabilities will be present only if IPMI version is V2.0
    uint8_t reserved4 : 6;
    uint8_t extCapabilities : 2; // Channel support for IPMI V2.0 connections
#endif

    // Below 4 bytes will all the 0's if no OEM authentication type available.
    uint8_t oemID[3];     // IANA enterprise number for OEM/organization
    uint8_t oemAuxillary; // Addition OEM specific information..
} __attribute__((packed));

/**
 * @brief Get Channel Authentication Capabilities
 *
 * This message exchange provides a way for a remote console to discover what
 * IPMI version is supported i.e. whether or not the BMC supports the IPMI
 * v2.0 / RMCP+ packet format. It also provides information that the remote
 * console can use to determine whether anonymous, “one-key”, or “two-key”
 * logins are used.This information can guide a remote console in how it
 * presents queries to users for username and password information. This is a
 * ‘session-less’ command that the BMC accepts in both IPMI v1.5 and v2.0/RMCP+
 * packet formats.
 *
 * @param[in] inPayload - Request Data for the command
 * @param[in] handler - Reference to the Message Handler
 *
 * @return Response data for the command
 */
std::vector<uint8_t>
    GetChannelCapabilities(const std::vector<uint8_t>& inPayload,
                           const message::Handler& handler);

} // namespace command
