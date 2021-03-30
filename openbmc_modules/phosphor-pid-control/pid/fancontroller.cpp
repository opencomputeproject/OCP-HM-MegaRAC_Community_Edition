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

#include "fancontroller.hpp"

#include "tuning.hpp"
#include "util.hpp"
#include "zone.hpp"

#include <algorithm>
#include <iostream>

std::unique_ptr<PIDController>
    FanController::createFanPid(ZoneInterface* owner, const std::string& id,
                                const std::vector<std::string>& inputs,
                                const ec::pidinfo& initial)
{
    if (inputs.size() == 0)
    {
        return nullptr;
    }
    auto fan = std::make_unique<FanController>(id, inputs, owner);
    ec::pid_info_t* info = fan->getPIDInfo();

    initializePIDStruct(info, initial);

    return fan;
}

double FanController::inputProc(void)
{
    double value = 0;
    std::vector<int64_t> values;
    std::vector<int64_t>::iterator result;

    try
    {
        for (const auto& name : _inputs)
        {
            value = _owner->getCachedValue(name);
            /* If we have a fan we can't read, its value will be 0 for at least
             * some boards, while others... the fan will drop off dbus (if
             * that's how it's being read and in that case its value will never
             * be updated anymore, which is relatively harmless, except, when
             * something tries to read its value through IPMI, and can't, they
             * sort of have to guess -- all the other fans are reporting, why
             * not this one?  Maybe it's unable to be read, so it's "bad."
             */
            if (value > 0)
            {
                values.push_back(value);
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "exception on inputProc.\n";
        throw;
    }

    /* Reset the value from the above loop. */
    value = 0;
    if (values.size() > 0)
    {
        /* the fan PID algorithm was unstable with average, and seemed to work
         * better with minimum.  I had considered making this choice a variable
         * in the configuration, and it's a nice-to-have..
         */
        result = std::min_element(values.begin(), values.end());
        value = *result;
    }

    return value;
}

double FanController::setptProc(void)
{
    double maxRPM = _owner->getMaxSetPointRequest();

    // store for reference, and check if more or less.
    double prev = getSetpoint();

    if (maxRPM > prev)
    {
        setFanDirection(FanSpeedDirection::UP);
    }
    else if (prev > maxRPM)
    {
        setFanDirection(FanSpeedDirection::DOWN);
    }
    else
    {
        setFanDirection(FanSpeedDirection::NEUTRAL);
    }

    setSetpoint(maxRPM);

    return (maxRPM);
}

void FanController::outputProc(double value)
{
    double percent = value;

    /* If doing tuning, don't go into failsafe mode. */
    if (!tuningEnabled)
    {
        if (_owner->getFailSafeMode())
        {
            /* In case it's being set to 100% */
            if (percent < _owner->getFailSafePercent())
            {
                percent = _owner->getFailSafePercent();
            }
        }
    }

    // value and kFanFailSafeDutyCycle are 10 for 10% so let's fix that.
    percent /= 100;

    // PidSensorMap for writing.
    for (const auto& it : _inputs)
    {
        auto sensor = _owner->getSensor(it);
        sensor->write(percent);
    }

    return;
}
