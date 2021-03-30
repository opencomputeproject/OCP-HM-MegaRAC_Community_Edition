/**
 * Copyright © 2019 IBM Corporation
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
#include "config.h"

#include "data_interface.hpp"

#include "util.hpp"

#include <fstream>
#include <xyz/openbmc_project/State/OperatingSystem/Status/server.hpp>

namespace openpower
{
namespace pels
{

namespace service_name
{
constexpr auto objectMapper = "xyz.openbmc_project.ObjectMapper";
} // namespace service_name

namespace object_path
{
constexpr auto objectMapper = "/xyz/openbmc_project/object_mapper";
constexpr auto systemInv = "/xyz/openbmc_project/inventory/system";
constexpr auto chassisInv = "/xyz/openbmc_project/inventory/system/chassis";
constexpr auto baseInv = "/xyz/openbmc_project/inventory";
constexpr auto bmcState = "/xyz/openbmc_project/state/bmc0";
constexpr auto chassisState = "/xyz/openbmc_project/state/chassis0";
constexpr auto hostState = "/xyz/openbmc_project/state/host0";
constexpr auto pldm = "/xyz/openbmc_project/pldm";
constexpr auto enableHostPELs =
    "/xyz/openbmc_project/logging/send_event_logs_to_host";
} // namespace object_path

namespace interface
{
constexpr auto dbusProperty = "org.freedesktop.DBus.Properties";
constexpr auto objectMapper = "xyz.openbmc_project.ObjectMapper";
constexpr auto invAsset = "xyz.openbmc_project.Inventory.Decorator.Asset";
constexpr auto osStatus = "xyz.openbmc_project.State.OperatingSystem.Status";
constexpr auto pldmRequester = "xyz.openbmc_project.PLDM.Requester";
constexpr auto enable = "xyz.openbmc_project.Object.Enable";
constexpr auto bmcState = "xyz.openbmc_project.State.BMC";
constexpr auto chassisState = "xyz.openbmc_project.State.Chassis";
constexpr auto hostState = "xyz.openbmc_project.State.Host";
constexpr auto invMotherboard =
    "xyz.openbmc_project.Inventory.Item.Board.Motherboard";
constexpr auto viniRecordVPD = "com.ibm.ipzvpd.VINI";
constexpr auto locCode = "com.ibm.ipzvpd.Location";
constexpr auto invCompatible =
    "xyz.openbmc_project.Inventory.Decorator.Compatible";
} // namespace interface

using namespace sdbusplus::xyz::openbmc_project::State::OperatingSystem::server;
using sdbusplus::exception::SdBusError;

DataInterface::DataInterface(sdbusplus::bus::bus& bus) : _bus(bus)
{
    readBMCFWVersion();
    readServerFWVersion();
    readBMCFWVersionID();
    readMotherboardCCIN();

    // Watch both the Model and SN properties on the system's Asset iface
    _properties.emplace_back(std::make_unique<InterfaceWatcher<DataInterface>>(
        bus, object_path::systemInv, interface::invAsset, *this,
        [this](const auto& properties) {
            auto model = properties.find("Model");
            if (model != properties.end())
            {
                this->_machineTypeModel = std::get<std::string>(model->second);
            }

            auto sn = properties.find("SerialNumber");
            if (sn != properties.end())
            {
                this->_machineSerialNumber = std::get<std::string>(sn->second);
            }
        }));

    // Watch the OperatingSystemState property
    _properties.emplace_back(std::make_unique<PropertyWatcher<DataInterface>>(
        bus, object_path::hostState, interface::osStatus,
        "OperatingSystemState", *this, [this](const auto& value) {
            auto status =
                Status::convertOSStatusFromString(std::get<std::string>(value));

            if ((status == Status::OSStatus::BootComplete) ||
                (status == Status::OSStatus::Standby))
            {
                setHostUp(true);
            }
            else
            {
                setHostUp(false);
            }
        }));

    // Watch the host PEL enable property
    _properties.emplace_back(std::make_unique<PropertyWatcher<DataInterface>>(
        bus, object_path::enableHostPELs, interface::enable, "Enabled", *this,
        [this](const auto& value) {
            this->_sendPELsToHost = std::get<bool>(value);
        }));

    // Watch the BMCState property
    _properties.emplace_back(std::make_unique<PropertyWatcher<DataInterface>>(
        bus, object_path::bmcState, interface::bmcState, "CurrentBMCState",
        *this, [this](const auto& value) {
            this->_bmcState = std::get<std::string>(value);
        }));

    // Watch the chassis current and requested power state properties
    _properties.emplace_back(std::make_unique<InterfaceWatcher<DataInterface>>(
        bus, object_path::chassisState, interface::chassisState, *this,
        [this](const auto& properties) {
            auto state = properties.find("CurrentPowerState");
            if (state != properties.end())
            {
                this->_chassisState = std::get<std::string>(state->second);
            }

            auto trans = properties.find("RequestedPowerTransition");
            if (trans != properties.end())
            {
                this->_chassisTransition = std::get<std::string>(trans->second);
            }
        }));

    // Watch the CurrentHostState property
    _properties.emplace_back(std::make_unique<PropertyWatcher<DataInterface>>(
        bus, object_path::hostState, interface::hostState, "CurrentHostState",
        *this, [this](const auto& value) {
            this->_hostState = std::get<std::string>(value);
        }));

    // Watch the compatible system names property
    _properties.emplace_back(std::make_unique<PropertyWatcher<DataInterface>>(
        bus, object_path::chassisInv, interface::invCompatible, "Names", *this,
        [this](const auto& value) {
            this->_systemNames = std::get<std::vector<std::string>>(value);
        }));
}

DBusPropertyMap
    DataInterface::getAllProperties(const std::string& service,
                                    const std::string& objectPath,
                                    const std::string& interface) const
{
    DBusPropertyMap properties;

    auto method = _bus.new_method_call(service.c_str(), objectPath.c_str(),
                                       interface::dbusProperty, "GetAll");
    method.append(interface);
    auto reply = _bus.call(method);

    reply.read(properties);

    return properties;
}

void DataInterface::getProperty(const std::string& service,
                                const std::string& objectPath,
                                const std::string& interface,
                                const std::string& property,
                                DBusValue& value) const
{

    auto method = _bus.new_method_call(service.c_str(), objectPath.c_str(),
                                       interface::dbusProperty, "Get");
    method.append(interface, property);
    auto reply = _bus.call(method);

    reply.read(value);
}

DBusPathList DataInterface::getPaths(const DBusInterfaceList& interfaces) const
{

    auto method = _bus.new_method_call(
        service_name::objectMapper, object_path::objectMapper,
        interface::objectMapper, "GetSubTreePaths");

    method.append(std::string{"/"}, 0, interfaces);

    auto reply = _bus.call(method);

    DBusPathList paths;
    reply.read(paths);

    return paths;
}

DBusService DataInterface::getService(const std::string& objectPath,
                                      const std::string& interface) const
{
    auto method = _bus.new_method_call(service_name::objectMapper,
                                       object_path::objectMapper,
                                       interface::objectMapper, "GetObject");

    method.append(objectPath, std::vector<std::string>({interface}));

    auto reply = _bus.call(method);

    std::map<DBusService, DBusInterfaceList> response;
    reply.read(response);

    if (!response.empty())
    {
        return response.begin()->first;
    }

    return std::string{};
}

void DataInterface::readBMCFWVersion()
{
    _bmcFWVersion =
        phosphor::logging::util::getOSReleaseValue("VERSION").value_or("");
}

void DataInterface::readServerFWVersion()
{
    // Not available yet
}

void DataInterface::readBMCFWVersionID()
{
    _bmcFWVersionID =
        phosphor::logging::util::getOSReleaseValue("VERSION_ID").value_or("");
}

void DataInterface::readMotherboardCCIN()
{
    try
    {
        // First, try to find the motherboard
        auto motherboards = getPaths({interface::invMotherboard});
        if (motherboards.empty())
        {
            throw std::runtime_error("No motherboards yet");
        }

        // Found it, so now get the CCIN
        _properties.emplace_back(
            std::make_unique<PropertyWatcher<DataInterface>>(
                _bus, motherboards.front(), interface::viniRecordVPD, "CC",
                *this,
                [this](const auto& ccin) { this->setMotherboardCCIN(ccin); }));
    }
    catch (const std::exception& e)
    {
        // No motherboard in the inventory yet - watch for it
        _inventoryIfacesAddedMatch = std::make_unique<sdbusplus::bus::match_t>(
            _bus, match_rules::interfacesAdded(object_path::baseInv),
            std::bind(std::mem_fn(&DataInterface::motherboardIfaceAdded), this,
                      std::placeholders::_1));
    }
}

void DataInterface::motherboardIfaceAdded(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path path;
    DBusInterfaceMap interfaces;

    msg.read(path, interfaces);

    // This is watching the whole inventory, so check if it's what we want
    if (interfaces.find(interface::invMotherboard) == interfaces.end())
    {
        return;
    }

    // Done watching for any new inventory interfaces
    _inventoryIfacesAddedMatch.reset();

    // Watch the motherboard CCIN, using the service from this signal
    // for the initial property read.
    _properties.emplace_back(std::make_unique<PropertyWatcher<DataInterface>>(
        _bus, path, interface::viniRecordVPD, "CC", msg.get_sender(), *this,
        [this](const auto& ccin) { this->setMotherboardCCIN(ccin); }));
}

void DataInterface::getHWCalloutFields(const std::string& inventoryPath,
                                       std::string& fruPartNumber,
                                       std::string& ccin,
                                       std::string& serialNumber) const
{
    // For now, attempt to get all of the properties directly on the path
    // passed in.  In the future, may need to make use of an algorithm
    // to figure out which inventory objects actually hold these
    // interfaces in the case of non FRUs, or possibly another service
    // will provide this info.  Any missing interfaces will result
    // in exceptions being thrown.

    auto service = getService(inventoryPath, interface::viniRecordVPD);

    auto properties =
        getAllProperties(service, inventoryPath, interface::viniRecordVPD);

    auto value = std::get<std::vector<uint8_t>>(properties["FN"]);
    fruPartNumber = std::string{value.begin(), value.end()};

    value = std::get<std::vector<uint8_t>>(properties["CC"]);
    ccin = std::string{value.begin(), value.end()};

    value = std::get<std::vector<uint8_t>>(properties["SN"]);
    serialNumber = std::string{value.begin(), value.end()};
}

std::string
    DataInterface::getLocationCode(const std::string& inventoryPath) const
{
    auto service = getService(inventoryPath, interface::locCode);

    DBusValue locCode;
    getProperty(service, inventoryPath, interface::locCode, "LocationCode",
                locCode);

    return std::get<std::string>(locCode);
}

std::string
    DataInterface::addLocationCodePrefix(const std::string& locationCode)
{
    static const std::string locationCodePrefix{"Ufcs-"};

    if (locationCode.find(locationCodePrefix) == std::string::npos)
    {
        return locationCodePrefix + locationCode;
    }

    return locationCode;
}

std::string DataInterface::expandLocationCode(const std::string& locationCode,
                                              uint16_t node) const
{
    // TODO: fill in when that API is available
    return addLocationCodePrefix(locationCode);
}

std::string DataInterface::getInventoryFromLocCode(
    const std::string& unexpandedLocationCode, uint16_t node) const
{
    // TODO: fill in when that API is available and  call on
    // addLocationCodePrefix(unexpandedLocationCode);
    return {};
}

} // namespace pels
} // namespace openpower
