/*
// Copyright (c) 2017 Intel Corporation
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

#include "PwmSensor.hpp"
#include "TachSensor.hpp"
#include "Utils.hpp"
#include "VariantVisitors.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/lexical_cast.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <utility>
#include <variant>
#include <vector>

static constexpr bool DEBUG = false;

namespace fs = std::filesystem;

static constexpr std::array<const char*, 3> sensorTypes = {
    "xyz.openbmc_project.Configuration.AspeedFan",
    "xyz.openbmc_project.Configuration.I2CFan",
    "xyz.openbmc_project.Configuration.NuvotonFan"};
constexpr const char* redundancyConfiguration =
    "xyz.openbmc_project.Configuration.FanRedundancy";
static std::regex inputRegex(R"(fan(\d+)_input)");

enum class FanTypes
{
    aspeed,
    i2c,
    nuvoton
};

// todo: power supply fan redundancy
std::optional<RedundancySensor> systemRedundancy;

FanTypes getFanType(const fs::path& parentPath)
{
    fs::path linkPath = parentPath / "device";
    std::string canonical = fs::read_symlink(linkPath);
    if (boost::ends_with(canonical, "1e786000.pwm-tacho-controller") ||
        boost::ends_with(canonical, "1e610000.pwm-tacho-controller"))
    {
        return FanTypes::aspeed;
    }
    else if (boost::ends_with(canonical, "f0103000.pwm-fan-controller"))
    {
        return FanTypes::nuvoton;
    }
    // todo: will we need to support other types?
    return FanTypes::i2c;
}

void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<TachSensor>>&
        tachSensors,
    boost::container::flat_map<std::string, std::unique_ptr<PwmSensor>>&
        pwmSensors,
    std::shared_ptr<sdbusplus::asio::connection>& dbusConnection,
    const std::shared_ptr<boost::container::flat_set<std::string>>&
        sensorsChanged)
{

    auto getter = std::make_shared<GetSensorConfiguration>(
        dbusConnection,
        std::move([&io, &objectServer, &tachSensors, &pwmSensors,
                   &dbusConnection, sensorsChanged](
                      const ManagedObjectType& sensorConfigurations) {
            bool firstScan = sensorsChanged == nullptr;
            std::vector<fs::path> paths;
            if (!findFiles(fs::path("/sys/class/hwmon"), R"(fan\d+_input)",
                           paths))
            {
                std::cerr << "No temperature sensors in system\n";
                return;
            }

            // pwm index, sysfs path, pwm name
            std::vector<std::tuple<uint8_t, std::string, std::string>>
                pwmNumbers;

            // iterate through all found fan sensors, and try to match them with
            // configuration
            for (const auto& path : paths)
            {
                std::smatch match;
                std::string pathStr = path.string();

                std::regex_search(pathStr, match, inputRegex);
                std::string indexStr = *(match.begin() + 1);

                auto directory = path.parent_path();
                FanTypes fanType = getFanType(directory);
                size_t bus = 0;
                size_t address = 0;
                if (fanType == FanTypes::i2c)
                {
                    std::string link =
                        fs::read_symlink(directory / "device").filename();

                    size_t findDash = link.find("-");
                    if (findDash == std::string::npos ||
                        link.size() <= findDash + 1)
                    {
                        std::cerr << "Error finding device from symlink";
                    }
                    bus = std::stoi(link.substr(0, findDash));
                    address = std::stoi(link.substr(findDash + 1), nullptr, 16);
                }
                // convert to 0 based
                size_t index = std::stoul(indexStr) - 1;

                const char* baseType;
                const SensorData* sensorData = nullptr;
                const std::string* interfacePath = nullptr;
                const SensorBaseConfiguration* baseConfiguration = nullptr;
                for (const std::pair<sdbusplus::message::object_path,
                                     SensorData>& sensor : sensorConfigurations)
                {
                    // find the base of the configuration to see if indexes
                    // match
                    for (const char* type : sensorTypes)
                    {
                        auto sensorBaseFind = sensor.second.find(type);
                        if (sensorBaseFind != sensor.second.end())
                        {
                            baseConfiguration = &(*sensorBaseFind);
                            interfacePath = &(sensor.first.str);
                            baseType = type;
                            break;
                        }
                    }
                    if (baseConfiguration == nullptr)
                    {
                        continue;
                    }

                    auto findIndex = baseConfiguration->second.find("Index");
                    if (findIndex == baseConfiguration->second.end())
                    {
                        std::cerr << baseConfiguration->first
                                  << " missing index\n";
                        continue;
                    }
                    unsigned int configIndex = std::visit(
                        VariantToUnsignedIntVisitor(), findIndex->second);
                    if (configIndex != index)
                    {
                        continue;
                    }
                    if (fanType == FanTypes::aspeed ||
                        fanType == FanTypes::nuvoton)
                    {
                        // there will be only 1 aspeed or nuvoton sensor object
                        // in sysfs, we found the fan
                        sensorData = &(sensor.second);
                        break;
                    }
                    else if (baseType ==
                             std::string(
                                 "xyz.openbmc_project.Configuration.I2CFan"))
                    {
                        auto findBus = baseConfiguration->second.find("Bus");
                        auto findAddress =
                            baseConfiguration->second.find("Address");
                        if (findBus == baseConfiguration->second.end() ||
                            findAddress == baseConfiguration->second.end())
                        {
                            std::cerr << baseConfiguration->first
                                      << " missing bus or address\n";
                            continue;
                        }
                        unsigned int configBus = std::visit(
                            VariantToUnsignedIntVisitor(), findBus->second);
                        unsigned int configAddress = std::visit(
                            VariantToUnsignedIntVisitor(), findAddress->second);

                        if (configBus == bus && configAddress == address)
                        {
                            sensorData = &(sensor.second);
                            break;
                        }
                    }
                }
                if (sensorData == nullptr)
                {
                    std::cerr << "failed to find match for " << path.string()
                              << "\n";
                    continue;
                }

                auto findSensorName = baseConfiguration->second.find("Name");

                if (findSensorName == baseConfiguration->second.end())
                {
                    std::cerr << "could not determine configuration name for "
                              << path.string() << "\n";
                    continue;
                }
                std::string sensorName =
                    std::get<std::string>(findSensorName->second);

                // on rescans, only update sensors we were signaled by
                auto findSensor = tachSensors.find(sensorName);
                if (!firstScan && findSensor != tachSensors.end())
                {
                    bool found = false;
                    for (auto it = sensorsChanged->begin();
                         it != sensorsChanged->end(); it++)
                    {
                        if (boost::ends_with(*it, findSensor->second->name))
                        {
                            sensorsChanged->erase(it);
                            findSensor->second = nullptr;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        continue;
                    }
                }
                std::vector<thresholds::Threshold> sensorThresholds;
                if (!parseThresholdsFromConfig(*sensorData, sensorThresholds))
                {
                    std::cerr << "error populating thresholds for "
                              << sensorName << "\n";
                }

                auto presenceConfig =
                    sensorData->find(baseType + std::string(".Presence"));

                std::unique_ptr<PresenceSensor> presenceSensor(nullptr);

                // presence sensors are optional
                if (presenceConfig != sensorData->end())
                {
                    auto findPolarity = presenceConfig->second.find("Polarity");
                    auto findPinName = presenceConfig->second.find("PinName");

                    if (findPinName == presenceConfig->second.end() ||
                        findPolarity == presenceConfig->second.end())
                    {
                        std::cerr << "Malformed Presence Configuration\n";
                    }
                    else
                    {
                        bool inverted = std::get<std::string>(
                                            findPolarity->second) == "Low";
                        if (auto pinName =
                                std::get_if<std::string>(&findPinName->second))
                        {
                            presenceSensor = std::make_unique<PresenceSensor>(
                                *pinName, inverted, io, sensorName);
                        }
                        else
                        {
                            std::cerr
                                << "Malformed Presence pinName for sensor "
                                << sensorName << " \n";
                        }
                    }
                }
                std::optional<RedundancySensor>* redundancy = nullptr;
                if (fanType == FanTypes::aspeed)
                {
                    redundancy = &systemRedundancy;
                }

                constexpr double defaultMaxReading = 25000;
                constexpr double defaultMinReading = 0;
                auto limits =
                    std::make_pair(defaultMinReading, defaultMaxReading);

                findLimits(limits, baseConfiguration);
                tachSensors[sensorName] = std::make_unique<TachSensor>(
                    path.string(), baseType, objectServer, dbusConnection,
                    std::move(presenceSensor), redundancy, io, sensorName,
                    std::move(sensorThresholds), *interfacePath, limits);

                auto connector =
                    sensorData->find(baseType + std::string(".Connector"));
                if (connector != sensorData->end())
                {
                    auto findPwm = connector->second.find("Pwm");
                    if (findPwm == connector->second.end())
                    {
                        std::cerr << "Connector Missing PWM!\n";
                        continue;
                    }
                    size_t pwm = std::visit(VariantToUnsignedIntVisitor(),
                                            findPwm->second);
                    /* use pwm name override if found in configuration else use
                     * default */
                    auto findOverride = connector->second.find("PwmName");
                    std::string pwmName;
                    if (findOverride != connector->second.end())
                    {
                        pwmName = std::visit(VariantToStringVisitor(),
                                             findOverride->second);
                    }
                    else
                    {
                        pwmName = "Pwm_" + std::to_string(pwm + 1);
                    }
                    pwmNumbers.emplace_back(pwm, *interfacePath, pwmName);
                }
            }
            std::vector<fs::path> pwms;
            if (!findFiles(fs::path("/sys/class/hwmon"), R"(pwm\d+$)", pwms))
            {
                std::cerr << "No pwm in system\n";
                return;
            }
            for (const fs::path& pwm : pwms)
            {
                if (pwmSensors.find(pwm) != pwmSensors.end())
                {
                    continue;
                }
                const std::string* path = nullptr;
                const std::string* pwmName = nullptr;

                for (const auto& [index, configPath, name] : pwmNumbers)
                {
                    if (boost::ends_with(pwm.string(),
                                         std::to_string(index + 1)))
                    {
                        path = &configPath;
                        pwmName = &name;
                        break;
                    }
                }

                if (path == nullptr)
                {
                    continue;
                }

                // only add new elements
                const std::string& sysPath = pwm.string();
                pwmSensors.insert(
                    std::pair<std::string, std::unique_ptr<PwmSensor>>(
                        sysPath, std::make_unique<PwmSensor>(
                                     *pwmName, sysPath, dbusConnection,
                                     objectServer, *path, "Fan")));
            }
        }));
    getter->getConfiguration(
        std::vector<std::string>{sensorTypes.begin(), sensorTypes.end()});
}

void createRedundancySensor(
    const boost::container::flat_map<std::string, std::unique_ptr<TachSensor>>&
        sensors,
    std::shared_ptr<sdbusplus::asio::connection> conn,
    sdbusplus::asio::object_server& objectServer)
{

    conn->async_method_call(
        [&objectServer, &sensors](boost::system::error_code& ec,
                                  const ManagedObjectType managedObj) {
            if (ec)
            {
                std::cerr << "Error calling entity manager \n";
                return;
            }
            for (const auto& pathPair : managedObj)
            {
                for (const auto& interfacePair : pathPair.second)
                {
                    if (interfacePair.first == redundancyConfiguration)
                    {
                        // currently only support one
                        auto findCount =
                            interfacePair.second.find("AllowedFailures");
                        if (findCount == interfacePair.second.end())
                        {
                            std::cerr << "Malformed redundancy record \n";
                            return;
                        }
                        std::vector<std::string> sensorList;

                        for (const auto& sensor : sensors)
                        {
                            sensorList.push_back(
                                "/xyz/openbmc_project/sensors/fan_tach/" +
                                sensor.second->name);
                        }
                        systemRedundancy.reset();
                        systemRedundancy.emplace(RedundancySensor(
                            std::get<uint64_t>(findCount->second), sensorList,
                            objectServer, pathPair.first));

                        return;
                    }
                }
            }
        },
        "xyz.openbmc_project.EntityManager", "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

int main()
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name("xyz.openbmc_project.FanSensor");
    sdbusplus::asio::object_server objectServer(systemBus);
    boost::container::flat_map<std::string, std::unique_ptr<TachSensor>>
        tachSensors;
    boost::container::flat_map<std::string, std::unique_ptr<PwmSensor>>
        pwmSensors;
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;
    auto sensorsChanged =
        std::make_shared<boost::container::flat_set<std::string>>();

    io.post([&]() {
        createSensors(io, objectServer, tachSensors, pwmSensors, systemBus,
                      nullptr);
        createRedundancySensor(tachSensors, systemBus, objectServer);
    });

    boost::asio::deadline_timer filterTimer(io);
    std::function<void(sdbusplus::message::message&)> eventHandler =
        [&](sdbusplus::message::message& message) {
            if (message.is_method_error())
            {
                std::cerr << "callback method error\n";
                return;
            }
            sensorsChanged->insert(message.get_path());
            // this implicitly cancels the timer
            filterTimer.expires_from_now(boost::posix_time::seconds(1));

            filterTimer.async_wait([&](const boost::system::error_code& ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    /* we were canceled*/
                    return;
                }
                else if (ec)
                {
                    std::cerr << "timer error\n";
                    return;
                }
                createSensors(io, objectServer, tachSensors, pwmSensors,
                              systemBus, sensorsChanged);
            });
        };

    for (const char* type : sensorTypes)
    {
        auto match = std::make_unique<sdbusplus::bus::match::match>(
            static_cast<sdbusplus::bus::bus&>(*systemBus),
            "type='signal',member='PropertiesChanged',path_namespace='" +
                std::string(inventoryPath) + "',arg0namespace='" + type + "'",
            eventHandler);
        matches.emplace_back(std::move(match));
    }

    // redundancy sensor
    std::function<void(sdbusplus::message::message&)> redundancyHandler =
        [&tachSensors, &systemBus,
         &objectServer](sdbusplus::message::message&) {
            createRedundancySensor(tachSensors, systemBus, objectServer);
        };
    auto match = std::make_unique<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus&>(*systemBus),
        "type='signal',member='PropertiesChanged',path_namespace='" +
            std::string(inventoryPath) + "',arg0namespace='" +
            redundancyConfiguration + "'",
        std::move(redundancyHandler));
    matches.emplace_back(std::move(match));

    io.run();
}
