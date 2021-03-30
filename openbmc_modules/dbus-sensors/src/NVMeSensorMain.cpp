/*
// Copyright (c) 2019 Intel Corporation
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

#include "NVMeSensor.hpp"

#include <boost/asio/deadline_timer.hpp>

#include <regex>

static constexpr const char* sensorType =
    "xyz.openbmc_project.Configuration.NVME1000";

static NVMEMap nvmeDeviceMap;

static constexpr bool DEBUG = false;

NVMEMap& getNVMEMap()
{
    return nvmeDeviceMap;
}

void createSensors(boost::asio::io_service& io,
                   sdbusplus::asio::object_server& objectServer,
                   std::shared_ptr<sdbusplus::asio::connection>& dbusConnection)
{

    auto getter = std::make_shared<GetSensorConfiguration>(
        dbusConnection,
        std::move([&io, &objectServer, &dbusConnection](
                      const ManagedObjectType& sensorConfigurations) {
            // todo: it'd be better to only update the ones we care about
            for (const auto& [_, nvmeContextPtr] : nvmeDeviceMap)
            {
                if (nvmeContextPtr)
                {
                    nvmeContextPtr->close();
                }
            }
            nvmeDeviceMap.clear();

            // iterate through all found configurations
            for (const std::pair<sdbusplus::message::object_path, SensorData>&
                     sensor : sensorConfigurations)
            {
                const SensorData& sensorData = sensor.second;
                const std::string& interfacePath = sensor.first.str;
                const std::pair<
                    std::string,
                    boost::container::flat_map<std::string, BasicVariantType>>*
                    baseConfiguration = nullptr;

                // find base configuration
                auto sensorBase = sensor.second.find(sensorType);
                if (sensorBase != sensor.second.end())
                {
                    baseConfiguration = &(*sensorBase);
                }

                if (baseConfiguration == nullptr)
                {
                    continue;
                }
                auto findBus = baseConfiguration->second.find("Bus");
                if (findBus == baseConfiguration->second.end())
                {
                    continue;
                }

                unsigned int busNumber =
                    std::visit(VariantToUnsignedIntVisitor(), findBus->second);

                auto findSensorName = baseConfiguration->second.find("Name");
                if (findSensorName == baseConfiguration->second.end())
                {
                    std::cerr << "could not determine configuration name for "
                              << interfacePath << "\n";
                    continue;
                }
                std::string sensorName =
                    std::get<std::string>(findSensorName->second);

                std::vector<thresholds::Threshold> sensorThresholds;

                if (!parseThresholdsFromConfig(sensorData, sensorThresholds))
                {
                    std::cerr << "error populating thresholds for "
                              << sensorName << "\n";
                }

                int rootBus = busNumber;

                std::string muxPath = "/sys/bus/i2c/devices/i2c-" +
                                      std::to_string(busNumber) + "/mux_device";

                if (std::filesystem::is_symlink(muxPath))
                {
                    std::string rootName =
                        std::filesystem::read_symlink(muxPath).filename();
                    size_t dash = rootName.find("-");
                    if (dash == std::string::npos)
                    {
                        std::cerr << "Error finding root bus for " << rootName
                                  << "\n";
                        continue;
                    }
                    rootBus = std::stoi(rootName.substr(0, dash));
                }

                std::shared_ptr<NVMeContext> context;
                auto findRoot = nvmeDeviceMap.find(rootBus);
                if (findRoot != nvmeDeviceMap.end())
                {
                    context = findRoot->second;
                }
                else
                {
                    context = std::make_shared<NVMeContext>(io, rootBus);
                    nvmeDeviceMap[rootBus] = context;
                }

                std::shared_ptr<NVMeSensor> sensorPtr =
                    std::make_shared<NVMeSensor>(
                        objectServer, io, dbusConnection, sensorName,
                        std::move(sensorThresholds), interfacePath, busNumber);

                context->sensors.emplace_back(sensorPtr);
            }
            for (const auto& [_, context] : nvmeDeviceMap)
            {
                context->pollNVMeDevices();
            }
        }));
    getter->getConfiguration(std::vector<std::string>{sensorType});
}

int main()
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name("xyz.openbmc_project.NVMeSensor");
    sdbusplus::asio::object_server objectServer(systemBus);
    nvmeMCTP::init();

    io.post([&]() { createSensors(io, objectServer, systemBus); });

    boost::asio::deadline_timer filterTimer(io);
    std::function<void(sdbusplus::message::message&)> eventHandler =
        [&filterTimer, &io, &objectServer,
         &systemBus](sdbusplus::message::message& message) {
            // this implicitly cancels the timer
            filterTimer.expires_from_now(boost::posix_time::seconds(1));

            filterTimer.async_wait([&](const boost::system::error_code& ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    return; // we're being canceled
                }
                else if (ec)
                {
                    std::cerr << "Error: " << ec.message() << "\n";
                    return;
                }

                createSensors(io, objectServer, systemBus);
            });
        };

    sdbusplus::bus::match::match configMatch(
        static_cast<sdbusplus::bus::bus&>(*systemBus),
        "type='signal',member='PropertiesChanged',path_namespace='" +
            std::string(inventoryPath) + "',arg0namespace='" +
            std::string(sensorType) + "'",
        eventHandler);

    io.run();
}
