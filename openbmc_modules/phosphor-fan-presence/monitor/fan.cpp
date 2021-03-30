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
#include "types.hpp"
#include "utility.hpp"

#include <phosphor-logging/log.hpp>

#include <algorithm>

namespace phosphor
{
namespace fan
{
namespace monitor
{

using namespace phosphor::logging;

Fan::Fan(Mode mode, sdbusplus::bus::bus& bus, const sdeventplus::Event& event,
         std::unique_ptr<trust::Manager>& trust, const FanDefinition& def) :
    _bus(bus),
    _name(std::get<fanNameField>(def)),
    _deviation(std::get<fanDeviationField>(def)),
    _numSensorFailsForNonFunc(std::get<numSensorFailsForNonfuncField>(def)),
    _trustManager(trust)
{
    // Setup tach sensors for monitoring
    auto& sensors = std::get<sensorListField>(def);
    for (auto& s : sensors)
    {
        try
        {
            _sensors.emplace_back(std::make_shared<TachSensor>(
                mode, bus, *this, std::get<sensorNameField>(s),
                std::get<hasTargetField>(s), std::get<funcDelay>(def),
                std::get<targetInterfaceField>(s), std::get<factorField>(s),
                std::get<offsetField>(s), std::get<timeoutField>(def), event));

            _trustManager->registerSensor(_sensors.back());
        }
        catch (InvalidSensorError& e)
        {}
    }

    // Start from a known state of functional
    updateInventory(true);

    // Check current tach state when entering monitor mode
    if (mode != Mode::init)
    {
        // The TachSensors will now have already read the input
        // and target values, so check them.
        tachChanged();
    }
}

void Fan::tachChanged()
{
    for (auto& s : _sensors)
    {
        tachChanged(*s);
    }
}

void Fan::tachChanged(TachSensor& sensor)
{
    if (_trustManager->active())
    {
        if (!_trustManager->checkTrust(sensor))
        {
            return;
        }
    }

    // If this sensor is out of range at this moment, start
    // its timer, at the end of which the inventory
    // for the fan may get updated to not functional.

    // If this sensor is OK, put everything back into a good state.

    if (outOfRange(sensor))
    {
        if (sensor.functional())
        {
            // Start nonfunctional timer if not already running
            sensor.startTimer(TimerMode::nonfunc);
        }
    }
    else
    {
        if (sensor.functional())
        {
            sensor.stopTimer();
        }
        else
        {
            // Start functional timer if not already running
            sensor.startTimer(TimerMode::func);
        }
    }
}

uint64_t Fan::findTargetSpeed()
{
    uint64_t target = 0;
    // The sensor doesn't support a target,
    // so get it from another sensor.
    auto s = std::find_if(_sensors.begin(), _sensors.end(),
                          [](const auto& s) { return s->hasTarget(); });

    if (s != _sensors.end())
    {
        target = (*s)->getTarget();
    }

    return target;
}

bool Fan::tooManySensorsNonfunctional()
{
    size_t numFailed =
        std::count_if(_sensors.begin(), _sensors.end(),
                      [](const auto& s) { return !s->functional(); });

    return (numFailed >= _numSensorFailsForNonFunc);
}

bool Fan::outOfRange(const TachSensor& sensor)
{
    auto actual = static_cast<uint64_t>(sensor.getInput());
    auto target = sensor.getTarget();
    auto factor = sensor.getFactor();
    auto offset = sensor.getOffset();

    uint64_t min = target * (100 - _deviation) / 100;
    uint64_t max = target * (100 + _deviation) / 100;

    // TODO: openbmc/openbmc#2937 enhance this function
    // either by making it virtual, or by predefining different
    // outOfRange ops and selecting by yaml config
    min = min * factor + offset;
    max = max * factor + offset;
    if ((actual < min) || (actual > max))
    {
        return true;
    }

    return false;
}

void Fan::timerExpired(TachSensor& sensor)
{
    sensor.setFunctional(!sensor.functional());

    // If the fan was nonfunctional and enough sensors are now OK,
    // the fan can go back to functional
    if (!_functional && !tooManySensorsNonfunctional())
    {
        log<level::INFO>("Setting a fan back to functional",
                         entry("FAN=%s", _name.c_str()));

        updateInventory(true);
    }

    // If the fan is currently functional, but too many
    // contained sensors are now nonfunctional, update
    // the whole fan nonfunctional.
    if (_functional && tooManySensorsNonfunctional())
    {
        log<level::ERR>("Setting a fan to nonfunctional",
                        entry("FAN=%s", _name.c_str()),
                        entry("TACH_SENSOR=%s", sensor.name().c_str()),
                        entry("ACTUAL_SPEED=%lld", sensor.getInput()),
                        entry("TARGET_SPEED=%lld", sensor.getTarget()));

        updateInventory(false);
    }
}

void Fan::updateInventory(bool functional)
{
    auto objectMap =
        util::getObjMap<bool>(_name, util::OPERATIONAL_STATUS_INTF,
                              util::FUNCTIONAL_PROPERTY, functional);
    auto response = util::SDBusPlus::lookupAndCallMethod(
        _bus, util::INVENTORY_PATH, util::INVENTORY_INTF, "Notify", objectMap);
    if (response.is_method_error())
    {
        log<level::ERR>("Error in Notify call to update inventory");
        return;
    }

    // This will always track the current state of the inventory.
    _functional = functional;
}

} // namespace monitor
} // namespace fan
} // namespace phosphor
