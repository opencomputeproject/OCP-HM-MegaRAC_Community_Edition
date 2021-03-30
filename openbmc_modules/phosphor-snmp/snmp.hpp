#pragma once

#include "snmp_notification.hpp"

namespace phosphor
{
namespace network
{
namespace snmp
{

/* @brief sends the trap to the snmp manager
 * T - Notification type
 * @param[in] tArgs - arguments for the trap.
 */

template <typename T, typename... ArgTypes> void sendTrap(ArgTypes&&... tArgs)
{
    T obj(std::forward<ArgTypes>(tArgs)...);
    obj.sendTrap();
}

} // namespace snmp
} // namespace network
} // namespace phosphor
