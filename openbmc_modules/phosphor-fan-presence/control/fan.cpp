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
#include "fan.hpp"

#include "sdbusplus.hpp"

#include <string>

namespace phosphor
{
namespace fan
{
namespace control
{

// For throwing exception
using namespace phosphor::logging;

constexpr auto FAN_SENSOR_PATH = "/xyz/openbmc_project/sensors/fan_tach/";
constexpr auto FAN_TARGET_PROPERTY = "Target";

Fan::Fan(sdbusplus::bus::bus& bus, const FanDefinition& def) :
    _bus(bus), _name(std::get<fanNamePos>(def)),
    _interface(std::get<targetInterfacePos>(def))
{
    std::string path;
    auto sensors = std::get<sensorListPos>(def);
    for (auto& s : sensors)
    {
        path = FAN_SENSOR_PATH + s;
        auto service = util::SDBusPlus::getService(bus, path, _interface);
        _sensors[path] = service;
    }
    // All sensors associated with this fan are set to the same target speed,
    // so only need to read target property from one.
    if (!path.empty())
    {
        // Use getProperty with service lookup since each target sensor
        // path given could have different services providing them
        _targetSpeed = util::SDBusPlus::getProperty<uint64_t>(
            bus, path, _interface, FAN_TARGET_PROPERTY);
    }
}

void Fan::setSpeed(uint64_t speed)
{
    for (auto& sensor : _sensors)
    {
        auto value = speed;
        try
        {
            util::SDBusPlus::setProperty<uint64_t>(
                _bus, sensor.second, sensor.first, _interface,
                FAN_TARGET_PROPERTY, std::move(value));
        }
        catch (const sdbusplus::exception::SdBusError&)
        {
            throw util::DBusPropertyError{"DBus set property failed",
                                          sensor.second, sensor.first,
                                          _interface, FAN_TARGET_PROPERTY};
        }
    }

    _targetSpeed = speed;
}

} // namespace control
} // namespace fan
} // namespace phosphor
