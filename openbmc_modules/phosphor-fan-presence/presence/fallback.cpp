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
#include "fallback.hpp"

#include "fan.hpp"
#include "psensor.hpp"

#include <phosphor-logging/log.hpp>

#include <algorithm>

namespace phosphor
{
namespace fan
{
namespace presence
{

void Fallback::stateChanged(bool present, PresenceSensor& sensor)
{
    if (!present)
    {
        // Starting with the first backup, find the first
        // sensor that reports the fan as present, if any.
        auto it = std::find_if(std::next(activeSensor), sensors.end(),
                               [](auto& s) { return s.get().present(); });

        if (it != sensors.end())
        {
            // A backup sensor disagrees with the active sensor.
            // Switch to the backup.
            activeSensor->get().stop();
            present = it->get().start();

            while (activeSensor != it)
            {
                // Callout the broken sensors.
                activeSensor->get().fail();
                ++activeSensor;
            }
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "Using backup presence sensor.",
                phosphor::logging::entry("FAN=%s", std::get<1>(fan).c_str()));
            activeSensor = it;
        }
    }

    setPresence(fan, present);
}

void Fallback::monitor()
{
    // Find the first sensor that says the fan is present
    // and set it as the active sensor.
    activeSensor = std::find_if(sensors.begin(), sensors.end(),
                                [](auto& s) { return s.get().present(); });
    if (activeSensor == sensors.end())
    {
        // The first sensor is working or all sensors
        // agree the fan isn't present.  Use the first sensor.
        activeSensor = sensors.begin();
    }

    if (activeSensor != sensors.begin())
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Using backup presence sensor.",
            phosphor::logging::entry("FAN=%s", std::get<1>(fan).c_str()));
    }

    // Callout the broken sensors.
    auto it = sensors.begin();
    while (it != activeSensor)
    {
        it->get().fail();
        ++it;
    }

    // Start the active sensor and set the initial state.
    setPresence(fan, activeSensor->get().start());
}

} // namespace presence
} // namespace fan
} // namespace phosphor
