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
#include "dbuspassive.hpp"

#include "dbuspassiveredundancy.hpp"
#include "util.hpp"

#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <sdbusplus/bus.hpp>
#include <string>
#include <variant>

std::unique_ptr<ReadInterface> DbusPassive::createDbusPassive(
    sdbusplus::bus::bus& bus, const std::string& type, const std::string& id,
    DbusHelperInterface* helper, const conf::SensorConfig* info,
    const std::shared_ptr<DbusPassiveRedundancy>& redundancy)
{
    if (helper == nullptr)
    {
        return nullptr;
    }
    if (!validType(type))
    {
        return nullptr;
    }

    /* Need to get the scale and initial value */
    auto tempBus = sdbusplus::bus::new_system();

    /* service == busname */
    std::string path = getSensorPath(type, id);

    struct SensorProperties settings;
    bool failed;

    try
    {
        std::string service = helper->getService(tempBus, sensorintf, path);

        helper->getProperties(tempBus, service, path, &settings);
        failed = helper->thresholdsAsserted(tempBus, service, path);
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }

    /* if these values are zero, they're ignored. */
    if (info->ignoreDbusMinMax)
    {
        settings.min = 0;
        settings.max = 0;
    }

    return std::make_unique<DbusPassive>(bus, type, id, helper, settings,
                                         failed, path, redundancy);
}

DbusPassive::DbusPassive(
    sdbusplus::bus::bus& bus, const std::string& type, const std::string& id,
    DbusHelperInterface* helper, const struct SensorProperties& settings,
    bool failed, const std::string& path,
    const std::shared_ptr<DbusPassiveRedundancy>& redundancy) :
    ReadInterface(),
    _bus(bus), _signal(bus, getMatch(type, id).c_str(), dbusHandleSignal, this),
    _id(id), _helper(helper), _failed(failed), path(path),
    redundancy(redundancy)

{
    _scale = settings.scale;
    _value = settings.value * pow(10, _scale);
    _min = settings.min * pow(10, _scale);
    _max = settings.max * pow(10, _scale);
    _updated = std::chrono::high_resolution_clock::now();
}

ReadReturn DbusPassive::read(void)
{
    std::lock_guard<std::mutex> guard(_lock);

    struct ReadReturn r = {_value, _updated};

    return r;
}

void DbusPassive::setValue(double value)
{
    std::lock_guard<std::mutex> guard(_lock);

    _value = value;
    _updated = std::chrono::high_resolution_clock::now();
}

bool DbusPassive::getFailed(void) const
{
    if (redundancy)
    {
        const std::set<std::string>& failures = redundancy->getFailed();
        if (failures.find(path) != failures.end())
        {
            return true;
        }
    }

    return _failed || !_functional;
}

void DbusPassive::setFailed(bool value)
{
    _failed = value;
}

void DbusPassive::setFunctional(bool value)
{
    _functional = value;
}

int64_t DbusPassive::getScale(void)
{
    return _scale;
}

std::string DbusPassive::getID(void)
{
    return _id;
}

double DbusPassive::getMax(void)
{
    return _max;
}

double DbusPassive::getMin(void)
{
    return _min;
}

int handleSensorValue(sdbusplus::message::message& msg, DbusPassive* owner)
{
    std::string msgSensor;
    std::map<std::string, std::variant<int64_t, double, bool>> msgData;

    msg.read(msgSensor, msgData);

    if (msgSensor == "xyz.openbmc_project.Sensor.Value")
    {
        auto valPropMap = msgData.find("Value");
        if (valPropMap != msgData.end())
        {
            double value =
                std::visit(VariantToDoubleVisitor(), valPropMap->second);

            value *= std::pow(10, owner->getScale());

            scaleSensorReading(owner->getMin(), owner->getMax(), value);

            owner->setValue(value);
        }
    }
    else if (msgSensor == "xyz.openbmc_project.Sensor.Threshold.Critical")
    {
        auto criticalAlarmLow = msgData.find("CriticalAlarmLow");
        auto criticalAlarmHigh = msgData.find("CriticalAlarmHigh");
        if (criticalAlarmHigh == msgData.end() &&
            criticalAlarmLow == msgData.end())
        {
            return 0;
        }

        bool asserted = false;
        if (criticalAlarmLow != msgData.end())
        {
            asserted = std::get<bool>(criticalAlarmLow->second);
        }

        // checking both as in theory you could de-assert one threshold and
        // assert the other at the same moment
        if (!asserted && criticalAlarmHigh != msgData.end())
        {
            asserted = std::get<bool>(criticalAlarmHigh->second);
        }
        owner->setFailed(asserted);
    }
    else if (msgSensor ==
             "xyz.openbmc_project.State.Decorator.OperationalStatus")
    {
        auto functional = msgData.find("Functional");
        if (functional == msgData.end())
        {
            return 0;
        }
        bool asserted = std::get<bool>(functional->second);
        owner->setFunctional(asserted);
    }

    return 0;
}

int dbusHandleSignal(sd_bus_message* msg, void* usrData, sd_bus_error* err)
{
    auto sdbpMsg = sdbusplus::message::message(msg);
    DbusPassive* obj = static_cast<DbusPassive*>(usrData);

    return handleSensorValue(sdbpMsg, obj);
}
