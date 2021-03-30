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
#include "anyof.hpp"

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
AnyOf::AnyOf(const Fan& fan,
             const std::vector<std::reference_wrapper<PresenceSensor>>& s) :
    RedundancyPolicy(fan),
    state()
{
    for (auto& sensor : s)
    {
        state.emplace_back(sensor, false);
    }
}

void AnyOf::stateChanged(bool present, PresenceSensor& sensor)
{
    // Find the sensor that changed state.
    auto sit =
        std::find_if(state.begin(), state.end(), [&sensor](const auto& s) {
            return std::get<0>(s).get() == sensor;
        });
    if (sit != state.end())
    {
        // Update our cache of the sensors state and re-evaluate.
        std::get<bool>(*sit) = present;
        auto newState =
            std::any_of(state.begin(), state.end(),
                        [](const auto& s) { return std::get<bool>(s); });
        setPresence(fan, newState);
    }
}

void AnyOf::monitor()
{
    // Start all sensors in the anyof redundancy set.

    for (auto& s : state)
    {
        auto& sensor = std::get<0>(s);
        std::get<bool>(s) = sensor.get().start();
    }

    auto present = std::any_of(state.begin(), state.end(),
                               [](const auto& s) { return std::get<bool>(s); });
    setPresence(fan, present);
}

} // namespace presence
} // namespace fan
} // namespace phosphor
