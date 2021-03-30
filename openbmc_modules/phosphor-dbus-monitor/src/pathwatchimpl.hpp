/**
 * @file PathWatchimpl.hpp
 * @brief Add interfaces added watch for the specified path
 *
 */
#pragma once

#include "callback.hpp"
#include "data_types.hpp"
#include "pathwatch.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <vector>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

template <typename DBusInterfaceType>
void PathWatch<DBusInterfaceType>::start()
{
    if (alreadyRan)
    {
        return;
    }
    // Watch for new interfaces added on this path.
    DBusInterfaceType::addMatch(
        sdbusplus::bus::match::rules::interfacesAdded(objectPath),
        [this](auto& msg)
        // *INDENT-OFF*
        { (this->cb)(Context::SIGNAL, msg); });
    // *INDENT-ON*

    alreadyRan = true;
}

template <typename DBusInterfaceType>
void PathWatch<DBusInterfaceType>::callback(Context ctx)
{
    (this->cb)(ctx);
}

template <typename DBusInterfaceType>
void PathWatch<DBusInterfaceType>::callback(Context ctx,
                                            sdbusplus::message::message& msg)
{
    (this->cb)(ctx, msg);
}
} // namespace monitoring
} // namespace dbus
} // namespace phosphor
