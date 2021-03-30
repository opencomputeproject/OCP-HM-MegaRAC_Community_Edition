/**
 * Copyright 2017 Google Inc.
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

#include "pidloop.hpp"

#include "pid/pidcontroller.hpp"
#include "pid/tuning.hpp"
#include "sensors/sensor.hpp"

#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <map>
#include <memory>
#include <thread>
#include <vector>

static void processThermals(PIDZone* zone)
{
    // Get the latest margins.
    zone->updateSensors();
    // Zero out the set point goals.
    zone->clearSetPoints();
    zone->clearRPMCeilings();
    // Run the margin PIDs.
    zone->processThermals();
    // Get the maximum RPM setpoint.
    zone->determineMaxSetPointRequest();
}

void pidControlLoop(PIDZone* zone, boost::asio::steady_timer& timer, bool first,
                    int ms100cnt)
{
    if (first)
    {
        if (loggingEnabled)
        {
            zone->initializeLog();
        }

        zone->initializeCache();
        processThermals(zone);
    }

    timer.expires_after(std::chrono::milliseconds(100));
    timer.async_wait(
        [zone, &timer, ms100cnt](const boost::system::error_code& ec) mutable {
            if (ec == boost::asio::error::operation_aborted)
            {
                return; // timer being canceled, stop loop
            }

            /*
             * This should sleep on the conditional wait for the listen thread
             * to tell us it's in sync.  But then we also need a timeout option
             * in case phosphor-hwmon is down, we can go into some weird failure
             * more.
             *
             * Another approach would be to start all sensors in worst-case
             * values, and fail-safe mode and then clear out of fail-safe mode
             * once we start getting values.  Which I think it is a solid
             * approach.
             *
             * For now this runs before it necessarily has any sensor values.
             * For the host sensors they start out in fail-safe mode.  For the
             * fans, they start out as 0 as input and then are adjusted once
             * they have values.
             *
             * If a fan has failed, it's value will be whatever we're told or
             * however we retrieve it.  This program disregards fan values of 0,
             * so any code providing a fan speed can set to 0 on failure and
             * that fan value will be effectively ignored.  The PID algorithm
             * will be unhappy but nothing bad will happen.
             *
             * TODO(venture): If the fan value is 0 should that loop just be
             * skipped? Right now, a 0 value is ignored in
             * FanController::inputProc()
             */

            // Check if we should just go back to sleep.
            if (zone->getManualMode())
            {
                pidControlLoop(zone, timer, false, ms100cnt);
                return;
            }

            // Get the latest fan speeds.
            zone->updateFanTelemetry();

            if (10 <= ms100cnt)
            {
                ms100cnt = 0;

                processThermals(zone);
            }

            // Run the fan PIDs every iteration.
            zone->processFans();

            if (loggingEnabled)
            {
                zone->getLogHandle() << "," << zone->getFailSafeMode();
                zone->getLogHandle() << std::endl;
            }

            ms100cnt += 1;

            pidControlLoop(zone, timer, false, ms100cnt);
        });
}
