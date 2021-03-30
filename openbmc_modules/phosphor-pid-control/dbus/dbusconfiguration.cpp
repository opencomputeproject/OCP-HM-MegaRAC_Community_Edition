/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "conf.hpp"
#include "util.hpp"

#include <algorithm>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <regex>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>
#include <set>
#include <unordered_map>
#include <variant>

static constexpr bool DEBUG = false; // enable to print found configuration

extern std::map<std::string, struct conf::SensorConfig> sensorConfig;
extern std::map<int64_t, conf::PIDConf> zoneConfig;
extern std::map<int64_t, struct conf::ZoneConfig> zoneDetailsConfig;

constexpr const char* pidConfigurationInterface =
    "xyz.openbmc_project.Configuration.Pid";
constexpr const char* objectManagerInterface =
    "org.freedesktop.DBus.ObjectManager";
constexpr const char* pidZoneConfigurationInterface =
    "xyz.openbmc_project.Configuration.Pid.Zone";
constexpr const char* stepwiseConfigurationInterface =
    "xyz.openbmc_project.Configuration.Stepwise";
constexpr const char* thermalControlIface =
    "xyz.openbmc_project.Control.ThermalMode";
constexpr const char* sensorInterface = "xyz.openbmc_project.Sensor.Value";
constexpr const char* pwmInterface = "xyz.openbmc_project.Control.FanPwm";

using Association = std::tuple<std::string, std::string, std::string>;
using Associations = std::vector<Association>;

namespace thresholds
{
constexpr const char* warningInterface =
    "xyz.openbmc_project.Sensor.Threshold.Warning";
constexpr const char* criticalInterface =
    "xyz.openbmc_project.Sensor.Threshold.Critical";
const std::array<const char*, 4> types = {"CriticalLow", "CriticalHigh",
                                          "WarningLow", "WarningHigh"};

} // namespace thresholds

namespace dbus_configuration
{

using DbusVariantType =
    std::variant<uint64_t, int64_t, double, std::string,
                 std::vector<std::string>, std::vector<double>>;

bool findSensors(const std::unordered_map<std::string, std::string>& sensors,
                 const std::string& search,
                 std::vector<std::pair<std::string, std::string>>& matches)
{
    std::smatch match;
    std::regex reg(search + '$');
    for (const auto& sensor : sensors)
    {
        if (std::regex_search(sensor.first, match, reg))
        {
            matches.push_back(sensor);
        }
    }
    return matches.size() > 0;
}

// this function prints the configuration into a form similar to the cpp
// generated code to help in verification, should be turned off during normal
// use
void debugPrint(void)
{
    // print sensor config
    std::cout << "sensor config:\n";
    std::cout << "{\n";
    for (const auto& pair : sensorConfig)
    {

        std::cout << "\t{" << pair.first << ",\n\t\t{";
        std::cout << pair.second.type << ", ";
        std::cout << pair.second.readPath << ", ";
        std::cout << pair.second.writePath << ", ";
        std::cout << pair.second.min << ", ";
        std::cout << pair.second.max << ", ";
        std::cout << pair.second.timeout << "},\n\t},\n";
    }
    std::cout << "}\n\n";
    std::cout << "ZoneDetailsConfig\n";
    std::cout << "{\n";
    for (const auto& zone : zoneDetailsConfig)
    {
        std::cout << "\t{" << zone.first << ",\n";
        std::cout << "\t\t{" << zone.second.minThermalOutput << ", ";
        std::cout << zone.second.failsafePercent << "}\n\t},\n";
    }
    std::cout << "}\n\n";
    std::cout << "ZoneConfig\n";
    std::cout << "{\n";
    for (const auto& zone : zoneConfig)
    {
        std::cout << "\t{" << zone.first << "\n";
        for (const auto& pidconf : zone.second)
        {
            std::cout << "\t\t{" << pidconf.first << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.type << ",\n";
            std::cout << "\t\t\t{";
            for (const auto& input : pidconf.second.inputs)
            {
                std::cout << "\n\t\t\t" << input << ",\n";
            }
            std::cout << "\t\t\t}\n";
            std::cout << "\t\t\t" << pidconf.second.setpoint << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.pidInfo.ts << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.proportionalCoeff
                      << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.integralCoeff
                      << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.feedFwdOffset
                      << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.feedFwdGain
                      << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.pidInfo.integralLimit.min
                      << "," << pidconf.second.pidInfo.integralLimit.max
                      << "},\n";
            std::cout << "\t\t\t{" << pidconf.second.pidInfo.outLim.min << ","
                      << pidconf.second.pidInfo.outLim.max << "},\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.slewNeg << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.slewPos << ",\n";
            std::cout << "\t\t\t}\n\t\t}\n";
        }
        std::cout << "\t},\n";
    }
    std::cout << "}\n\n";
}

size_t getZoneIndex(const std::string& name, std::vector<std::string>& zones)
{
    auto it = std::find(zones.begin(), zones.end(), name);
    if (it == zones.end())
    {
        zones.emplace_back(name);
        it = zones.end() - 1;
    }

    return it - zones.begin();
}

std::vector<std::string> getSelectedProfiles(sdbusplus::bus::bus& bus)
{
    std::vector<std::string> ret;
    auto mapper =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    mapper.append("/", 0, std::array<const char*, 1>{thermalControlIface});
    std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>
        respData;

    try
    {
        auto resp = bus.call(mapper);
        resp.read(respData);
    }
    catch (sdbusplus::exception_t&)
    {
        // can't do anything without mapper call data
        throw std::runtime_error("ObjectMapper Call Failure");
    }
    if (respData.empty())
    {
        // if the user has profiles but doesn't expose the interface to select
        // one, just go ahead without using profiles
        return ret;
    }

    // assumption is that we should only have a small handful of selected
    // profiles at a time (probably only 1), so calling each individually should
    // not incur a large cost
    for (const auto& objectPair : respData)
    {
        const std::string& path = objectPair.first;
        for (const auto& ownerPair : objectPair.second)
        {
            const std::string& busName = ownerPair.first;
            auto getProfile =
                bus.new_method_call(busName.c_str(), path.c_str(),
                                    "org.freedesktop.DBus.Properties", "Get");
            getProfile.append(thermalControlIface, "Current");
            std::variant<std::string> variantResp;
            try
            {
                auto resp = bus.call(getProfile);
                resp.read(variantResp);
            }
            catch (sdbusplus::exception_t&)
            {
                throw std::runtime_error("Failure getting profile");
            }
            std::string mode = std::get<std::string>(variantResp);
            ret.emplace_back(std::move(mode));
        }
    }
    if constexpr (DEBUG)
    {
        std::cout << "Profiles selected: ";
        for (const auto& profile : ret)
        {
            std::cout << profile << " ";
        }
        std::cout << "\n";
    }
    return ret;
}

int eventHandler(sd_bus_message* m, void* context, sd_bus_error*)
{

    if (context == nullptr || m == nullptr)
    {
        throw std::runtime_error("Invalid match");
    }

    // we skip associations because the mapper populates these, not the sensors
    const std::array<const char*, 1> skipList = {
        "xyz.openbmc_project.Association"};

    sdbusplus::message::message message(m);
    if (std::string(message.get_member()) == "InterfacesAdded")
    {
        sdbusplus::message::object_path path;
        std::unordered_map<
            std::string,
            std::unordered_map<std::string, std::variant<Associations, bool>>>
            data;

        message.read(path, data);

        for (const char* skip : skipList)
        {
            auto find = data.find(skip);
            if (find != data.end())
            {
                data.erase(find);
                if (data.empty())
                {
                    return 1;
                }
            }
        }
    }

    boost::asio::steady_timer* timer =
        static_cast<boost::asio::steady_timer*>(context);

    // do a brief sleep as we tend to get a bunch of these events at
    // once
    timer->expires_after(std::chrono::seconds(2));
    timer->async_wait([](const boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            /* another timer started*/
            return;
        }

        std::cout << "New configuration detected, reloading\n.";
        tryRestartControlLoops();
    });

    return 1;
}

void createMatches(sdbusplus::bus::bus& bus, boost::asio::steady_timer& timer)
{
    // this is a list because the matches can't be moved
    static std::list<sdbusplus::bus::match::match> matches;

    const std::array<std::string, 4> interfaces = {
        thermalControlIface, pidConfigurationInterface,
        pidZoneConfigurationInterface, stepwiseConfigurationInterface};

    // this list only needs to be created once
    if (!matches.empty())
    {
        return;
    }

    // we restart when the configuration changes or there are new sensors
    for (const auto& interface : interfaces)
    {
        matches.emplace_back(
            bus,
            "type='signal',member='PropertiesChanged',arg0namespace='" +
                interface + "'",
            eventHandler, &timer);
    }
    matches.emplace_back(
        bus,
        "type='signal',member='InterfacesAdded',arg0path='/xyz/openbmc_project/"
        "sensors/'",
        eventHandler, &timer);
}

/**
 * retrieve an attribute from the pid configuration map
 * @param[in] base - the PID configuration map, keys are the attributes and
 * value is the variant associated with that attribute.
 * @param attributeName - the name of the attribute
 * @return a variant holding the value associated with a key
 * @throw runtime_error : attributeName is not in base
 */
inline DbusVariantType getPIDAttribute(
    const std::unordered_map<std::string, DbusVariantType>& base,
    const std::string& attributeName)
{
    auto search = base.find(attributeName);
    if (search == base.end())
    {
        throw std::runtime_error("missing attribute " + attributeName);
    }
    return search->second;
}

void populatePidInfo(
    sdbusplus::bus::bus& bus,
    const std::unordered_map<std::string, DbusVariantType>& base,
    struct conf::ControllerInfo& info, const std::string* thresholdProperty)
{
    info.type = std::get<std::string>(getPIDAttribute(base, "Class"));
    if (info.type == "fan")
    {
        info.setpoint = 0;
    }
    else
    {
        info.setpoint = std::visit(VariantToDoubleVisitor(),
                                   getPIDAttribute(base, "SetPoint"));
    }

    if (thresholdProperty != nullptr)
    {
        std::string interface;
        if (*thresholdProperty == "WarningHigh" ||
            *thresholdProperty == "WarningLow")
        {
            interface = thresholds::warningInterface;
        }
        else
        {
            interface = thresholds::criticalInterface;
        }
        const std::string& path = sensorConfig[info.inputs.front()].readPath;

        DbusHelper helper;
        std::string service = helper.getService(bus, interface, path);
        double reading = 0;
        try
        {
            helper.getProperty(bus, service, path, interface,
                               *thresholdProperty, reading);
        }
        catch (const sdbusplus::exception::SdBusError& ex)
        {
            // unsupported threshold, leaving reading at 0
        }

        info.setpoint += reading;
    }

    info.pidInfo.ts = 1.0; // currently unused
    info.pidInfo.proportionalCoeff = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "PCoefficient"));
    info.pidInfo.integralCoeff = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "ICoefficient"));
    info.pidInfo.feedFwdOffset = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "FFOffCoefficient"));
    info.pidInfo.feedFwdGain = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "FFGainCoefficient"));
    info.pidInfo.integralLimit.max = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "ILimitMax"));
    info.pidInfo.integralLimit.min = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "ILimitMin"));
    info.pidInfo.outLim.max = std::visit(VariantToDoubleVisitor(),
                                         getPIDAttribute(base, "OutLimitMax"));
    info.pidInfo.outLim.min = std::visit(VariantToDoubleVisitor(),
                                         getPIDAttribute(base, "OutLimitMin"));
    info.pidInfo.slewNeg =
        std::visit(VariantToDoubleVisitor(), getPIDAttribute(base, "SlewNeg"));
    info.pidInfo.slewPos =
        std::visit(VariantToDoubleVisitor(), getPIDAttribute(base, "SlewPos"));
    double negativeHysteresis = 0;
    double positiveHysteresis = 0;

    auto findNeg = base.find("NegativeHysteresis");
    auto findPos = base.find("PositiveHysteresis");

    if (findNeg != base.end())
    {
        negativeHysteresis =
            std::visit(VariantToDoubleVisitor(), findNeg->second);
    }

    if (findPos != base.end())
    {
        positiveHysteresis =
            std::visit(VariantToDoubleVisitor(), findPos->second);
    }
    info.pidInfo.negativeHysteresis = negativeHysteresis;
    info.pidInfo.positiveHysteresis = positiveHysteresis;
}

bool init(sdbusplus::bus::bus& bus, boost::asio::steady_timer& timer)
{

    sensorConfig.clear();
    zoneConfig.clear();
    zoneDetailsConfig.clear();

    createMatches(bus, timer);

    using DbusVariantType =
        std::variant<uint64_t, int64_t, double, std::string,
                     std::vector<std::string>, std::vector<double>>;

    using ManagedObjectType = std::unordered_map<
        sdbusplus::message::object_path,
        std::unordered_map<std::string,
                           std::unordered_map<std::string, DbusVariantType>>>;

    auto mapper =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    mapper.append("/", 0,
                  std::array<const char*, 6>{objectManagerInterface,
                                             pidConfigurationInterface,
                                             pidZoneConfigurationInterface,
                                             stepwiseConfigurationInterface,
                                             sensorInterface, pwmInterface});
    std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>
        respData;
    try
    {
        auto resp = bus.call(mapper);
        resp.read(respData);
    }
    catch (sdbusplus::exception_t&)
    {
        // can't do anything without mapper call data
        throw std::runtime_error("ObjectMapper Call Failure");
    }

    if (respData.empty())
    {
        // can't do anything without mapper call data
        throw std::runtime_error("No configuration data available from Mapper");
    }
    // create a map of pair of <has pid configuration, ObjectManager path>
    std::unordered_map<std::string, std::pair<bool, std::string>> owners;
    // and a map of <path, interface> for sensors
    std::unordered_map<std::string, std::string> sensors;
    for (const auto& objectPair : respData)
    {
        for (const auto& ownerPair : objectPair.second)
        {
            auto& owner = owners[ownerPair.first];
            for (const std::string& interface : ownerPair.second)
            {

                if (interface == objectManagerInterface)
                {
                    owner.second = objectPair.first;
                }
                if (interface == pidConfigurationInterface ||
                    interface == pidZoneConfigurationInterface ||
                    interface == stepwiseConfigurationInterface)
                {
                    owner.first = true;
                }
                if (interface == sensorInterface || interface == pwmInterface)
                {
                    // we're not interested in pwm sensors, just pwm control
                    if (interface == sensorInterface &&
                        objectPair.first.find("pwm") != std::string::npos)
                    {
                        continue;
                    }
                    sensors[objectPair.first] = interface;
                }
            }
        }
    }
    ManagedObjectType configurations;
    for (const auto& owner : owners)
    {
        // skip if no pid configuration (means probably a sensor)
        if (!owner.second.first)
        {
            continue;
        }
        auto endpoint = bus.new_method_call(
            owner.first.c_str(), owner.second.second.c_str(),
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        ManagedObjectType configuration;
        try
        {
            auto responce = bus.call(endpoint);
            responce.read(configuration);
        }
        catch (sdbusplus::exception_t&)
        {
            // this shouldn't happen, probably means daemon crashed
            throw std::runtime_error("Error getting managed objects from " +
                                     owner.first);
        }

        for (auto& pathPair : configuration)
        {
            if (pathPair.second.find(pidConfigurationInterface) !=
                    pathPair.second.end() ||
                pathPair.second.find(pidZoneConfigurationInterface) !=
                    pathPair.second.end() ||
                pathPair.second.find(stepwiseConfigurationInterface) !=
                    pathPair.second.end())
            {
                configurations.emplace(pathPair);
            }
        }
    }

    // remove controllers from config that aren't in the current profile(s)
    std::vector<std::string> selectedProfiles = getSelectedProfiles(bus);
    if (selectedProfiles.size())
    {
        for (auto pathIt = configurations.begin();
             pathIt != configurations.end();)
        {
            for (auto confIt = pathIt->second.begin();
                 confIt != pathIt->second.end();)
            {
                auto profilesFind = confIt->second.find("Profiles");
                if (profilesFind == confIt->second.end())
                {
                    confIt++;
                    continue; // if no profiles selected, apply always
                }
                auto profiles =
                    std::get<std::vector<std::string>>(profilesFind->second);
                if (profiles.empty())
                {
                    confIt++;
                    continue;
                }

                bool found = false;
                for (const std::string& profile : profiles)
                {
                    if (std::find(selectedProfiles.begin(),
                                  selectedProfiles.end(),
                                  profile) != selectedProfiles.end())
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    confIt++;
                }
                else
                {
                    confIt = pathIt->second.erase(confIt);
                }
            }
            if (pathIt->second.empty())
            {
                pathIt = configurations.erase(pathIt);
            }
            else
            {
                pathIt++;
            }
        }
    }

    // on dbus having an index field is a bit strange, so randomly
    // assign index based on name property
    std::vector<std::string> foundZones;
    for (const auto& configuration : configurations)
    {
        auto findZone =
            configuration.second.find(pidZoneConfigurationInterface);
        if (findZone != configuration.second.end())
        {
            const auto& zone = findZone->second;

            const std::string& name = std::get<std::string>(zone.at("Name"));
            size_t index = getZoneIndex(name, foundZones);

            auto& details = zoneDetailsConfig[index];
            details.minThermalOutput = std::visit(VariantToDoubleVisitor(),
                                                  zone.at("MinThermalOutput"));
            details.failsafePercent = std::visit(VariantToDoubleVisitor(),
                                                 zone.at("FailSafePercent"));
        }
        auto findBase = configuration.second.find(pidConfigurationInterface);
        if (findBase != configuration.second.end())
        {

            const auto& base =
                configuration.second.at(pidConfigurationInterface);
            const std::vector<std::string>& zones =
                std::get<std::vector<std::string>>(base.at("Zones"));
            for (const std::string& zone : zones)
            {
                size_t index = getZoneIndex(zone, foundZones);
                conf::PIDConf& conf = zoneConfig[index];

                std::vector<std::string> sensorNames =
                    std::get<std::vector<std::string>>(base.at("Inputs"));
                auto findOutputs =
                    base.find("Outputs"); // currently only fans have outputs
                if (findOutputs != base.end())
                {
                    std::vector<std::string> outputs =
                        std::get<std::vector<std::string>>(findOutputs->second);
                    sensorNames.insert(sensorNames.end(), outputs.begin(),
                                       outputs.end());
                }

                std::vector<std::string> inputs;
                std::vector<std::pair<std::string, std::string>>
                    sensorInterfaces;
                for (const std::string& sensorName : sensorNames)
                {
                    std::string name = sensorName;
                    // replace spaces with underscores to be legal on dbus
                    std::replace(name.begin(), name.end(), ' ', '_');
                    findSensors(sensors, name, sensorInterfaces);
                }

                for (const auto& sensorPathIfacePair : sensorInterfaces)
                {

                    if (sensorPathIfacePair.second == sensorInterface)
                    {
                        size_t idx =
                            sensorPathIfacePair.first.find_last_of("/") + 1;
                        std::string shortName =
                            sensorPathIfacePair.first.substr(idx);

                        inputs.push_back(shortName);
                        auto& config = sensorConfig[shortName];
                        config.type = std::get<std::string>(base.at("Class"));
                        config.readPath = sensorPathIfacePair.first;
                        // todo: maybe un-hardcode this if we run into slower
                        // timeouts with sensors
                        if (config.type == "temp")
                        {
                            config.timeout = 0;
                            config.ignoreDbusMinMax = true;
                        }
                    }
                    else if (sensorPathIfacePair.second == pwmInterface)
                    {
                        // copy so we can modify it
                        for (std::string otherSensor : sensorNames)
                        {
                            std::replace(otherSensor.begin(), otherSensor.end(),
                                         ' ', '_');
                            if (sensorPathIfacePair.first.find(otherSensor) !=
                                std::string::npos)
                            {
                                continue;
                            }

                            auto& config = sensorConfig[otherSensor];
                            config.writePath = sensorPathIfacePair.first;
                            // todo: un-hardcode this if there are fans with
                            // different ranges
                            config.max = 255;
                            config.min = 0;
                        }
                    }
                }

                // if the sensors aren't available in the current state, don't
                // add them to the configuration.
                if (inputs.empty())
                {
                    continue;
                }

                std::string offsetType;

                // SetPointOffset is a threshold value to pull from the sensor
                // to apply an offset. For upper thresholds this means the
                // setpoint is usually negative.
                auto findSetpointOffset = base.find("SetPointOffset");
                if (findSetpointOffset != base.end())
                {
                    offsetType =
                        std::get<std::string>(findSetpointOffset->second);
                    if (std::find(thresholds::types.begin(),
                                  thresholds::types.end(),
                                  offsetType) == thresholds::types.end())
                    {
                        throw std::runtime_error("Unsupported type: " +
                                                 offsetType);
                    }
                }

                if (offsetType.empty())
                {
                    struct conf::ControllerInfo& info =
                        conf[std::get<std::string>(base.at("Name"))];
                    info.inputs = std::move(inputs);
                    populatePidInfo(bus, base, info, nullptr);
                }
                else
                {
                    // we have to split up the inputs, as in practice t-control
                    // values will differ, making setpoints differ
                    for (const std::string& input : inputs)
                    {
                        struct conf::ControllerInfo& info = conf[input];
                        info.inputs.emplace_back(input);
                        populatePidInfo(bus, base, info, &offsetType);
                    }
                }
            }
        }
        auto findStepwise =
            configuration.second.find(stepwiseConfigurationInterface);
        if (findStepwise != configuration.second.end())
        {
            const auto& base = findStepwise->second;
            const std::vector<std::string>& zones =
                std::get<std::vector<std::string>>(base.at("Zones"));
            for (const std::string& zone : zones)
            {
                size_t index = getZoneIndex(zone, foundZones);
                conf::PIDConf& conf = zoneConfig[index];

                std::vector<std::string> inputs;
                std::vector<std::string> sensorNames =
                    std::get<std::vector<std::string>>(base.at("Inputs"));

                bool sensorFound = false;
                for (const std::string& sensorName : sensorNames)
                {
                    std::string name = sensorName;
                    // replace spaces with underscores to be legal on dbus
                    std::replace(name.begin(), name.end(), ' ', '_');
                    std::vector<std::pair<std::string, std::string>>
                        sensorPathIfacePairs;

                    if (!findSensors(sensors, name, sensorPathIfacePairs))
                    {
                        break;
                    }

                    for (const auto& sensorPathIfacePair : sensorPathIfacePairs)
                    {
                        size_t idx =
                            sensorPathIfacePair.first.find_last_of("/") + 1;
                        std::string shortName =
                            sensorPathIfacePair.first.substr(idx);

                        inputs.push_back(shortName);
                        auto& config = sensorConfig[shortName];
                        config.readPath = sensorPathIfacePair.first;
                        config.type = "temp";
                        config.ignoreDbusMinMax = true;
                        // todo: maybe un-hardcode this if we run into slower
                        // timeouts with sensors

                        config.timeout = 0;
                        sensorFound = true;
                    }
                }
                if (!sensorFound)
                {
                    continue;
                }
                struct conf::ControllerInfo& info =
                    conf[std::get<std::string>(base.at("Name"))];
                info.inputs = std::move(inputs);

                info.type = "stepwise";
                info.stepwiseInfo.ts = 1.0; // currently unused
                info.stepwiseInfo.positiveHysteresis = 0.0;
                info.stepwiseInfo.negativeHysteresis = 0.0;
                std::string subtype = std::get<std::string>(base.at("Class"));

                info.stepwiseInfo.isCeiling = (subtype == "Ceiling");
                auto findPosHyst = base.find("PositiveHysteresis");
                auto findNegHyst = base.find("NegativeHysteresis");
                if (findPosHyst != base.end())
                {
                    info.stepwiseInfo.positiveHysteresis = std::visit(
                        VariantToDoubleVisitor(), findPosHyst->second);
                }
                if (findNegHyst != base.end())
                {
                    info.stepwiseInfo.negativeHysteresis = std::visit(
                        VariantToDoubleVisitor(), findNegHyst->second);
                }
                std::vector<double> readings =
                    std::get<std::vector<double>>(base.at("Reading"));
                if (readings.size() > ec::maxStepwisePoints)
                {
                    throw std::invalid_argument("Too many stepwise points.");
                }
                if (readings.empty())
                {
                    throw std::invalid_argument(
                        "Must have one stepwise point.");
                }
                std::copy(readings.begin(), readings.end(),
                          info.stepwiseInfo.reading);
                if (readings.size() < ec::maxStepwisePoints)
                {
                    info.stepwiseInfo.reading[readings.size()] =
                        std::numeric_limits<double>::quiet_NaN();
                }
                std::vector<double> outputs =
                    std::get<std::vector<double>>(base.at("Output"));
                if (readings.size() != outputs.size())
                {
                    throw std::invalid_argument(
                        "Outputs size must match readings");
                }
                std::copy(outputs.begin(), outputs.end(),
                          info.stepwiseInfo.output);
                if (outputs.size() < ec::maxStepwisePoints)
                {
                    info.stepwiseInfo.output[outputs.size()] =
                        std::numeric_limits<double>::quiet_NaN();
                }
            }
        }
    }
    if constexpr (DEBUG)
    {
        debugPrint();
    }
    if (zoneConfig.empty() || zoneDetailsConfig.empty())
    {
        std::cerr
            << "No fan zones, application pausing until new configuration\n";
        return false;
    }
    return true;
}
} // namespace dbus_configuration
