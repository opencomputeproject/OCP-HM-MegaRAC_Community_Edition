/**
 * Copyright © 2017 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "config.h"

#include "pgood_monitor.hpp"

#include "elog-errors.hpp"
#include "utility.hpp"

#include <org/open_power/Witherspoon/Fault/error.hpp>
#include <phosphor-logging/log.hpp>

namespace witherspoon
{
namespace power
{

constexpr auto POWER_OBJ_PATH = "/org/openbmc/control/power0";
constexpr auto POWER_INTERFACE = "org.openbmc.control.Power";

using namespace phosphor::logging;
using namespace sdbusplus::org::open_power::Witherspoon::Fault::Error;

bool PGOODMonitor::pgoodPending()
{
    bool pending = false;
    int32_t state = 0;
    int32_t pgood = 0;

    auto service = util::getService(POWER_OBJ_PATH, POWER_INTERFACE, bus);

    util::getProperty<int32_t>(POWER_INTERFACE, "pgood", POWER_OBJ_PATH,
                               service, bus, pgood);

    // When state = 1, system was switched on
    util::getProperty<int32_t>(POWER_INTERFACE, "state", POWER_OBJ_PATH,
                               service, bus, state);

    // On but no PGOOD
    if (state && !pgood)
    {
        pending = true;
    }

    return pending;
}

void PGOODMonitor::analyze()
{
    // Timer callback.
    // The timer expired before it was stopped.
    // If PGOOD is still pending (it should be),
    // then there is a real failure.

    if (pgoodPending())
    {
#ifdef UCD90160_DEVICE_ACCESS
        device->onFailure();
#endif
        report<PowerOnFailure>();
    }

    // The pgood-wait service (with a longer timeout)
    // will handle powering off the system.
    timer.get_event().exit(EXIT_SUCCESS);
}

void PGOODMonitor::propertyChanged()
{
    // Multiple properties could have changed here.
    // Keep things simple and just recheck the important ones.
    if (!pgoodPending())
    {
        // PGOOD is on, or system is off, so we are done.
        timer.get_event().exit(EXIT_SUCCESS);
    }
}

void PGOODMonitor::startListening()
{
    match = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::propertiesChanged(POWER_OBJ_PATH,
                                                        POWER_INTERFACE),
        [this](auto&) { this->propertyChanged(); });
}

int PGOODMonitor::run()
{
    try
    {
        startListening();

        // If PGOOD came up before we got here, we're done.
        // Otherwise if PGOOD doesn't get asserted before
        // the timer expires, it's a failure.
        if (!pgoodPending())
        {
            return EXIT_SUCCESS;
        }

        return timer.get_event().loop();
    }
    catch (std::exception& e)
    {
        log<level::ERR>(e.what());
        log<level::ERR>("Unexpected failure prevented PGOOD checking");
    }

    // Letting the service fail won't help anything, so don't do it.
    return EXIT_SUCCESS;
}

} // namespace power
} // namespace witherspoon
