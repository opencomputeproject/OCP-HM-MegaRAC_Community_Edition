#pragma once

#include <ipmid/api.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ipmid/iana.hpp>
#include <vector>

namespace oem
{
constexpr std::size_t groupMagicSize = 3;

using Group = std::array<std::uint8_t, groupMagicSize>;

// Handler signature includes ipmi cmd to support wildcard cmd match.
// Buffers and lengths exclude the OemGroup bytes in the IPMI message.
// dataLen supplies length of reqBuf upon call, and should be set to the
// length of replyBuf upon return - conventional in this code base.
using Handler = std::function<ipmi_ret_t(ipmi_cmd_t,          // cmd byte
                                         const std::uint8_t*, // reqBuf
                                         std::uint8_t*,       // replyBuf
                                         std::size_t*)>;      // dataLen

/// Router Interface class.
/// @brief Abstract Router Interface
class Router
{
  public:
    virtual ~Router()
    {
    }

    /// Enable message routing to begin.
    virtual void activate() = 0;

    /// Register a handler for given OEMNumber & cmd.
    /// Use IPMI_CMD_WILDCARD to catch any unregistered cmd
    /// for the given OEMNumber.
    ///
    /// @param[in] oen - the OEM Number.
    /// @param[in] cmd - the Command.
    /// @param[in] handler - the handler to call given that OEN and
    ///                      command.
    virtual void registerHandler(Number oen, ipmi_cmd_t cmd,
                                 Handler handler) = 0;
};

/// Expose mutable Router for configuration & activation.
///
/// @returns pointer to OEM Router to use.
Router* mutableRouter();

/// Convert a group to an OEN.
///
/// @param[in] oeg - request buffer for IPMI command.
/// @return the OEN.
constexpr Number toOemNumber(const std::uint8_t oeg[groupMagicSize])
{
    return (oeg[2] << 16) | (oeg[1] << 8) | oeg[0];
}

/// Given a Group convert to an OEN.
///
/// @param[in] oeg - OEM Group reference.
/// @return the OEN.
constexpr Number toOemNumber(const Group& oeg)
{
    return (oeg[2] << 16) | (oeg[1] << 8) | oeg[0];
}

/// Given an OEN, conver to the OEM Group.
///
/// @param[in] oen - the OEM Number.
/// @return the OEM Group.
constexpr Group toOemGroup(Number oen)
{
    return Group{static_cast<std::uint8_t>(oen),
                 static_cast<std::uint8_t>(oen >> 8),
                 static_cast<std::uint8_t>(oen >> 16)};
}

} // namespace oem
