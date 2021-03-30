/**
 * Copyright 2019 Google Inc.
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

#include "pid/buildjson.hpp"

#include "conf.hpp"

#include <map>
#include <nlohmann/json.hpp>
#include <tuple>

using json = nlohmann::json;

namespace conf
{
void from_json(const json& j, conf::ControllerInfo& c)
{
    j.at("type").get_to(c.type);
    j.at("inputs").get_to(c.inputs);
    j.at("setpoint").get_to(c.setpoint);

    /* TODO: We need to handle parsing other PID controller configurations.
     * We can do that by checking for different keys and making the decision
     * accordingly.
     */
    auto p = j.at("pid");

    auto positiveHysteresis = p.find("positiveHysteresis");
    auto negativeHysteresis = p.find("negativeHysteresis");
    auto positiveHysteresisValue = 0.0;
    auto negativeHysteresisValue = 0.0;
    if (positiveHysteresis != p.end())
    {
        p.at("positiveHysteresis").get_to(positiveHysteresisValue);
    }
    if (negativeHysteresis != p.end())
    {
        p.at("negativeHysteresis").get_to(negativeHysteresisValue);
    }

    if (c.type != "stepwise")
    {
        p.at("samplePeriod").get_to(c.pidInfo.ts);
        p.at("proportionalCoeff").get_to(c.pidInfo.proportionalCoeff);
        p.at("integralCoeff").get_to(c.pidInfo.integralCoeff);
        p.at("feedFwdOffsetCoeff").get_to(c.pidInfo.feedFwdOffset);
        p.at("feedFwdGainCoeff").get_to(c.pidInfo.feedFwdGain);
        p.at("integralLimit_min").get_to(c.pidInfo.integralLimit.min);
        p.at("integralLimit_max").get_to(c.pidInfo.integralLimit.max);
        p.at("outLim_min").get_to(c.pidInfo.outLim.min);
        p.at("outLim_max").get_to(c.pidInfo.outLim.max);
        p.at("slewNeg").get_to(c.pidInfo.slewNeg);
        p.at("slewPos").get_to(c.pidInfo.slewPos);

        c.pidInfo.positiveHysteresis = positiveHysteresisValue;
        c.pidInfo.negativeHysteresis = negativeHysteresisValue;
    }
    else
    {
        p.at("samplePeriod").get_to(c.stepwiseInfo.ts);
        p.at("isCeiling").get_to(c.stepwiseInfo.isCeiling);

        for (size_t i = 0; i < ec::maxStepwisePoints; i++)
        {
            c.stepwiseInfo.reading[i] =
                std::numeric_limits<double>::quiet_NaN();
            c.stepwiseInfo.output[i] = std::numeric_limits<double>::quiet_NaN();
        }

        auto reading = p.find("reading");
        if (reading != p.end())
        {
            auto r = p.at("reading");
            for (size_t i = 0; i < ec::maxStepwisePoints; i++)
            {
                auto n = r.find(std::to_string(i));
                if (n != r.end())
                {
                    r.at(std::to_string(i)).get_to(c.stepwiseInfo.reading[i]);
                }
            }
        }

        auto output = p.find("output");
        if (output != p.end())
        {
            auto o = p.at("output");
            for (size_t i = 0; i < ec::maxStepwisePoints; i++)
            {
                auto n = o.find(std::to_string(i));
                if (n != o.end())
                {
                    o.at(std::to_string(i)).get_to(c.stepwiseInfo.output[i]);
                }
            }
        }

        c.stepwiseInfo.positiveHysteresis = positiveHysteresisValue;
        c.stepwiseInfo.negativeHysteresis = negativeHysteresisValue;
    }
}
} // namespace conf

std::pair<std::map<int64_t, conf::PIDConf>,
          std::map<int64_t, struct conf::ZoneConfig>>
    buildPIDsFromJson(const json& data)
{
    // zone -> pids
    std::map<int64_t, conf::PIDConf> pidConfig;
    // zone -> configs
    std::map<int64_t, struct conf::ZoneConfig> zoneConfig;

    /* TODO: if zones is empty, that's invalid. */
    auto zones = data["zones"];
    for (const auto& zone : zones)
    {
        int64_t id;
        conf::PIDConf thisZone;
        struct conf::ZoneConfig thisZoneConfig;

        /* TODO: using at() throws a specific exception we can catch */
        id = zone["id"];
        thisZoneConfig.minThermalOutput = zone["minThermalOutput"];
        thisZoneConfig.failsafePercent = zone["failsafePercent"];

        auto pids = zone["pids"];
        for (const auto& pid : pids)
        {
            auto name = pid["name"];
            auto item = pid.get<conf::ControllerInfo>();

            thisZone[name] = item;
        }

        pidConfig[id] = thisZone;
        zoneConfig[id] = thisZoneConfig;
    }

    return std::make_pair(pidConfig, zoneConfig);
}
