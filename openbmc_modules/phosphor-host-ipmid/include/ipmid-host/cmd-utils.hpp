#pragma once

#include <unistd.h>

#include <cstdint>
#include <functional>
#include <tuple>

namespace phosphor
{
namespace host
{
namespace command
{
/** @detail After sending SMS_ATN to the Host, Host comes down and
 *          asks why an 'SMS_ATN` was sent.
 *          BMC then sends 'There is a Message to be Read` as response.
 *          Host then comes down asks for Message and the specified
 *          commands and data would go as data conforming to IPMI spec.
 *
 *          Refer: 6.13.2 Send Message Command From System Interface
 *          in IPMI V2.0 spec.
 */

/** @brief IPMI command */
using IPMIcmd = uint8_t;

/** @brief Data associated with command */
using Data = uint8_t;

/** @brief <IPMI command, Data> to be sent as payload when Host asks for
 *          the message that can be associated with the previous SMS_ATN
 */
using IpmiCmdData = std::pair<IPMIcmd, Data>;

/** @detail Implementation specific callback function to be invoked
 *          conveying the status of the executed command. Specific
 *          implementations may then broadcast an agreed signal
 */
using CallBack = std::function<void(IpmiCmdData, bool)>;

/** @detail Tuple encapsulating above 2 to enable using Manager by
 *          different implementations. Users of Manager will supply
 *          <Ipmi command, Data> along with the callback handler.
 *          Manager will invoke the handler onveying the status of
 *          the command.
 */
using CommandHandler = std::tuple<IpmiCmdData, CallBack>;

} // namespace command
} // namespace host
} // namespace phosphor
