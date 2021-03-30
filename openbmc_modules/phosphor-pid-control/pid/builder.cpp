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

#include "pid/builder.hpp"

#include "conf.hpp"
#include "pid/controller.hpp"
#include "pid/fancontroller.hpp"
#include "pid/stepwisecontroller.hpp"
#include "pid/thermalcontroller.hpp"

#include <iostream>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <unordered_map>

static constexpr bool deferSignals = true;
static constexpr auto objectPath = "/xyz/openbmc_project/settings/fanctrl/zone";

static std::string getControlPath(int64_t zone)
{
    return std::string(objectPath) + std::to_string(zone);
}

std::unordered_map<int64_t, std::unique_ptr<PIDZone>>
    buildZones(std::map<int64_t, conf::PIDConf>& zonePids,
               std::map<int64_t, struct conf::ZoneConfig>& zoneConfigs,
               SensorManager& mgr, sdbusplus::bus::bus& modeControlBus)
{
    std::unordered_map<int64_t, std::unique_ptr<PIDZone>> zones;

    for (const auto& zi : zonePids)
    {
        auto zoneId = static_cast<int64_t>(zi.first);
        /* The above shouldn't be necessary but is, and I am having trouble
         * locating my notes on why.  If I recall correctly it was casting it
         * down to a byte in at least some cases causing weird behaviors.
         */

        auto zoneConf = zoneConfigs.find(zoneId);
        if (zoneConf == zoneConfigs.end())
        {
            /* The Zone doesn't have a configuration, bail. */
            static constexpr auto err =
                "Bailing during load, missing Zone Configuration";
            std::cerr << err << std::endl;
            throw std::runtime_error(err);
        }

        const conf::PIDConf& pidConfig = zi.second;

        auto zone = std::make_unique<PIDZone>(
            zoneId, zoneConf->second.minThermalOutput,
            zoneConf->second.failsafePercent, mgr, modeControlBus,
            getControlPath(zi.first).c_str(), deferSignals);

        std::cerr << "Zone Id: " << zone->getZoneID() << "\n";

        // For each PID create a Controller and a Sensor.
        for (const auto& pit : pidConfig)
        {
            std::vector<std::string> inputs;
            std::string name = pit.first;
            const struct conf::ControllerInfo* info = &pit.second;

            std::cerr << "PID name: " << name << "\n";

            /*
             * TODO(venture): Need to check if input is known to the
             * SensorManager.
             */
            if (info->type == "fan")
            {
                for (const auto& i : info->inputs)
                {
                    inputs.push_back(i);
                    zone->addFanInput(i);
                }

                auto pid = FanController::createFanPid(zone.get(), name, inputs,
                                                       info->pidInfo);
                zone->addFanPID(std::move(pid));
            }
            else if (isThermalType(info->type))
            {
                for (const auto& i : info->inputs)
                {
                    inputs.push_back(i);
                    zone->addThermalInput(i);
                }

                auto pid = ThermalController::createThermalPid(
                    zone.get(), name, inputs, info->setpoint, info->pidInfo,
                    getThermalType(info->type));

                zone->addThermalPID(std::move(pid));
            }
            else if (info->type == "stepwise")
            {
                for (const auto& i : info->inputs)
                {
                    inputs.push_back(i);
                    zone->addThermalInput(i);
                }
                auto stepwise = StepwiseController::createStepwiseController(
                    zone.get(), name, inputs, info->stepwiseInfo);
                zone->addThermalPID(std::move(stepwise));
            }

            std::cerr << "inputs: ";
            for (const auto& i : inputs)
            {
                std::cerr << i << ", ";
            }
            std::cerr << "\n";
        }

        zone->emit_object_added();
        zones[zoneId] = std::move(zone);
    }

    return zones;
}
