/**
 * Copyright © 2020 IBM Corporation
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
#include "json_parser.hpp"

#include "conditions.hpp"
#include "json_config.hpp"
#include "nonzero_speed_trust.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;
using namespace phosphor::logging;

namespace tClass
{

// Get a constructed trust group class for a non-zero speed group
CreateGroupFunction
    getNonZeroSpeed(const std::vector<trust::GroupDefinition>& group)
{
    return [group]() {
        return std::make_unique<trust::NonzeroSpeed>(std::move(group));
    };
}

} // namespace tClass

const std::map<std::string, trustHandler> trusts = {
    {"nonzerospeed", tClass::getNonZeroSpeed}};
const std::map<std::string, condHandler> conditions = {
    {"propertiesmatch", condition::getPropertiesMatch}};

const std::vector<CreateGroupFunction> getTrustGrps(const json& obj)
{
    std::vector<CreateGroupFunction> grpFuncs;

    if (obj.contains("sensor_trust_groups"))
    {
        for (auto& stg : obj["sensor_trust_groups"])
        {
            if (!stg.contains("class") || !stg.contains("group"))
            {
                // Log error on missing required parameters
                log<level::ERR>(
                    "Missing required fan monitor trust group parameters",
                    entry("REQUIRED_PARAMETERS=%s", "{class, group}"));
                throw std::runtime_error(
                    "Missing required fan trust group parameters");
            }
            auto tgClass = stg["class"].get<std::string>();
            std::vector<trust::GroupDefinition> group;
            for (auto& member : stg["group"])
            {
                // Construct list of group members
                if (!member.contains("name"))
                {
                    // Log error on missing required parameter
                    log<level::ERR>(
                        "Missing required fan monitor trust group member name",
                        entry("CLASS=%s", tgClass.c_str()));
                    throw std::runtime_error(
                        "Missing required fan monitor trust group member name");
                }
                auto in_trust = true;
                if (member.contains("in_trust"))
                {
                    in_trust = member["in_trust"].get<bool>();
                }
                group.emplace_back(trust::GroupDefinition{
                    member["name"].get<std::string>(), in_trust});
            }
            // The class for fan sensor trust groups
            // (Must have a supported function within the tClass namespace)
            std::transform(tgClass.begin(), tgClass.end(), tgClass.begin(),
                           tolower);
            auto handler = trusts.find(tgClass);
            if (handler != trusts.end())
            {
                // Call function for trust group class
                grpFuncs.emplace_back(handler->second(group));
            }
            else
            {
                // Log error on unsupported trust group class
                log<level::ERR>("Invalid fan monitor trust group class",
                                entry("CLASS=%s", tgClass.c_str()));
                throw std::runtime_error(
                    "Invalid fan monitor trust group class");
            }
        }
    }

    return grpFuncs;
}

const std::vector<SensorDefinition> getSensorDefs(const json& sensors)
{
    std::vector<SensorDefinition> sensorDefs;

    for (const auto& sensor : sensors)
    {
        if (!sensor.contains("name") || !sensor.contains("has_target"))
        {
            // Log error on missing required parameters
            log<level::ERR>(
                "Missing required fan sensor definition parameters",
                entry("REQUIRED_PARAMETERS=%s", "{name, has_target}"));
            throw std::runtime_error(
                "Missing required fan sensor definition parameters");
        }
        // Target interface is optional and defaults to
        // 'xyz.openbmc_project.Control.FanSpeed'
        std::string targetIntf = "xyz.openbmc_project.Control.FanSpeed";
        if (sensor.contains("target_interface"))
        {
            targetIntf = sensor["target_interface"].get<std::string>();
        }
        // Factor is optional and defaults to 1
        auto factor = 1.0;
        if (sensor.contains("factor"))
        {
            factor = sensor["factor"].get<double>();
        }
        // Offset is optional and defaults to 0
        auto offset = 0;
        if (sensor.contains("offset"))
        {
            offset = sensor["offset"].get<int64_t>();
        }

        sensorDefs.emplace_back(std::tuple(sensor["name"].get<std::string>(),
                                           sensor["has_target"].get<bool>(),
                                           targetIntf, factor, offset));
    }

    return sensorDefs;
}

const std::vector<FanDefinition> getFanDefs(const json& obj)
{
    std::vector<FanDefinition> fanDefs;

    for (const auto& fan : obj["fans"])
    {
        if (!fan.contains("inventory") ||
            !fan.contains("allowed_out_of_range_time") ||
            !fan.contains("deviation") ||
            !fan.contains("num_sensors_nonfunc_for_fan_nonfunc") ||
            !fan.contains("sensors"))
        {
            // Log error on missing required parameters
            log<level::ERR>(
                "Missing required fan monitor definition parameters",
                entry("REQUIRED_PARAMETERS=%s",
                      "{inventory, allowed_out_of_range_time, deviation, "
                      "num_sensors_nonfunc_for_fan_nonfunc, sensors}"));
            throw std::runtime_error(
                "Missing required fan monitor definition parameters");
        }
        // Construct the sensor definitions for this fan
        auto sensorDefs = getSensorDefs(fan["sensors"]);

        // Functional delay is optional and defaults to 0
        size_t funcDelay = 0;
        if (fan.contains("functional_delay"))
        {
            funcDelay = fan["functional_delay"].get<size_t>();
        }

        // Handle optional conditions
        auto cond = std::optional<Condition>();
        if (fan.contains("condition"))
        {
            if (!fan["condition"].contains("name"))
            {
                // Log error on missing required parameter
                log<level::ERR>(
                    "Missing required fan monitor condition parameter",
                    entry("REQUIRED_PARAMETER=%s", "{name}"));
                throw std::runtime_error(
                    "Missing required fan monitor condition parameter");
            }
            auto name = fan["condition"]["name"].get<std::string>();
            // The function for fan monitoring condition
            // (Must have a supported function within the condition namespace)
            std::transform(name.begin(), name.end(), name.begin(), tolower);
            auto handler = conditions.find(name);
            if (handler != conditions.end())
            {
                cond = handler->second(fan["condition"]);
            }
            else
            {
                log<level::INFO>(
                    "No handler found for configured condition",
                    entry("CONDITION_NAME=%s", name.c_str()),
                    entry("JSON_DUMP=%s", fan["condition"].dump().c_str()));
            }
        }

        fanDefs.emplace_back(
            std::tuple(fan["inventory"].get<std::string>(), funcDelay,
                       fan["allowed_out_of_range_time"].get<size_t>(),
                       fan["deviation"].get<size_t>(),
                       fan["num_sensors_nonfunc_for_fan_nonfunc"].get<size_t>(),
                       sensorDefs, cond));
    }

    return fanDefs;
}

} // namespace phosphor::fan::monitor
