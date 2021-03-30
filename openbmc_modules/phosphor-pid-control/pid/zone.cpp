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

/* Configuration. */
#include "zone.hpp"

#include "conf.hpp"
#include "pid/controller.hpp"
#include "pid/ec/pid.hpp"
#include "pid/fancontroller.hpp"
#include "pid/stepwisecontroller.hpp"
#include "pid/thermalcontroller.hpp"
#include "pid/tuning.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>

using tstamp = std::chrono::high_resolution_clock::time_point;
using namespace std::literals::chrono_literals;

double PIDZone::getMaxSetPointRequest(void) const
{
    return _maximumSetPoint;
}

bool PIDZone::getManualMode(void) const
{
    return _manualMode;
}

void PIDZone::setManualMode(bool mode)
{
    _manualMode = mode;
}

bool PIDZone::getFailSafeMode(void) const
{
    // If any keys are present at least one sensor is in fail safe mode.
    return !_failSafeSensors.empty();
}

int64_t PIDZone::getZoneID(void) const
{
    return _zoneId;
}

void PIDZone::addSetPoint(double setpoint)
{
    _SetPoints.push_back(setpoint);
}

void PIDZone::addRPMCeiling(double ceiling)
{
    _RPMCeilings.push_back(ceiling);
}

void PIDZone::clearRPMCeilings(void)
{
    _RPMCeilings.clear();
}

void PIDZone::clearSetPoints(void)
{
    _SetPoints.clear();
}

double PIDZone::getFailSafePercent(void) const
{
    return _failSafePercent;
}

double PIDZone::getMinThermalSetpoint(void) const
{
    return _minThermalOutputSetPt;
}

void PIDZone::addFanPID(std::unique_ptr<Controller> pid)
{
    _fans.push_back(std::move(pid));
}

void PIDZone::addThermalPID(std::unique_ptr<Controller> pid)
{
    _thermals.push_back(std::move(pid));
}

double PIDZone::getCachedValue(const std::string& name)
{
    return _cachedValuesByName.at(name);
}

void PIDZone::addFanInput(const std::string& fan)
{
    _fanInputs.push_back(fan);
}

void PIDZone::addThermalInput(const std::string& therm)
{
    _thermalInputs.push_back(therm);
}

void PIDZone::determineMaxSetPointRequest(void)
{
    double max = 0;
    std::vector<double>::iterator result;

    if (_SetPoints.size() > 0)
    {
        result = std::max_element(_SetPoints.begin(), _SetPoints.end());
        max = *result;
    }

    if (_RPMCeilings.size() > 0)
    {
        result = std::min_element(_RPMCeilings.begin(), _RPMCeilings.end());
        max = std::min(max, *result);
    }

    /*
     * If the maximum RPM setpoint output is below the minimum RPM
     * setpoint, set it to the minimum.
     */
    max = std::max(getMinThermalSetpoint(), max);

    if (tuningEnabled)
    {
        /*
         * We received no setpoints from thermal sensors.
         * This is a case experienced during tuning where they only specify
         * fan sensors and one large fan PID for all the fans.
         */
        static constexpr auto setpointpath = "/etc/thermal.d/setpoint";
        try
        {
            std::ifstream ifs;
            ifs.open(setpointpath);
            if (ifs.good())
            {
                int value;
                ifs >> value;

                /* expecting RPM setpoint, not pwm% */
                max = static_cast<double>(value);
            }
        }
        catch (const std::exception& e)
        {
            /* This exception is uninteresting. */
            std::cerr << "Unable to read from '" << setpointpath << "'\n";
        }
    }

    _maximumSetPoint = max;
    return;
}

void PIDZone::initializeLog(void)
{
    /* Print header for log file:
     * epoch_ms,setpt,fan1,fan2,fanN,sensor1,sensor2,sensorN,failsafe
     */

    _log << "epoch_ms,setpt";

    for (const auto& f : _fanInputs)
    {
        _log << "," << f;
    }
    for (const auto& t : _thermalInputs)
    {
        _log << "," << t;
    }
    _log << ",failsafe";
    _log << std::endl;

    return;
}

std::ofstream& PIDZone::getLogHandle(void)
{
    return _log;
}

/*
 * TODO(venture) This is effectively updating the cache and should check if the
 * values they're using to update it are new or old, or whatnot.  For instance,
 * if we haven't heard from the host in X time we need to detect this failure.
 *
 * I haven't decided if the Sensor should have a lastUpdated method or whether
 * that should be for the ReadInterface or etc...
 */

/**
 * We want the PID loop to run with values cached, so this will get all the
 * fan tachs for the loop.
 */
void PIDZone::updateFanTelemetry(void)
{
    /* TODO(venture): Should I just make _log point to /dev/null when logging
     * is disabled?  I think it's a waste to try and log things even if the
     * data is just being dropped though.
     */
    tstamp now = std::chrono::high_resolution_clock::now();
    if (loggingEnabled)
    {
        _log << std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch())
                    .count();
        _log << "," << _maximumSetPoint;
    }

    for (const auto& f : _fanInputs)
    {
        auto sensor = _mgr.getSensor(f);
        ReadReturn r = sensor->read();
        _cachedValuesByName[f] = r.value;
        int64_t timeout = sensor->getTimeout();
        tstamp then = r.updated;

        auto duration =
            std::chrono::duration_cast<std::chrono::seconds>(now - then)
                .count();
        auto period = std::chrono::seconds(timeout).count();
        /*
         * TODO(venture): We should check when these were last read.
         * However, these are the fans, so if I'm not getting updated values
         * for them... what should I do?
         */
        if (loggingEnabled)
        {
            _log << "," << r.value;
        }

        // check if fan fail.
        if (sensor->getFailed())
        {
            _failSafeSensors.insert(f);
        }
        else if (timeout != 0 && duration >= period)
        {
            _failSafeSensors.insert(f);
        }
        else
        {
            // Check if it's in there: remove it.
            auto kt = _failSafeSensors.find(f);
            if (kt != _failSafeSensors.end())
            {
                _failSafeSensors.erase(kt);
            }
        }
    }

    if (loggingEnabled)
    {
        for (const auto& t : _thermalInputs)
        {
            _log << "," << _cachedValuesByName[t];
        }
    }

    return;
}

void PIDZone::updateSensors(void)
{
    using namespace std::chrono;
    /* margin and temp are stored as temp */
    tstamp now = high_resolution_clock::now();

    for (const auto& t : _thermalInputs)
    {
        auto sensor = _mgr.getSensor(t);
        ReadReturn r = sensor->read();
        int64_t timeout = sensor->getTimeout();

        _cachedValuesByName[t] = r.value;
        tstamp then = r.updated;

        auto duration = duration_cast<std::chrono::seconds>(now - then).count();
        auto period = std::chrono::seconds(timeout).count();

        if (sensor->getFailed())
        {
            _failSafeSensors.insert(t);
        }
        else if (timeout != 0 && duration >= period)
        {
            // std::cerr << "Entering fail safe mode.\n";
            _failSafeSensors.insert(t);
        }
        else
        {
            // Check if it's in there: remove it.
            auto kt = _failSafeSensors.find(t);
            if (kt != _failSafeSensors.end())
            {
                _failSafeSensors.erase(kt);
            }
        }
    }

    return;
}

void PIDZone::initializeCache(void)
{
    for (const auto& f : _fanInputs)
    {
        _cachedValuesByName[f] = 0;

        // Start all fans in fail-safe mode.
        _failSafeSensors.insert(f);
    }

    for (const auto& t : _thermalInputs)
    {
        _cachedValuesByName[t] = 0;

        // Start all sensors in fail-safe mode.
        _failSafeSensors.insert(t);
    }
}

void PIDZone::dumpCache(void)
{
    std::cerr << "Cache values now: \n";
    for (const auto& k : _cachedValuesByName)
    {
        std::cerr << k.first << ": " << k.second << "\n";
    }
}

void PIDZone::processFans(void)
{
    for (auto& p : _fans)
    {
        p->process();
    }
}

void PIDZone::processThermals(void)
{
    for (auto& p : _thermals)
    {
        p->process();
    }
}

Sensor* PIDZone::getSensor(const std::string& name)
{
    return _mgr.getSensor(name);
}

bool PIDZone::manual(bool value)
{
    std::cerr << "manual: " << value << std::endl;
    setManualMode(value);
    return ModeObject::manual(value);
}

bool PIDZone::failSafe() const
{
    return getFailSafeMode();
}
