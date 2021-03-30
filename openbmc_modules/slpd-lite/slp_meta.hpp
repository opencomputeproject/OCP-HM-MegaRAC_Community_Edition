#pragma once

namespace slp
{
/** @brief SLP Version */
constexpr size_t VERSION_2 = 2;
constexpr auto SUCCESS = 0;
/** @brief SLP Port */
constexpr auto PORT = 427;

constexpr auto TIMEOUT = 30;
/** @brief SLP service lifetime */
constexpr auto LIFETIME = 5;

/** @brief Defines the constants for slp header.
 *  Size and the offsets.
 */
namespace header
{

constexpr size_t SIZE_VERSION = 1;
constexpr size_t SIZE_LENGTH = 1;
constexpr size_t SIZE_FLAGS = 2;
constexpr size_t SIZE_EXT = 3;
constexpr size_t SIZE_XID = 2;
constexpr size_t SIZE_LANG = 2;

constexpr size_t OFFSET_VERSION = 0;
constexpr size_t OFFSET_FUNCTION = 1;
constexpr size_t OFFSET_LENGTH = 4;
constexpr size_t OFFSET_FLAGS = 5;
constexpr size_t OFFSET_EXT = 7;
constexpr size_t OFFSET_XID = 10;
constexpr size_t OFFSET_LANG_LEN = 12;
constexpr size_t OFFSET_LANG = 14;

constexpr size_t MIN_LEN = 14;
} // namespace header

/** @brief Defines the constants for slp response.
 *  Size and the offsets.
 */

namespace response
{

constexpr size_t SIZE_ERROR = 2;
constexpr size_t SIZE_SERVICE = 2;
constexpr size_t SIZE_URL_COUNT = 2;
constexpr size_t SIZE_URL_ENTRY = 6;
constexpr size_t SIZE_RESERVED = 1;
constexpr size_t SIZE_LIFETIME = 2;
constexpr size_t SIZE_URLLENGTH = 2;
constexpr size_t SIZE_AUTH = 1;

constexpr size_t OFFSET_ERROR = 16;
constexpr size_t OFFSET_SERVICE_LEN = 18;
constexpr size_t OFFSET_SERVICE = 20;
constexpr size_t OFFSET_URL_ENTRY = 18;

} // namespace response

/** @brief Defines the constants for slp request.
 *  Size and the offsets.
 */
namespace request
{

constexpr size_t MIN_SRVTYPE_LEN = 22;
constexpr size_t MIN_SRV_LEN = 26;

constexpr size_t SIZE_PRLIST = 2;
constexpr size_t SIZE_NAMING = 2;
constexpr size_t SIZE_SCOPE = 2;
constexpr size_t SIZE_SERVICE_TYPE = 2;
constexpr size_t SIZE_PREDICATE = 2;
constexpr size_t SIZE_SLPI = 2;

constexpr size_t OFFSET_PR_LEN = 16;
constexpr size_t OFFSET_PR = 18;
constexpr size_t OFFSET_SERVICE = 20;

} // namespace request
} // namespace slp
