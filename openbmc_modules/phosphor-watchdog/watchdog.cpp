#include "watchdog.hpp"

#include <algorithm>
#include <chrono>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace watchdog
{
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace phosphor::logging;

using sdbusplus::exception::SdBusError;
using sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

// systemd service to kick start a target.
constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_ROOT = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

void Watchdog::resetTimeRemaining(bool enableWatchdog)
{
    timeRemaining(interval());
    if (enableWatchdog)
    {
        enabled(true);
    }
}

// Enable or disable watchdog
bool Watchdog::enabled(bool value)
{
    if (!value)
    {
        // Make sure we accurately reflect our enabled state to the
        // tryFallbackOrDisable() call
        WatchdogInherits::enabled(value);

        // Attempt to fallback or disable our timer if needed
        tryFallbackOrDisable();

        return false;
    }
    else if (!this->enabled())
    {
        auto interval_ms = this->interval();
        timer.restart(milliseconds(interval_ms));
        log<level::INFO>("watchdog: enabled and started",
                         entry("INTERVAL=%llu", interval_ms));
    }

    return WatchdogInherits::enabled(value);
}

// Get the remaining time before timer expires.
// If the timer is disabled, returns 0
uint64_t Watchdog::timeRemaining() const
{
    // timer may have already expired and disabled
    if (!timerEnabled())
    {
        return 0;
    }

    return duration_cast<milliseconds>(timer.getRemaining()).count();
}

// Reset the timer to a new expiration value
uint64_t Watchdog::timeRemaining(uint64_t value)
{
    if (!timerEnabled())
    {
        // We don't need to update the timer because it is off
        return 0;
    }

    if (this->enabled())
    {
        // Update interval to minInterval if applicable
        value = std::max(value, minInterval);
    }
    else
    {
        // Having a timer but not displaying an enabled value means we
        // are inside of the fallback
        value = fallback->interval;
    }

    // Update new expiration
    timer.setRemaining(milliseconds(value));

    // Update Base class data.
    return WatchdogInherits::timeRemaining(value);
}

// Set value of Interval
uint64_t Watchdog::interval(uint64_t value)
{
    return WatchdogInherits::interval(std::max(value, minInterval));
}

// Optional callback function on timer expiration
void Watchdog::timeOutHandler()
{
    Action action = expireAction();
    if (!this->enabled())
    {
        action = fallback->action;
    }

    expiredTimerUse(currentTimerUse());

    auto target = actionTargetMap.find(action);
    if (target == actionTargetMap.end())
    {
        log<level::INFO>("watchdog: Timed out with no target",
                         entry("ACTION=%s", convertForMessage(action).c_str()),
                         entry("TIMER_USE=%s",
                               convertForMessage(expiredTimerUse()).c_str()));
    }
    else
    {
        log<level::INFO>(
            "watchdog: Timed out",
            entry("ACTION=%s", convertForMessage(action).c_str()),
            entry("TIMER_USE=%s", convertForMessage(expiredTimerUse()).c_str()),
            entry("TARGET=%s", target->second.c_str()));

        try
        {
            auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_ROOT,
                                              SYSTEMD_INTERFACE, "StartUnit");
            method.append(target->second);
            method.append("replace");

            bus.call_noreply(method);
        }
        catch (const SdBusError& e)
        {
            log<level::ERR>("watchdog: Failed to start unit",
                            entry("TARGET=%s", target->second.c_str()),
                            entry("ERROR=%s", e.what()));
            commit<InternalFailure>();
        }
    }

    tryFallbackOrDisable();
}

void Watchdog::tryFallbackOrDisable()
{
    // We only re-arm the watchdog if we were already enabled and have
    // a possible fallback
    if (fallback && (fallback->always || this->enabled()))
    {
        auto interval_ms = fallback->interval;
        timer.restart(milliseconds(interval_ms));
        log<level::INFO>("watchdog: falling back",
                         entry("INTERVAL=%llu", interval_ms));
    }
    else if (timerEnabled())
    {
        timer.setEnabled(false);

        log<level::INFO>("watchdog: disabled");
    }

    // Make sure we accurately reflect our enabled state to the
    // dbus interface.
    WatchdogInherits::enabled(false);
}

} // namespace watchdog
} // namespace phosphor
