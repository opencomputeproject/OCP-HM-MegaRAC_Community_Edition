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
#include "tach_sensor.hpp"

#include "fan.hpp"
#include "sdbusplus.hpp"
#include "utility.hpp"

#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>

#include <experimental/filesystem>
#include <functional>

namespace phosphor
{
namespace fan
{
namespace monitor
{

constexpr auto FAN_SENSOR_VALUE_INTF = "xyz.openbmc_project.Sensor.Value";
constexpr auto FAN_TARGET_PROPERTY = "Target";
constexpr auto FAN_VALUE_PROPERTY = "Value";

using namespace std::experimental::filesystem;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

/**
 * @brief Helper function to read a property
 *
 * @param[in] interface - the interface the property is on
 * @param[in] propertName - the name of the property
 * @param[in] path - the dbus path
 * @param[in] bus - the dbus object
 * @param[out] value - filled in with the property value
 */
template <typename T>
static void
    readProperty(const std::string& interface, const std::string& propertyName,
                 const std::string& path, sdbusplus::bus::bus& bus, T& value)
{
    try
    {
        value =
            util::SDBusPlus::getProperty<T>(bus, path, interface, propertyName);
    }
    catch (std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
    }
}

TachSensor::TachSensor(Mode mode, sdbusplus::bus::bus& bus, Fan& fan,
                       const std::string& id, bool hasTarget, size_t funcDelay,
                       const std::string& interface, double factor,
                       int64_t offset, size_t timeout,
                       const sdeventplus::Event& event) :
    _bus(bus),
    _fan(fan), _name(FAN_SENSOR_PATH + id), _invName(path(fan.getName()) / id),
    _hasTarget(hasTarget), _funcDelay(funcDelay), _interface(interface),
    _factor(factor), _offset(offset), _timeout(timeout),
    _timerMode(TimerMode::func),
    _timer(event, std::bind(&Fan::timerExpired, &fan, std::ref(*this)))
{
    // Start from a known state of functional
    setFunctional(true);

    // Load in current Target and Input values when entering monitor mode
    if (mode != Mode::init)
    {
        try
        {
            // Use getProperty directly to allow a missing sensor object
            // to abort construction.
            _tachInput = util::SDBusPlus::getProperty<decltype(_tachInput)>(
                _bus, _name, FAN_SENSOR_VALUE_INTF, FAN_VALUE_PROPERTY);
        }
        catch (std::exception& e)
        {
            log<level::INFO>("Not monitoring a tach sensor",
                             entry("SENSOR=%s", _name.c_str()));
            throw InvalidSensorError();
        }

        if (_hasTarget)
        {
            readProperty(_interface, FAN_TARGET_PROPERTY, _name, _bus,
                         _tachTarget);
        }

        auto match = getMatchString(FAN_SENSOR_VALUE_INTF);

        tachSignal = std::make_unique<sdbusplus::server::match::match>(
            _bus, match.c_str(),
            [this](auto& msg) { this->handleTachChange(msg); });

        if (_hasTarget)
        {
            match = getMatchString(_interface);

            targetSignal = std::make_unique<sdbusplus::server::match::match>(
                _bus, match.c_str(),
                [this](auto& msg) { this->handleTargetChange(msg); });
        }
    }
}

std::string TachSensor::getMatchString(const std::string& interface)
{
    return sdbusplus::bus::match::rules::propertiesChanged(_name, interface);
}

uint64_t TachSensor::getTarget() const
{
    if (!_hasTarget)
    {
        return _fan.findTargetSpeed();
    }
    return _tachTarget;
}

void TachSensor::setFunctional(bool functional)
{
    _functional = functional;
    updateInventory(_functional);
}

/**
 * @brief Reads a property from the input message and stores it in value.
 *        T is the value type.
 *
 *        Note: This can only be called once per message.
 *
 * @param[in] msg - the dbus message that contains the data
 * @param[in] interface - the interface the property is on
 * @param[in] propertName - the name of the property
 * @param[out] value - the value to store the property value in
 */
template <typename T>
static void readPropertyFromMessage(sdbusplus::message::message& msg,
                                    const std::string& interface,
                                    const std::string& propertyName, T& value)
{
    std::string sensor;
    std::map<std::string, std::variant<T>> data;
    msg.read(sensor, data);

    if (sensor.compare(interface) == 0)
    {
        auto propertyMap = data.find(propertyName);
        if (propertyMap != data.end())
        {
            value = std::get<T>(propertyMap->second);
        }
    }
}

void TachSensor::handleTargetChange(sdbusplus::message::message& msg)
{
    readPropertyFromMessage(msg, _interface, FAN_TARGET_PROPERTY, _tachTarget);

    // Check all tach sensors on the fan against the target
    _fan.tachChanged();
}

void TachSensor::handleTachChange(sdbusplus::message::message& msg)
{
    readPropertyFromMessage(msg, FAN_SENSOR_VALUE_INTF, FAN_VALUE_PROPERTY,
                            _tachInput);

    // Check just this sensor against the target
    _fan.tachChanged(*this);
}

void TachSensor::startTimer(TimerMode mode)
{
    if (!timerRunning() || mode != _timerMode)
    {
        _timer.restartOnce(getDelay(mode));
        _timerMode = mode;
    }
}

std::chrono::microseconds TachSensor::getDelay(TimerMode mode)
{
    using namespace std::chrono;

    switch (mode)
    {
        case TimerMode::nonfunc:
            return duration_cast<microseconds>(seconds(_timeout));
        case TimerMode::func:
            return duration_cast<microseconds>(seconds(_funcDelay));
        default:
            // Log an internal error for undefined timer mode
            log<level::ERR>("Undefined timer mode",
                            entry("TIMER_MODE=%u", mode));
            elog<InternalFailure>();
            return duration_cast<microseconds>(seconds(0));
    }
}

void TachSensor::updateInventory(bool functional)
{
    auto objectMap =
        util::getObjMap<bool>(_invName, util::OPERATIONAL_STATUS_INTF,
                              util::FUNCTIONAL_PROPERTY, functional);
    auto response = util::SDBusPlus::lookupAndCallMethod(
        _bus, util::INVENTORY_PATH, util::INVENTORY_INTF, "Notify", objectMap);
    if (response.is_method_error())
    {
        log<level::ERR>("Error in notify update of tach sensor inventory");
    }
}

} // namespace monitor
} // namespace fan
} // namespace phosphor
