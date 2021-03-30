#pragma once

#include "config.h"

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace rsyslog_utils
{

/** @brief Restart rsyslog's systemd unit
 *         Ensures that it is restarted even if the start limit was
 *         hit in systemd.
 */
void restart()
{
    auto bus = sdbusplus::bus::new_default();
    constexpr char service[] = "rsyslog.service";

    {
        auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                          SYSTEMD_INTERFACE, "ResetFailedUnit");
        method.append(service);
        bus.call_noreply(method);
    }

    {
        auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                          SYSTEMD_INTERFACE, "RestartUnit");
        method.append(service, "replace");
        bus.call_noreply(method);
    }
}

} // namespace rsyslog_utils
} // namespace phosphor
