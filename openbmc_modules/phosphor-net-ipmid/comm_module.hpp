#pragma once

#include "message_handler.hpp"

#include <cstdint>

namespace command
{

/**
 * @brief RMCP+ and RAKP Message Status Codes
 */
enum class RAKP_ReturnCode : uint8_t
{
    NO_ERROR,                    //!< No errors
    INSUFFICIENT_RESOURCE,       //!< Insufficient resources to create a session
    INVALID_SESSION_ID,          //!< Invalid Session ID
    INVALID_PAYLOAD_TYPE,        //!< Invalid payload type
    INVALID_AUTH_ALGO,           //!< Invalid authentication algorithm
    INVALID_INTEGRITY_ALGO,      //!< Invalid integrity algorithm
    NO_MATCH_AUTH_PAYLOAD,       //!< No matching authentication payload
    NO_MATCH_INTEGRITY_PAYLOAD,  //!< No matching integrity payload
    INACTIVE_SESSIONID,          //!< Inactive Session ID
    INACTIVE_ROLE,               //!< Invalid role
    UNAUTH_ROLE_PRIV,            //!< Unauthorized role or privilege requested
    INSUFFICIENT_RESOURCES_ROLE, //!< Insufficient resources to create a session
    INVALID_NAME_LENGTH,         //!< Invalid name length
    UNAUTH_NAME,                 //!< Unauthorized name
    UNAUTH_GUID,                 //!< Unauthorized GUID
    INVALID_INTEGRITY_VALUE,     //!< Invalid integrity check value
    INVALID_CONF_ALGO,           //!< Invalid confidentiality algorithm
    NO_CIPHER_SUITE_MATCH,       //!< No Cipher Suite match with security algos
    ILLEGAL_PARAMETER,           //!< Illegal or unrecognized parameter
};

/**
 * @brief Register Session Setup commands to the Command Table
 */
void sessionSetupCommands();

} // namespace command
