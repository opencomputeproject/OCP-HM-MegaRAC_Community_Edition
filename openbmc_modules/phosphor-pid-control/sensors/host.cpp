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

#include "host.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>

template <typename T>
void scaleHelper(T& ptr, int64_t value)
{
    if constexpr (std::is_same_v<ValueType, int64_t>)
    {
        ptr->scale(value);
    }
}

std::unique_ptr<Sensor> HostSensor::createTemp(const std::string& name,
                                               int64_t timeout,
                                               sdbusplus::bus::bus& bus,
                                               const char* objPath, bool defer)
{
    auto sensor =
        std::make_unique<HostSensor>(name, timeout, bus, objPath, defer);
    sensor->value(0);

    // DegreesC and value of 0 are the defaults at present, therefore testing
    // this code only sees scale get updated as a property.

    // TODO(venture): Need to not hard-code that this is DegreesC and scale
    // 10x-3 unless it is! :D
    sensor->unit(ValueInterface::Unit::DegreesC);
    scaleHelper(sensor, -3);
    sensor->emit_object_added();
    // emit_object_added() can be called twice, harmlessly, the second time it
    // doesn't actually happen, but we don't want to call it before we set up
    // the initial values, so we should not let someone call this with
    // defer=false.

    /* TODO(venture): Need to set that _updated is set to epoch or something
     * else.  what is the default value?
     */
    return sensor;
}

template <typename T>
int64_t getScale(T* sensor)
{
    if constexpr (std::is_same_v<ValueType, int64_t>)
    {
        return sensor->scale();
    }
    return 0;
}

ValueType HostSensor::value(ValueType value)
{
    std::lock_guard<std::mutex> guard(_lock);

    _updated = std::chrono::high_resolution_clock::now();
    _value = value * pow(10, getScale(this)); /* scale value */

    return ValueObject::value(value);
}

ReadReturn HostSensor::read(void)
{
    std::lock_guard<std::mutex> guard(_lock);

    /* This doesn't sanity check anything, that's the caller's job. */
    struct ReadReturn r = {_value, _updated};

    return r;
}

void HostSensor::write(double value)
{
    throw std::runtime_error("Not Implemented.");
}
