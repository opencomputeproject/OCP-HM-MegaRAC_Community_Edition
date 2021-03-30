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

#include "IpmbSensor.hpp"

#include "Utils.hpp"
#include "VariantVisitors.hpp"

#include <math.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>

#include <chrono>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

constexpr const bool debug = false;

constexpr const char* configInterface =
    "xyz.openbmc_project.Configuration.IpmbSensor";
static constexpr double ipmbMaxReading = 0xFF;
static constexpr double ipmbMinReading = 0;

static constexpr uint8_t meAddress = 1;
static constexpr uint8_t lun = 0;

static constexpr const char* sensorPathPrefix = "/xyz/openbmc_project/sensors/";

using IpmbMethodType =
    std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>;

boost::container::flat_map<std::string, std::unique_ptr<IpmbSensor>> sensors;

std::unique_ptr<boost::asio::deadline_timer> initCmdTimer;

IpmbSensor::IpmbSensor(std::shared_ptr<sdbusplus::asio::connection>& conn,
                       boost::asio::io_service& io,
                       const std::string& sensorName,
                       const std::string& sensorConfiguration,
                       sdbusplus::asio::object_server& objectServer,
                       std::vector<thresholds::Threshold>&& thresholdData,
                       uint8_t deviceAddress, std::string& sensorTypeName) :
    Sensor(boost::replace_all_copy(sensorName, " ", "_"),
           std::move(thresholdData), sensorConfiguration,
           "xyz.openbmc_project.Configuration.ExitAirTemp", ipmbMaxReading,
           ipmbMinReading, PowerState::on),
    deviceAddress(deviceAddress), objectServer(objectServer),
    dbusConnection(conn), waitTimer(io)
{
    std::string dbusPath = sensorPathPrefix + sensorTypeName + "/" + name;

    sensorInterface = objectServer.add_interface(
        dbusPath, "xyz.openbmc_project.Sensor.Value");

    if (thresholds::hasWarningInterface(thresholds))
    {
        thresholdInterfaceWarning = objectServer.add_interface(
            dbusPath, "xyz.openbmc_project.Sensor.Threshold.Warning");
    }
    if (thresholds::hasCriticalInterface(thresholds))
    {
        thresholdInterfaceCritical = objectServer.add_interface(
            dbusPath, "xyz.openbmc_project.Sensor.Threshold.Critical");
    }
    association = objectServer.add_interface(dbusPath, association::interface);
}

IpmbSensor::~IpmbSensor()
{
    waitTimer.cancel();
    objectServer.remove_interface(thresholdInterfaceWarning);
    objectServer.remove_interface(thresholdInterfaceCritical);
    objectServer.remove_interface(sensorInterface);
    objectServer.remove_interface(association);
}

void IpmbSensor::init(void)
{
    loadDefaults();
    setInitialProperties(dbusConnection);
    if (initCommand)
    {
        runInitCmd();
    }
    read();
}

void IpmbSensor::runInitCmd()
{
    if (initCommand)
    {
        dbusConnection->async_method_call(
            [this](boost::system::error_code ec,
                   const IpmbMethodType& response) {
                const int& status = std::get<0>(response);

                if (ec || status)
                {
                    std::cerr
                        << "Error setting init command for device: " << name
                        << "\n";
                }
            },
            "xyz.openbmc_project.Ipmi.Channel.Ipmb",
            "/xyz/openbmc_project/Ipmi/Channel/Ipmb", "org.openbmc.Ipmb",
            "sendRequest", commandAddress, netfn, lun, *initCommand, initData);
    }
}

void IpmbSensor::loadDefaults()
{
    if (type == IpmbType::meSensor)
    {
        commandAddress = meAddress;
        netfn = 0x4;    // sensor
        command = 0x2d; // get sensor reading
        commandData = {deviceAddress};
        readingFormat = ReadingFormat::byte0;
    }
    else if (type == IpmbType::PXE1410CVR)
    {
        commandAddress = meAddress;
        netfn = 0x2e;       // me bridge
        command = 0xd9;     // send raw pmbus
        initCommand = 0xd9; // send raw pmbus
        // pmbus read temp
        commandData = {0x57, 0x01, 0x00, 0x16, 0x3,  deviceAddress, 0x00,
                       0x00, 0x00, 0x00, 0x01, 0x02, 0x8d};
        // goto page 0
        initData = {0x57, 0x01, 0x00, 0x14, 0x03, deviceAddress, 0x00,
                    0x00, 0x00, 0x00, 0x02, 0x00, 0x00,          0x00};
        readingFormat = ReadingFormat::byte3;
    }
    else if (type == IpmbType::IR38363VR)
    {
        commandAddress = meAddress;
        netfn = 0x2e;   // me bridge
        command = 0xd9; // send raw pmbus
        // pmbus read temp
        commandData = {0x57, 0x01, 0x00, 0x16, 0x03, deviceAddress, 00,
                       0x00, 0x00, 0x00, 0x01, 0x02, 0x8D};
        readingFormat = ReadingFormat::elevenBitShift;
    }
    else if (type == IpmbType::ADM1278HSC)
    {
        commandAddress = meAddress;
        switch (subType)
        {
            case IpmbSubType::temp:
            case IpmbSubType::curr:
                uint8_t snsNum;
                if (subType == IpmbSubType::temp)
                    snsNum = 0x8d;
                else
                    snsNum = 0x8c;
                netfn = 0x2e;   // me bridge
                command = 0xd9; // send raw pmbus
                commandData = {0x57, 0x01, 0x00, 0x86, deviceAddress,
                               0x00, 0x00, 0x01, 0x02, snsNum};
                readingFormat = ReadingFormat::elevenBit;
                break;
            case IpmbSubType::power:
            case IpmbSubType::volt:
                netfn = 0x4;    // sensor
                command = 0x2d; // get sensor reading
                commandData = {deviceAddress};
                readingFormat = ReadingFormat::byte0;
                break;
            default:
                throw std::runtime_error("Invalid sensor type");
        }
    }
    else if (type == IpmbType::mpsVR)
    {
        commandAddress = meAddress;
        netfn = 0x2e;       // me bridge
        command = 0xd9;     // send raw pmbus
        initCommand = 0xd9; // send raw pmbus
        // pmbus read temp
        commandData = {0x57, 0x01, 0x00, 0x16, 0x3,  deviceAddress, 0x00,
                       0x00, 0x00, 0x00, 0x01, 0x02, 0x8d};
        // goto page 0
        initData = {0x57, 0x01, 0x00, 0x14, 0x03, deviceAddress, 0x00,
                    0x00, 0x00, 0x00, 0x02, 0x00, 0x00,          0x00};
        readingFormat = ReadingFormat::byte3;
    }
    else
    {
        throw std::runtime_error("Invalid sensor type");
    }

    if (subType == IpmbSubType::util)
    {
        // Utilization need to be scaled to percent
        maxValue = 100;
        minValue = 0;
    }
}

void IpmbSensor::checkThresholds(void)
{
    thresholds::checkThresholds(this);
}

bool IpmbSensor::processReading(const std::vector<uint8_t>& data, double& resp)
{

    switch (readingFormat)
    {
        case (ReadingFormat::byte0):
            resp = data[0];
            return true;
        case (ReadingFormat::byte3):
            if (data.size() < 4)
            {
                if (!errCount)
                {
                    std::cerr << "Invalid data length returned for " << name
                              << "\n";
                }
                return false;
            }
            resp = data[3];
            return true;
        case (ReadingFormat::elevenBit):
            if (data.size() < 5)
            {
                if (!errCount)
                {
                    std::cerr << "Invalid data length returned for " << name
                              << "\n";
                }
                return false;
            }

            resp = ((data[4] << 8) | data[3]);
            return true;
        case (ReadingFormat::elevenBitShift):
            if (data.size() < 5)
            {
                if (!errCount)
                {
                    std::cerr << "Invalid data length returned for " << name
                              << "\n";
                }
                return false;
            }

            resp = ((data[4] << 8) | data[3]) >> 3;
            return true;
        default:
            throw std::runtime_error("Invalid reading type");
    }
}

void IpmbSensor::read(void)
{
    static constexpr size_t pollTime = 1; // in seconds

    waitTimer.expires_from_now(boost::posix_time::seconds(pollTime));
    waitTimer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return; // we're being canceled
        }
        if (!isPowerOn() && readState != PowerState::always)
        {
            read();
            return;
        }
        dbusConnection->async_method_call(
            [this](boost::system::error_code ec,
                   const IpmbMethodType& response) {
                const int& status = std::get<0>(response);
                if (ec || status)
                {
                    incrementError();
                    read();
                    return;
                }
                const std::vector<uint8_t>& data = std::get<5>(response);
                if constexpr (debug)
                {
                    std::cout << name << ": ";
                    for (size_t d : data)
                    {
                        std::cout << d << " ";
                    }
                    std::cout << "\n";
                }
                if (data.empty())
                {
                    incrementError();
                    read();
                    return;
                }

                double value = 0;

                if (!processReading(data, value))
                {
                    incrementError();
                    read();
                    return;
                }

                /* Adjust value as per scale and offset */
                value = (value * scaleVal) + offsetVal;
                updateValue(value);
                read();
            },
            "xyz.openbmc_project.Ipmi.Channel.Ipmb",
            "/xyz/openbmc_project/Ipmi/Channel/Ipmb", "org.openbmc.Ipmb",
            "sendRequest", commandAddress, netfn, lun, command, commandData);
    });
}
void createSensors(
    boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,
    boost::container::flat_map<std::string, std::unique_ptr<IpmbSensor>>&
        sensors,
    std::shared_ptr<sdbusplus::asio::connection>& dbusConnection)
{
    if (!dbusConnection)
    {
        std::cerr << "Connection not created\n";
        return;
    }
    dbusConnection->async_method_call(
        [&](boost::system::error_code ec, const ManagedObjectType& resp) {
            if (ec)
            {
                std::cerr << "Error contacting entity manager\n";
                return;
            }
            for (const auto& pathPair : resp)
            {
                for (const auto& entry : pathPair.second)
                {
                    if (entry.first != configInterface)
                    {
                        continue;
                    }
                    std::string name =
                        loadVariant<std::string>(entry.second, "Name");

                    std::vector<thresholds::Threshold> sensorThresholds;
                    if (!parseThresholdsFromConfig(pathPair.second,
                                                   sensorThresholds))
                    {
                        std::cerr << "error populating thresholds for " << name
                                  << "\n";
                    }
                    uint8_t deviceAddress =
                        loadVariant<uint8_t>(entry.second, "Address");

                    std::string sensorClass =
                        loadVariant<std::string>(entry.second, "Class");

                    /* Default sensor type is "temperature" */
                    std::string sensorTypeName = "temperature";
                    auto findType = entry.second.find("SensorType");
                    if (findType != entry.second.end())
                    {
                        sensorTypeName = std::visit(VariantToStringVisitor(),
                                                    findType->second);
                    }

                    auto& sensor = sensors[name];
                    sensor = std::make_unique<IpmbSensor>(
                        dbusConnection, io, name, pathPair.first, objectServer,
                        std::move(sensorThresholds), deviceAddress,
                        sensorTypeName);

                    /* Initialize scale and offset value */
                    sensor->scaleVal = 1;
                    sensor->offsetVal = 0;

                    auto findScaleVal = entry.second.find("ScaleValue");
                    if (findScaleVal != entry.second.end())
                    {
                        sensor->scaleVal = std::visit(VariantToDoubleVisitor(),
                                                      findScaleVal->second);
                    }

                    auto findOffsetVal = entry.second.find("OffsetValue");
                    if (findOffsetVal != entry.second.end())
                    {
                        sensor->offsetVal = std::visit(VariantToDoubleVisitor(),
                                                       findOffsetVal->second);
                    }

                    auto findPowerState = entry.second.find("PowerState");

                    if (findPowerState != entry.second.end())
                    {
                        std::string powerState = std::visit(
                            VariantToStringVisitor(), findPowerState->second);

                        setReadState(powerState, sensor->readState);
                    }

                    if (sensorClass == "PxeBridgeTemp")
                    {
                        sensor->type = IpmbType::PXE1410CVR;
                    }
                    else if (sensorClass == "IRBridgeTemp")
                    {
                        sensor->type = IpmbType::IR38363VR;
                    }
                    else if (sensorClass == "HSCBridge")
                    {
                        sensor->type = IpmbType::ADM1278HSC;
                    }
                    else if (sensorClass == "MpsBridgeTemp")
                    {
                        sensor->type = IpmbType::mpsVR;
                    }
                    else if (sensorClass == "METemp" ||
                             sensorClass == "MESensor")
                    {
                        sensor->type = IpmbType::meSensor;
                    }
                    else
                    {
                        std::cerr << "Invalid class " << sensorClass << "\n";
                        continue;
                    }

                    if (sensorTypeName == "voltage")
                    {
                        sensor->subType = IpmbSubType::volt;
                    }
                    else if (sensorTypeName == "power")
                    {
                        sensor->subType = IpmbSubType::power;
                    }
                    else if (sensorTypeName == "current")
                    {
                        sensor->subType = IpmbSubType::curr;
                    }
                    else if (sensorTypeName == "utilization")
                    {
                        sensor->subType = IpmbSubType::util;
                    }
                    else
                    {
                        sensor->subType = IpmbSubType::temp;
                    }
                    sensor->init();
                }
            }
        },
        entityManagerName, "/", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
}

void reinitSensors(sdbusplus::message::message& message)
{
    constexpr const size_t reinitWaitSeconds = 2;
    std::string objectName;
    boost::container::flat_map<std::string, std::variant<std::string>> values;
    message.read(objectName, values);

    auto findStatus = values.find(power::property);
    if (findStatus != values.end())
    {
        bool powerStatus = boost::ends_with(
            std::get<std::string>(findStatus->second), "Running");
        if (powerStatus)
        {
            if (!initCmdTimer)
            {
                // this should be impossible
                return;
            }
            // we seem to send this command too fast sometimes, wait before
            // sending
            initCmdTimer->expires_from_now(
                boost::posix_time::seconds(reinitWaitSeconds));

            initCmdTimer->async_wait([](const boost::system::error_code ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    return; // we're being canceled
                }

                for (const auto& sensor : sensors)
                {
                    if (sensor.second)
                    {
                        sensor.second->runInitCmd();
                    }
                }
            });
        }
    }
}

int main()
{

    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    systemBus->request_name("xyz.openbmc_project.IpmbSensor");
    sdbusplus::asio::object_server objectServer(systemBus);

    initCmdTimer = std::make_unique<boost::asio::deadline_timer>(io);

    io.post([&]() { createSensors(io, objectServer, sensors, systemBus); });

    boost::asio::deadline_timer configTimer(io);

    std::function<void(sdbusplus::message::message&)> eventHandler =
        [&](sdbusplus::message::message&) {
            configTimer.expires_from_now(boost::posix_time::seconds(1));
            // create a timer because normally multiple properties change
            configTimer.async_wait([&](const boost::system::error_code& ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    return; // we're being canceled
                }
                createSensors(io, objectServer, sensors, systemBus);
                if (sensors.empty())
                {
                    std::cout << "Configuration not detected\n";
                }
            });
        };

    sdbusplus::bus::match::match configMatch(
        static_cast<sdbusplus::bus::bus&>(*systemBus),
        "type='signal',member='PropertiesChanged',path_namespace='" +
            std::string(inventoryPath) + "',arg0namespace='" + configInterface +
            "'",
        eventHandler);

    sdbusplus::bus::match::match powerChangeMatch(
        static_cast<sdbusplus::bus::bus&>(*systemBus),
        "type='signal',interface='" + std::string(properties::interface) +
            "',path='" + std::string(power::path) + "',arg0='" +
            std::string(power::interface) + "'",
        reinitSensors);

    io.run();
}
