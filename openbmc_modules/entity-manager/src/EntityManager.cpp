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

#include "EntityManager.hpp"

#include "Overlay.hpp"
#include "Utils.hpp"
#include "VariantVisitors.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/range/iterator_range.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <variant>
constexpr const char* configurationDirectory = PACKAGE_DIR "configurations";
constexpr const char* schemaDirectory = PACKAGE_DIR "configurations/schemas";
constexpr const char* tempConfigDir = "/tmp/configuration/";
constexpr const char* lastConfiguration = "/tmp/configuration/last.json";
constexpr const char* currentConfiguration = "/var/configuration/system.json";
constexpr const char* globalSchema = "global.json";
constexpr const int32_t MAX_MAPPER_DEPTH = 0;

constexpr const bool DEBUG = false;

struct cmp_str
{
    bool operator()(const char* a, const char* b) const
    {
        return std::strcmp(a, b) < 0;
    }
};

// underscore T for collison with dbus c api
enum class probe_type_codes
{
    FALSE_T,
    TRUE_T,
    AND,
    OR,
    FOUND,
    MATCH_ONE
};
const static boost::container::flat_map<const char*, probe_type_codes, cmp_str>
    PROBE_TYPES{{{"FALSE", probe_type_codes::FALSE_T},
                 {"TRUE", probe_type_codes::TRUE_T},
                 {"AND", probe_type_codes::AND},
                 {"OR", probe_type_codes::OR},
                 {"FOUND", probe_type_codes::FOUND},
                 {"MATCH_ONE", probe_type_codes::MATCH_ONE}}};

static constexpr std::array<const char*, 6> settableInterfaces = {
    "FanProfile", "Pid", "Pid.Zone", "Stepwise", "Thresholds", "Polling"};
using JsonVariantType =
    std::variant<std::vector<std::string>, std::vector<double>, std::string,
                 int64_t, uint64_t, double, int32_t, uint32_t, int16_t,
                 uint16_t, uint8_t, bool>;
using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using ManagedObjectType = boost::container::flat_map<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string,
        boost::container::flat_map<std::string, BasicVariantType>>>;

// store reference to all interfaces so we can destroy them later
boost::container::flat_map<
    std::string, std::vector<std::weak_ptr<sdbusplus::asio::dbus_interface>>>
    inventory;

// todo: pass this through nicer
std::shared_ptr<sdbusplus::asio::connection> SYSTEM_BUS;
static nlohmann::json lastJson;

boost::asio::io_context io;

const std::regex ILLEGAL_DBUS_PATH_REGEX("[^A-Za-z0-9_.]");
const std::regex ILLEGAL_DBUS_MEMBER_REGEX("[^A-Za-z0-9_]");

void registerCallback(nlohmann::json& systemConfiguration,
                      sdbusplus::asio::object_server& objServer,
                      const std::string& interfaces);

static std::shared_ptr<sdbusplus::asio::dbus_interface>
    createInterface(sdbusplus::asio::object_server& objServer,
                    const std::string& path, const std::string& interface,
                    const std::string& parent, bool checkNull = false)
{
    // on first add we have no reason to check for null before add, as there
    // won't be any. For dynamically added interfaces, we check for null so that
    // a constant delete/add will not create a memory leak

    auto ptr = objServer.add_interface(path, interface);
    auto& dataVector = inventory[parent];
    if (checkNull)
    {
        auto it = std::find_if(dataVector.begin(), dataVector.end(),
                               [](const auto& p) { return p.expired(); });
        if (it != dataVector.end())
        {
            *it = ptr;
            return ptr;
        }
    }
    dataVector.emplace_back(ptr);
    return ptr;
}

void getInterfaces(
    const std::tuple<std::string, std::string, std::string>& call,
    const std::vector<std::shared_ptr<PerformProbe>>& probeVector,
    std::shared_ptr<PerformScan> scan, size_t retries = 5)
{
    if (!retries)
    {
        std::cerr << "retries exhausted on " << std::get<0>(call) << " "
                  << std::get<1>(call) << " " << std::get<2>(call) << "\n";
        return;
    }

    SYSTEM_BUS->async_method_call(
        [call, scan, probeVector, retries](
            boost::system::error_code& errc,
            const boost::container::flat_map<std::string, BasicVariantType>&
                resp) {
            if (errc)
            {
                std::cerr << "error calling getall on  " << std::get<0>(call)
                          << " " << std::get<1>(call) << " "
                          << std::get<2>(call) << "\n";

                std::shared_ptr<boost::asio::steady_timer> timer =
                    std::make_shared<boost::asio::steady_timer>(io);
                timer->expires_after(std::chrono::seconds(2));

                timer->async_wait([timer, call, scan, probeVector,
                                   retries](const boost::system::error_code&) {
                    getInterfaces(call, probeVector, scan, retries - 1);
                });
                return;
            }

            scan->dbusProbeObjects[std::get<2>(call)].emplace_back(resp);
        },
        std::get<0>(call), std::get<1>(call), "org.freedesktop.DBus.Properties",
        "GetAll", std::get<2>(call));

    if constexpr (DEBUG)
    {
        std::cerr << __func__ << " " << __LINE__ << "\n";
    }
}

// calls the mapper to find all exposed objects of an interface type
// and creates a vector<flat_map> that contains all the key value pairs
// getManagedObjects
void findDbusObjects(std::vector<std::shared_ptr<PerformProbe>>&& probeVector,
                     boost::container::flat_set<std::string>&& interfaces,
                     std::shared_ptr<PerformScan> scan)
{

    for (const auto& [interface, _] : scan->dbusProbeObjects)
    {
        interfaces.erase(interface);
    }
    if (interfaces.empty())
    {
        return;
    }

    // find all connections in the mapper that expose a specific type
    SYSTEM_BUS->async_method_call(
        [interfaces{std::move(interfaces)}, probeVector{std::move(probeVector)},
         scan](boost::system::error_code& ec,
               const GetSubTreeType& interfaceSubtree) {
            boost::container::flat_set<
                std::tuple<std::string, std::string, std::string>>
                interfaceConnections;
            if (ec)
            {
                if (ec.value() == ENOENT)
                {
                    return; // wasn't found by mapper
                }
                std::cerr << "Error communicating to mapper.\n";

                // if we can't communicate to the mapper something is very wrong
                std::exit(EXIT_FAILURE);
            }

            for (const auto& [path, object] : interfaceSubtree)
            {
                for (const auto& [busname, ifaces] : object)
                {
                    for (const std::string& iface : ifaces)
                    {
                        auto ifaceObjFind = interfaces.find(iface);

                        if (ifaceObjFind != interfaces.end())
                        {
                            interfaceConnections.emplace(busname, path, iface);
                        }
                    }
                }
            }

            if (interfaceConnections.empty())
            {
                return;
            }

            for (const auto& call : interfaceConnections)
            {
                getInterfaces(call, probeVector, scan);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", MAX_MAPPER_DEPTH,
        interfaces);

    if constexpr (DEBUG)
    {
        std::cerr << __func__ << " " << __LINE__ << "\n";
    }
}

// probes dbus interface dictionary for a key with a value that matches a regex
bool probeDbus(const std::string& interface,
               const std::map<std::string, nlohmann::json>& matches,
               FoundDeviceT& devices, std::shared_ptr<PerformScan> scan,
               bool& foundProbe)
{
    std::vector<boost::container::flat_map<std::string, BasicVariantType>>&
        dbusObject = scan->dbusProbeObjects[interface];
    if (dbusObject.empty())
    {
        foundProbe = false;
        return false;
    }
    foundProbe = true;

    bool foundMatch = false;
    for (auto& device : dbusObject)
    {
        bool deviceMatches = true;
        for (auto& match : matches)
        {
            auto deviceValue = device.find(match.first);
            if (deviceValue != device.end())
            {
                switch (match.second.type())
                {
                    case nlohmann::json::value_t::string:
                    {
                        std::regex search(match.second.get<std::string>());
                        std::smatch regMatch;

                        // convert value to string respresentation
                        std::string probeValue = std::visit(
                            VariantToStringVisitor(), deviceValue->second);
                        if (!std::regex_search(probeValue, regMatch, search))
                        {
                            deviceMatches = false;
                            break;
                        }
                        break;
                    }
                    case nlohmann::json::value_t::boolean:
                    case nlohmann::json::value_t::number_unsigned:
                    {
                        unsigned int probeValue = std::visit(
                            VariantToUnsignedIntVisitor(), deviceValue->second);

                        if (probeValue != match.second.get<unsigned int>())
                        {
                            deviceMatches = false;
                        }
                        break;
                    }
                    case nlohmann::json::value_t::number_integer:
                    {
                        int probeValue = std::visit(VariantToIntVisitor(),
                                                    deviceValue->second);

                        if (probeValue != match.second.get<int>())
                        {
                            deviceMatches = false;
                        }
                        break;
                    }
                    case nlohmann::json::value_t::number_float:
                    {
                        float probeValue = std::visit(VariantToFloatVisitor(),
                                                      deviceValue->second);

                        if (probeValue != match.second.get<float>())
                        {
                            deviceMatches = false;
                        }
                        break;
                    }
                    default:
                    {
                        std::cerr << "unexpected dbus probe type "
                                  << match.second.type_name() << "\n";
                    }
                }
            }
            else
            {
                deviceMatches = false;
                break;
            }
        }
        if (deviceMatches)
        {
            devices.emplace_back(device);
            foundMatch = true;
            deviceMatches = false; // for next iteration
        }
    }
    return foundMatch;
}

// default probe entry point, iterates a list looking for specific types to
// call specific probe functions
bool probe(
    const std::vector<std::string>& probeCommand,
    std::shared_ptr<PerformScan> scan,
    std::vector<boost::container::flat_map<std::string, BasicVariantType>>&
        foundDevs)
{
    const static std::regex command(R"(\((.*)\))");
    std::smatch match;
    bool ret = false;
    bool matchOne = false;
    bool cur = true;
    probe_type_codes lastCommand = probe_type_codes::FALSE_T;
    bool first = true;

    for (auto& probe : probeCommand)
    {
        bool foundProbe = false;
        boost::container::flat_map<const char*, probe_type_codes,
                                   cmp_str>::const_iterator probeType;

        for (probeType = PROBE_TYPES.begin(); probeType != PROBE_TYPES.end();
             ++probeType)
        {
            if (probe.find(probeType->first) != std::string::npos)
            {
                foundProbe = true;
                break;
            }
        }
        if (foundProbe)
        {
            switch (probeType->second)
            {
                case probe_type_codes::FALSE_T:
                {
                    cur = false;
                    break;
                }
                case probe_type_codes::TRUE_T:
                {
                    cur = true;
                    break;
                }
                case probe_type_codes::MATCH_ONE:
                {
                    // set current value to last, this probe type shouldn't
                    // affect the outcome
                    cur = ret;
                    matchOne = true;
                    break;
                }
                /*case probe_type_codes::AND:
                  break;
                case probe_type_codes::OR:
                  break;
                  // these are no-ops until the last command switch
                  */
                case probe_type_codes::FOUND:
                {
                    if (!std::regex_search(probe, match, command))
                    {
                        std::cerr << "found probe syntax error " << probe
                                  << "\n";
                        return false;
                    }
                    std::string commandStr = *(match.begin() + 1);
                    boost::replace_all(commandStr, "'", "");
                    cur = (std::find(scan->passedProbes.begin(),
                                     scan->passedProbes.end(),
                                     commandStr) != scan->passedProbes.end());
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
        // look on dbus for object
        else
        {
            if (!std::regex_search(probe, match, command))
            {
                std::cerr << "dbus probe syntax error " << probe << "\n";
                return false;
            }
            std::string commandStr = *(match.begin() + 1);
            // convert single ticks and single slashes into legal json
            boost::replace_all(commandStr, "'", "\"");
            boost::replace_all(commandStr, R"(\)", R"(\\)");
            auto json = nlohmann::json::parse(commandStr, nullptr, false);
            if (json.is_discarded())
            {
                std::cerr << "dbus command syntax error " << commandStr << "\n";
                return false;
            }
            // we can match any (string, variant) property. (string, string)
            // does a regex
            std::map<std::string, nlohmann::json> dbusProbeMap =
                json.get<std::map<std::string, nlohmann::json>>();
            auto findStart = probe.find("(");
            if (findStart == std::string::npos)
            {
                return false;
            }
            std::string probeInterface = probe.substr(0, findStart);
            cur = probeDbus(probeInterface, dbusProbeMap, foundDevs, scan,
                            foundProbe);
        }

        // some functions like AND and OR only take affect after the
        // fact
        if (lastCommand == probe_type_codes::AND)
        {
            ret = cur && ret;
        }
        else if (lastCommand == probe_type_codes::OR)
        {
            ret = cur || ret;
        }

        if (first)
        {
            ret = cur;
            first = false;
        }
        lastCommand = probeType != PROBE_TYPES.end()
                          ? probeType->second
                          : probe_type_codes::FALSE_T;
    }

    // probe passed, but empty device
    if (ret && foundDevs.size() == 0)
    {
        foundDevs.emplace_back(
            boost::container::flat_map<std::string, BasicVariantType>{});
    }
    if (matchOne && ret)
    {
        // match the last one
        auto last = foundDevs.back();
        foundDevs.clear();

        foundDevs.emplace_back(std::move(last));
    }
    return ret;
}

PerformProbe::PerformProbe(const std::vector<std::string>& probeCommand,
                           std::shared_ptr<PerformScan>& scanPtr,
                           std::function<void(FoundDeviceT&)>&& callback) :
    _probeCommand(probeCommand),
    scan(scanPtr), _callback(std::move(callback))
{}
PerformProbe::~PerformProbe()
{
    FoundDeviceT foundDevs;
    if (probe(_probeCommand, scan, foundDevs))
    {
        _callback(foundDevs);
    }
}

// writes output files to persist data
bool writeJsonFiles(const nlohmann::json& systemConfiguration)
{
    std::filesystem::create_directory(configurationOutDir);
    std::ofstream output(currentConfiguration);
    if (!output.good())
    {
        return false;
    }
    output << systemConfiguration.dump(4);
    output.close();
    return true;
}

template <typename JsonType>
bool setJsonFromPointer(const std::string& ptrStr, const JsonType& value,
                        nlohmann::json& systemConfiguration)
{
    try
    {
        nlohmann::json::json_pointer ptr(ptrStr);
        nlohmann::json& ref = systemConfiguration[ptr];
        ref = value;
        return true;
    }
    catch (const std::out_of_range&)
    {
        return false;
    }
}

// template function to add array as dbus property
template <typename PropertyType>
void addArrayToDbus(const std::string& name, const nlohmann::json& array,
                    sdbusplus::asio::dbus_interface* iface,
                    sdbusplus::asio::PropertyPermission permission,
                    nlohmann::json& systemConfiguration,
                    const std::string& jsonPointerString)
{
    std::vector<PropertyType> values;
    for (const auto& property : array)
    {
        auto ptr = property.get_ptr<const PropertyType*>();
        if (ptr != nullptr)
        {
            values.emplace_back(*ptr);
        }
    }

    if (permission == sdbusplus::asio::PropertyPermission::readOnly)
    {
        iface->register_property(name, values);
    }
    else
    {
        iface->register_property(
            name, values,
            [&systemConfiguration,
             jsonPointerString{std::string(jsonPointerString)}](
                const std::vector<PropertyType>& newVal,
                std::vector<PropertyType>& val) {
                val = newVal;
                if (!setJsonFromPointer(jsonPointerString, val,
                                        systemConfiguration))
                {
                    std::cerr << "error setting json field\n";
                    return -1;
                }
                if (!writeJsonFiles(systemConfiguration))
                {
                    std::cerr << "error setting json file\n";
                    return -1;
                }
                return 1;
            });
    }
}

template <typename PropertyType>
void addProperty(const std::string& propertyName, const PropertyType& value,
                 sdbusplus::asio::dbus_interface* iface,
                 nlohmann::json& systemConfiguration,
                 const std::string& jsonPointerString,
                 sdbusplus::asio::PropertyPermission permission)
{
    if (permission == sdbusplus::asio::PropertyPermission::readOnly)
    {
        iface->register_property(propertyName, value);
        return;
    }
    iface->register_property(
        propertyName, value,
        [&systemConfiguration,
         jsonPointerString{std::string(jsonPointerString)}](
            const PropertyType& newVal, PropertyType& val) {
            val = newVal;
            if (!setJsonFromPointer(jsonPointerString, val,
                                    systemConfiguration))
            {
                std::cerr << "error setting json field\n";
                return -1;
            }
            if (!writeJsonFiles(systemConfiguration))
            {
                std::cerr << "error setting json file\n";
                return -1;
            }
            return 1;
        });
}

void createDeleteObjectMethod(
    const std::string& jsonPointerPath,
    const std::shared_ptr<sdbusplus::asio::dbus_interface>& iface,
    sdbusplus::asio::object_server& objServer,
    nlohmann::json& systemConfiguration)
{
    std::weak_ptr<sdbusplus::asio::dbus_interface> interface = iface;
    iface->register_method(
        "Delete", [&objServer, &systemConfiguration, interface,
                   jsonPointerPath{std::string(jsonPointerPath)}]() {
            std::shared_ptr<sdbusplus::asio::dbus_interface> dbusInterface =
                interface.lock();
            if (!dbusInterface)
            {
                // this technically can't happen as the pointer is pointing to
                // us
                throw DBusInternalError();
            }
            nlohmann::json::json_pointer ptr(jsonPointerPath);
            systemConfiguration[ptr] = nullptr;

            // todo(james): dig through sdbusplus to find out why we can't
            // delete it in a method call
            io.post([&objServer, dbusInterface]() mutable {
                objServer.remove_interface(dbusInterface);
            });

            if (!writeJsonFiles(systemConfiguration))
            {
                std::cerr << "error setting json file\n";
                throw DBusInternalError();
            }
        });
}

// adds simple json types to interface's properties
void populateInterfaceFromJson(
    nlohmann::json& systemConfiguration, const std::string& jsonPointerPath,
    std::shared_ptr<sdbusplus::asio::dbus_interface>& iface,
    nlohmann::json& dict, sdbusplus::asio::object_server& objServer,
    sdbusplus::asio::PropertyPermission permission =
        sdbusplus::asio::PropertyPermission::readOnly)
{
    for (auto& dictPair : dict.items())
    {
        auto type = dictPair.value().type();
        bool array = false;
        if (dictPair.value().type() == nlohmann::json::value_t::array)
        {
            array = true;
            if (!dictPair.value().size())
            {
                continue;
            }
            type = dictPair.value()[0].type();
            bool isLegal = true;
            for (const auto& arrayItem : dictPair.value())
            {
                if (arrayItem.type() != type)
                {
                    isLegal = false;
                    break;
                }
            }
            if (!isLegal)
            {
                std::cerr << "dbus format error" << dictPair.value() << "\n";
                continue;
            }
        }
        if (type == nlohmann::json::value_t::object)
        {
            continue; // handled elsewhere
        }
        std::string key = jsonPointerPath + "/" + dictPair.key();
        if (permission == sdbusplus::asio::PropertyPermission::readWrite)
        {
            // all setable numbers are doubles as it is difficult to always
            // create a configuration file with all whole numbers as decimals
            // i.e. 1.0
            if (array)
            {
                if (dictPair.value()[0].is_number())
                {
                    type = nlohmann::json::value_t::number_float;
                }
            }
            else if (dictPair.value().is_number())
            {
                type = nlohmann::json::value_t::number_float;
            }
        }

        switch (type)
        {
            case (nlohmann::json::value_t::boolean):
            {
                if (array)
                {
                    // todo: array of bool isn't detected correctly by
                    // sdbusplus, change it to numbers
                    addArrayToDbus<uint64_t>(dictPair.key(), dictPair.value(),
                                             iface.get(), permission,
                                             systemConfiguration, key);
                }

                else
                {
                    addProperty(dictPair.key(), dictPair.value().get<bool>(),
                                iface.get(), systemConfiguration, key,
                                permission);
                }
                break;
            }
            case (nlohmann::json::value_t::number_integer):
            {
                if (array)
                {
                    addArrayToDbus<int64_t>(dictPair.key(), dictPair.value(),
                                            iface.get(), permission,
                                            systemConfiguration, key);
                }
                else
                {
                    addProperty(dictPair.key(), dictPair.value().get<int64_t>(),
                                iface.get(), systemConfiguration, key,
                                sdbusplus::asio::PropertyPermission::readOnly);
                }
                break;
            }
            case (nlohmann::json::value_t::number_unsigned):
            {
                if (array)
                {
                    addArrayToDbus<uint64_t>(dictPair.key(), dictPair.value(),
                                             iface.get(), permission,
                                             systemConfiguration, key);
                }
                else
                {
                    addProperty(dictPair.key(),
                                dictPair.value().get<uint64_t>(), iface.get(),
                                systemConfiguration, key,
                                sdbusplus::asio::PropertyPermission::readOnly);
                }
                break;
            }
            case (nlohmann::json::value_t::number_float):
            {
                if (array)
                {
                    addArrayToDbus<double>(dictPair.key(), dictPair.value(),
                                           iface.get(), permission,
                                           systemConfiguration, key);
                }

                else
                {
                    addProperty(dictPair.key(), dictPair.value().get<double>(),
                                iface.get(), systemConfiguration, key,
                                permission);
                }
                break;
            }
            case (nlohmann::json::value_t::string):
            {
                if (array)
                {
                    addArrayToDbus<std::string>(
                        dictPair.key(), dictPair.value(), iface.get(),
                        permission, systemConfiguration, key);
                }
                else
                {
                    addProperty(
                        dictPair.key(), dictPair.value().get<std::string>(),
                        iface.get(), systemConfiguration, key, permission);
                }
                break;
            }
            default:
            {
                std::cerr << "Unexpected json type in system configuration "
                          << dictPair.key() << ": "
                          << dictPair.value().type_name() << "\n";
                break;
            }
        }
    }
    if (permission == sdbusplus::asio::PropertyPermission::readWrite)
    {
        createDeleteObjectMethod(jsonPointerPath, iface, objServer,
                                 systemConfiguration);
    }
    iface->initialize();
}

sdbusplus::asio::PropertyPermission getPermission(const std::string& interface)
{
    return std::find(settableInterfaces.begin(), settableInterfaces.end(),
                     interface) != settableInterfaces.end()
               ? sdbusplus::asio::PropertyPermission::readWrite
               : sdbusplus::asio::PropertyPermission::readOnly;
}

void createAddObjectMethod(const std::string& jsonPointerPath,
                           const std::string& path,
                           nlohmann::json& systemConfiguration,
                           sdbusplus::asio::object_server& objServer,
                           const std::string& board)
{
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface = createInterface(
        objServer, path, "xyz.openbmc_project.AddObject", board);

    iface->register_method(
        "AddObject",
        [&systemConfiguration, &objServer,
         jsonPointerPath{std::string(jsonPointerPath)}, path{std::string(path)},
         board](const boost::container::flat_map<std::string, JsonVariantType>&
                    data) {
            nlohmann::json::json_pointer ptr(jsonPointerPath);
            nlohmann::json& base = systemConfiguration[ptr];
            auto findExposes = base.find("Exposes");

            if (findExposes == base.end())
            {
                throw std::invalid_argument("Entity must have children.");
            }

            // this will throw invalid-argument to sdbusplus if invalid json
            nlohmann::json newData{};
            for (const auto& item : data)
            {
                nlohmann::json& newJson = newData[item.first];
                std::visit([&newJson](auto&& val) { newJson = std::move(val); },
                           item.second);
            }

            auto findName = newData.find("Name");
            auto findType = newData.find("Type");
            if (findName == newData.end() || findType == newData.end())
            {
                throw std::invalid_argument("AddObject missing Name or Type");
            }
            const std::string* type = findType->get_ptr<const std::string*>();
            const std::string* name = findName->get_ptr<const std::string*>();
            if (type == nullptr || name == nullptr)
            {
                throw std::invalid_argument("Type and Name must be a string.");
            }

            bool foundNull = false;
            size_t lastIndex = 0;
            // we add in the "exposes"
            for (const auto& expose : *findExposes)
            {
                if (expose.is_null())
                {
                    foundNull = true;
                    continue;
                }

                if (expose["Name"] == *name && expose["Type"] == *type)
                {
                    throw std::invalid_argument(
                        "Field already in JSON, not adding");
                }

                if (foundNull)
                {
                    continue;
                }

                lastIndex++;
            }

            std::ifstream schemaFile(std::string(schemaDirectory) + "/" +
                                     *type + ".json");
            // todo(james) we might want to also make a list of 'can add'
            // interfaces but for now I think the assumption if there is a
            // schema avaliable that it is allowed to update is fine
            if (!schemaFile.good())
            {
                throw std::invalid_argument(
                    "No schema avaliable, cannot validate.");
            }
            nlohmann::json schema =
                nlohmann::json::parse(schemaFile, nullptr, false);
            if (schema.is_discarded())
            {
                std::cerr << "Schema not legal" << *type << ".json\n";
                throw DBusInternalError();
            }
            if (!validateJson(schema, newData))
            {
                throw std::invalid_argument("Data does not match schema");
            }
            if (foundNull)
            {
                findExposes->at(lastIndex) = newData;
            }
            else
            {
                findExposes->push_back(newData);
            }
            if (!writeJsonFiles(systemConfiguration))
            {
                std::cerr << "Error writing json files\n";
                throw DBusInternalError();
            }
            std::string dbusName = *name;

            std::regex_replace(dbusName.begin(), dbusName.begin(),
                               dbusName.end(), ILLEGAL_DBUS_MEMBER_REGEX, "_");

            std::shared_ptr<sdbusplus::asio::dbus_interface> interface =
                createInterface(objServer, path + "/" + dbusName,
                                "xyz.openbmc_project.Configuration." + *type,
                                board, true);
            // permission is read-write, as since we just created it, must be
            // runtime modifiable
            populateInterfaceFromJson(
                systemConfiguration,
                jsonPointerPath + "/Exposes/" + std::to_string(lastIndex),
                interface, newData, objServer,
                sdbusplus::asio::PropertyPermission::readWrite);
        });
    iface->initialize();
}

void postToDbus(const nlohmann::json& newConfiguration,
                nlohmann::json& systemConfiguration,
                sdbusplus::asio::object_server& objServer)

{
    // iterate through boards
    for (auto& boardPair : newConfiguration.items())
    {
        std::string boardKey = boardPair.value()["Name"];
        std::string boardKeyOrig = boardPair.value()["Name"];
        std::string jsonPointerPath = "/" + boardPair.key();
        // loop through newConfiguration, but use values from system
        // configuration to be able to modify via dbus later
        auto boardValues = systemConfiguration[boardPair.key()];
        auto findBoardType = boardValues.find("Type");
        std::string boardType;
        if (findBoardType != boardValues.end() &&
            findBoardType->type() == nlohmann::json::value_t::string)
        {
            boardType = findBoardType->get<std::string>();
            std::regex_replace(boardType.begin(), boardType.begin(),
                               boardType.end(), ILLEGAL_DBUS_MEMBER_REGEX, "_");
        }
        else
        {
            std::cerr << "Unable to find type for " << boardKey
                      << " reverting to Chassis.\n";
            boardType = "Chassis";
        }
        std::string boardtypeLower = boost::algorithm::to_lower_copy(boardType);

        std::regex_replace(boardKey.begin(), boardKey.begin(), boardKey.end(),
                           ILLEGAL_DBUS_MEMBER_REGEX, "_");
        std::string boardName = "/xyz/openbmc_project/inventory/system/" +
                                boardtypeLower + "/" + boardKey;

        std::shared_ptr<sdbusplus::asio::dbus_interface> inventoryIface =
            createInterface(objServer, boardName,
                            "xyz.openbmc_project.Inventory.Item", boardKey);

        std::shared_ptr<sdbusplus::asio::dbus_interface> boardIface =
            createInterface(objServer, boardName,
                            "xyz.openbmc_project.Inventory.Item." + boardType,
                            boardKeyOrig);

        createAddObjectMethod(jsonPointerPath, boardName, systemConfiguration,
                              objServer, boardKeyOrig);

        populateInterfaceFromJson(systemConfiguration, jsonPointerPath,
                                  boardIface, boardValues, objServer);
        jsonPointerPath += "/";
        // iterate through board properties
        for (auto& boardField : boardValues.items())
        {
            if (boardField.value().type() == nlohmann::json::value_t::object)
            {
                std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
                    createInterface(objServer, boardName, boardField.key(),
                                    boardKeyOrig);

                populateInterfaceFromJson(systemConfiguration,
                                          jsonPointerPath + boardField.key(),
                                          iface, boardField.value(), objServer);
            }
        }

        auto exposes = boardValues.find("Exposes");
        if (exposes == boardValues.end())
        {
            continue;
        }
        // iterate through exposes
        jsonPointerPath += "Exposes/";

        // store the board level pointer so we can modify it on the way down
        std::string jsonPointerPathBoard = jsonPointerPath;
        size_t exposesIndex = -1;
        for (auto& item : *exposes)
        {
            exposesIndex++;
            jsonPointerPath = jsonPointerPathBoard;
            jsonPointerPath += std::to_string(exposesIndex);

            auto findName = item.find("Name");
            if (findName == item.end())
            {
                std::cerr << "cannot find name in field " << item << "\n";
                continue;
            }
            auto findStatus = item.find("Status");
            // if status is not found it is assumed to be status = 'okay'
            if (findStatus != item.end())
            {
                if (*findStatus == "disabled")
                {
                    continue;
                }
            }
            auto findType = item.find("Type");
            std::string itemType;
            if (findType != item.end())
            {
                itemType = findType->get<std::string>();
                std::regex_replace(itemType.begin(), itemType.begin(),
                                   itemType.end(), ILLEGAL_DBUS_PATH_REGEX,
                                   "_");
            }
            else
            {
                itemType = "unknown";
            }
            std::string itemName = findName->get<std::string>();
            std::regex_replace(itemName.begin(), itemName.begin(),
                               itemName.end(), ILLEGAL_DBUS_MEMBER_REGEX, "_");

            std::shared_ptr<sdbusplus::asio::dbus_interface> itemIface =
                createInterface(objServer, boardName + "/" + itemName,
                                "xyz.openbmc_project.Configuration." + itemType,
                                boardKeyOrig);

            populateInterfaceFromJson(systemConfiguration, jsonPointerPath,
                                      itemIface, item, objServer,
                                      getPermission(itemType));

            for (auto& objectPair : item.items())
            {
                jsonPointerPath = jsonPointerPathBoard +
                                  std::to_string(exposesIndex) + "/" +
                                  objectPair.key();
                if (objectPair.value().type() ==
                    nlohmann::json::value_t::object)
                {
                    std::shared_ptr<sdbusplus::asio::dbus_interface>
                        objectIface = createInterface(
                            objServer, boardName + "/" + itemName,
                            "xyz.openbmc_project.Configuration." + itemType +
                                "." + objectPair.key(),
                            boardKeyOrig);

                    populateInterfaceFromJson(systemConfiguration,
                                              jsonPointerPath, objectIface,
                                              objectPair.value(), objServer,
                                              getPermission(objectPair.key()));
                }
                else if (objectPair.value().type() ==
                         nlohmann::json::value_t::array)
                {
                    size_t index = 0;
                    if (!objectPair.value().size())
                    {
                        continue;
                    }
                    bool isLegal = true;
                    auto type = objectPair.value()[0].type();
                    if (type != nlohmann::json::value_t::object)
                    {
                        continue;
                    }

                    // verify legal json
                    for (const auto& arrayItem : objectPair.value())
                    {
                        if (arrayItem.type() != type)
                        {
                            isLegal = false;
                            break;
                        }
                    }
                    if (!isLegal)
                    {
                        std::cerr << "dbus format error" << objectPair.value()
                                  << "\n";
                        break;
                    }

                    for (auto& arrayItem : objectPair.value())
                    {

                        std::shared_ptr<sdbusplus::asio::dbus_interface>
                            objectIface = createInterface(
                                objServer, boardName + "/" + itemName,
                                "xyz.openbmc_project.Configuration." +
                                    itemType + "." + objectPair.key() +
                                    std::to_string(index),
                                boardKeyOrig);

                        populateInterfaceFromJson(
                            systemConfiguration,
                            jsonPointerPath + "/" + std::to_string(index),
                            objectIface, arrayItem, objServer,
                            getPermission(objectPair.key()));
                        index++;
                    }
                }
            }
        }
    }
}

// reads json files out of the filesystem
bool findJsonFiles(std::list<nlohmann::json>& configurations)
{
    // find configuration files
    std::vector<std::filesystem::path> jsonPaths;
    if (!findFiles(std::filesystem::path(configurationDirectory), R"(.*\.json)",
                   jsonPaths))
    {
        std::cerr << "Unable to find any configuration files in "
                  << configurationDirectory << "\n";
        return false;
    }

    std::ifstream schemaStream(std::string(schemaDirectory) + "/" +
                               globalSchema);
    if (!schemaStream.good())
    {
        std::cerr
            << "Cannot open schema file,  cannot validate JSON, exiting\n\n";
        std::exit(EXIT_FAILURE);
        return false;
    }
    nlohmann::json schema = nlohmann::json::parse(schemaStream, nullptr, false);
    if (schema.is_discarded())
    {
        std::cerr
            << "Illegal schema file detected, cannot validate JSON, exiting\n";
        std::exit(EXIT_FAILURE);
        return false;
    }

    for (auto& jsonPath : jsonPaths)
    {
        std::ifstream jsonStream(jsonPath.c_str());
        if (!jsonStream.good())
        {
            std::cerr << "unable to open " << jsonPath.string() << "\n";
            continue;
        }
        auto data = nlohmann::json::parse(jsonStream, nullptr, false);
        if (data.is_discarded())
        {
            std::cerr << "syntax error in " << jsonPath.string() << "\n";
            continue;
        }
        /*
         * todo(james): reenable this once less things are in flight
         *
        if (!validateJson(schema, data))
        {
            std::cerr << "Error validating " << jsonPath.string() << "\n";
            continue;
        }
        */

        if (data.type() == nlohmann::json::value_t::array)
        {
            for (auto& d : data)
            {
                configurations.emplace_back(d);
            }
        }
        else
        {
            configurations.emplace_back(data);
        }
    }
    return true;
}

std::string getRecordName(
    const boost::container::flat_map<std::string, BasicVariantType>& probe,
    const std::string& probeName)
{
    if (probe.empty())
    {
        return probeName;
    }

    // use an array so alphabetical order from the
    // flat_map is maintained
    auto device = nlohmann::json::array();
    for (auto& devPair : probe)
    {
        device.push_back(devPair.first);
        std::visit([&device](auto&& v) { device.push_back(v); },
                   devPair.second);
    }
    size_t hash = std::hash<std::string>{}(probeName + device.dump());
    // hashes are hard to distinguish, use the
    // non-hashed version if we want debug
    if constexpr (DEBUG)
    {
        return probeName + device.dump();
    }
    else
    {
        return std::to_string(hash);
    }
}

PerformScan::PerformScan(nlohmann::json& systemConfiguration,
                         nlohmann::json& missingConfigurations,
                         std::list<nlohmann::json>& configurations,
                         sdbusplus::asio::object_server& objServerIn,
                         std::function<void()>&& callback) :
    _systemConfiguration(systemConfiguration),
    _missingConfigurations(missingConfigurations),
    _configurations(configurations), objServer(objServerIn),
    _callback(std::move(callback))
{}
void PerformScan::run()
{
    boost::container::flat_set<std::string> dbusProbeInterfaces;
    std::vector<std::shared_ptr<PerformProbe>> dbusProbePointers;

    for (auto it = _configurations.begin(); it != _configurations.end();)
    {
        auto findProbe = it->find("Probe");
        auto findName = it->find("Name");

        nlohmann::json probeCommand;
        // check for poorly formatted fields, probe must be an array
        if (findProbe == it->end())
        {
            std::cerr << "configuration file missing probe:\n " << *it << "\n";
            it = _configurations.erase(it);
            continue;
        }
        else if ((*findProbe).type() != nlohmann::json::value_t::array)
        {
            probeCommand = nlohmann::json::array();
            probeCommand.push_back(*findProbe);
        }
        else
        {
            probeCommand = *findProbe;
        }

        if (findName == it->end())
        {
            std::cerr << "configuration file missing name:\n " << *it << "\n";
            it = _configurations.erase(it);
            continue;
        }
        std::string probeName = *findName;

        if (std::find(passedProbes.begin(), passedProbes.end(), probeName) !=
            passedProbes.end())
        {
            it = _configurations.erase(it);
            continue;
        }
        nlohmann::json* recordPtr = &(*it);

        // store reference to this to children to makes sure we don't get
        // destroyed too early
        auto thisRef = shared_from_this();
        auto probePointer = std::make_shared<PerformProbe>(
            probeCommand, thisRef,
            [&, recordPtr, probeName](FoundDeviceT& foundDevices) {
                _passed = true;

                std::set<nlohmann::json> usedNames;
                passedProbes.push_back(probeName);
                std::list<size_t> indexes(foundDevices.size());
                std::iota(indexes.begin(), indexes.end(), 1);

                size_t indexIdx = probeName.find("$");
                bool hasTemplateName = (indexIdx != std::string::npos);

                // copy over persisted configurations and make sure we remove
                // indexes that are already used
                for (auto itr = foundDevices.begin();
                     itr != foundDevices.end();)
                {
                    std::string recordName = getRecordName(*itr, probeName);

                    auto fromLastJson = lastJson.find(recordName);
                    if (fromLastJson != lastJson.end())
                    {
                        auto findExposes = fromLastJson->find("Exposes");
                        // delete nulls from any updates
                        if (findExposes != fromLastJson->end())
                        {
                            auto copy = nlohmann::json::array();
                            for (auto& expose : *findExposes)
                            {
                                if (expose.is_null())
                                {
                                    continue;
                                }
                                copy.emplace_back(expose);
                            }
                            *findExposes = copy;
                        }

                        // keep user changes
                        _systemConfiguration[recordName] = *fromLastJson;
                        _missingConfigurations.erase(recordName);
                        itr = foundDevices.erase(itr);
                        if (hasTemplateName)
                        {
                            auto nameIt = fromLastJson->find("Name");
                            if (nameIt == fromLastJson->end())
                            {
                                std::cerr << "Last JSON Illegal\n";
                                continue;
                            }

                            int index = std::stoi(
                                nameIt->get<std::string>().substr(indexIdx),
                                nullptr, 0);
                            usedNames.insert(nameIt.value());
                            auto usedIt = std::find(indexes.begin(),
                                                    indexes.end(), index);

                            if (usedIt == indexes.end())
                            {
                                continue; // less items now
                            }
                            indexes.erase(usedIt);
                        }

                        continue;
                    }
                    itr++;
                }

                std::optional<std::string> replaceStr;

                for (auto& foundDevice : foundDevices)
                {
                    nlohmann::json record = *recordPtr;
                    std::string recordName =
                        getRecordName(foundDevice, probeName);
                    size_t foundDeviceIdx = indexes.front();
                    indexes.pop_front();

                    // check name first so we have no duplicate names
                    auto getName = record.find("Name");
                    if (getName == record.end())
                    {
                        std::cerr << "Record Missing Name! " << record.dump();
                        continue; // this should be impossible at this level
                    }

                    nlohmann::json copyForName = {{"Name", getName.value()}};
                    nlohmann::json::iterator copyIt = copyForName.begin();
                    std::optional<std::string> replaceVal = templateCharReplace(
                        copyIt, foundDevice, foundDeviceIdx, replaceStr);

                    if (!replaceStr && replaceVal)
                    {
                        if (usedNames.find(copyIt.value()) != usedNames.end())
                        {
                            replaceStr = replaceVal;
                            copyForName = {{"Name", getName.value()}};
                            copyIt = copyForName.begin();
                            templateCharReplace(copyIt, foundDevice,
                                                foundDeviceIdx, replaceStr);
                        }
                    }

                    if (replaceStr)
                    {
                        std::cerr << "Duplicates found, replacing "
                                  << *replaceStr
                                  << " with found device index.\n Consider "
                                     "fixing template to not have duplicates\n";
                    }

                    for (auto keyPair = record.begin(); keyPair != record.end();
                         keyPair++)
                    {
                        if (keyPair.key() == "Name")
                        {
                            keyPair.value() = copyIt.value();
                            usedNames.insert(copyIt.value());

                            continue; // already covered above
                        }
                        templateCharReplace(keyPair, foundDevice,
                                            foundDeviceIdx, replaceStr);
                    }

                    // insert into configuration temporarily to be able to
                    // reference ourselves

                    _systemConfiguration[recordName] = record;

                    auto findExpose = record.find("Exposes");
                    if (findExpose == record.end())
                    {
                        _systemConfiguration[recordName] = record;
                        continue;
                    }

                    for (auto& expose : *findExpose)
                    {
                        for (auto keyPair = expose.begin();
                             keyPair != expose.end(); keyPair++)
                        {

                            templateCharReplace(keyPair, foundDevice,
                                                foundDeviceIdx, replaceStr);

                            bool isBind =
                                boost::starts_with(keyPair.key(), "Bind");
                            bool isDisable = keyPair.key() == "DisableNode";

                            // special cases
                            if (!(isBind || isDisable))
                            {
                                continue;
                            }

                            if (keyPair.value().type() !=
                                    nlohmann::json::value_t::string &&
                                keyPair.value().type() !=
                                    nlohmann::json::value_t::array)
                            {
                                std::cerr << "Value is invalid type "
                                          << keyPair.key() << "\n";
                                continue;
                            }

                            std::vector<std::string> matches;
                            if (keyPair.value().type() ==
                                nlohmann::json::value_t::string)
                            {
                                matches.emplace_back(keyPair.value());
                            }
                            else
                            {
                                for (const auto& value : keyPair.value())
                                {
                                    if (value.type() !=
                                        nlohmann::json::value_t::string)
                                    {
                                        std::cerr << "Value is invalid type "
                                                  << value << "\n";
                                        break;
                                    }
                                    matches.emplace_back(value);
                                }
                            }

                            std::set<std::string> foundMatches;
                            for (auto& configurationPair :
                                 _systemConfiguration.items())
                            {
                                if (isDisable)
                                {
                                    // don't disable ourselves
                                    if (configurationPair.key() == recordName)
                                    {
                                        continue;
                                    }
                                }
                                auto configListFind =
                                    configurationPair.value().find("Exposes");

                                if (configListFind ==
                                        configurationPair.value().end() ||
                                    configListFind->type() !=
                                        nlohmann::json::value_t::array)
                                {
                                    continue;
                                }
                                for (auto& exposedObject : *configListFind)
                                {
                                    auto matchIt = std::find_if(
                                        matches.begin(), matches.end(),
                                        [name = (exposedObject)["Name"]
                                                    .get<std::string>()](
                                            const std::string& s) {
                                            return s == name;
                                        });
                                    if (matchIt == matches.end())
                                    {
                                        continue;
                                    }
                                    foundMatches.insert(*matchIt);

                                    if (isBind)
                                    {
                                        std::string bind = keyPair.key().substr(
                                            sizeof("Bind") - 1);

                                        exposedObject["Status"] = "okay";
                                        expose[bind] = exposedObject;
                                    }
                                    else if (isDisable)
                                    {
                                        exposedObject["Status"] = "disabled";
                                    }
                                }
                            }
                            if (foundMatches.size() != matches.size())
                            {
                                std::cerr << "configuration file "
                                             "dependency error, "
                                             "could not find "
                                          << keyPair.key() << " "
                                          << keyPair.value() << "\n";
                            }
                        }
                    }
                    // overwrite ourselves with cleaned up version
                    _systemConfiguration[recordName] = record;
                    _missingConfigurations.erase(recordName);
                }
            });

        // parse out dbus probes by discarding other probe types, store in a
        // map
        for (const std::string& probe : probeCommand)
        {
            bool found = false;
            boost::container::flat_map<const char*, probe_type_codes,
                                       cmp_str>::const_iterator probeType;
            for (probeType = PROBE_TYPES.begin();
                 probeType != PROBE_TYPES.end(); ++probeType)
            {
                if (probe.find(probeType->first) != std::string::npos)
                {
                    found = true;
                    break;
                }
            }
            if (found)
            {
                continue;
            }
            // syntax requires probe before first open brace
            auto findStart = probe.find("(");
            std::string interface = probe.substr(0, findStart);
            dbusProbeInterfaces.emplace(interface);
            dbusProbePointers.emplace_back(probePointer);
        }
        it++;
    }

    for (const std::string& interface : dbusProbeInterfaces)
    {
        registerCallback(_systemConfiguration, objServer, interface);
    }

    // probe vector stores a shared_ptr to each PerformProbe that cares
    // about a dbus interface
    findDbusObjects(std::move(dbusProbePointers),
                    std::move(dbusProbeInterfaces), shared_from_this());
    if constexpr (DEBUG)
    {
        std::cerr << __func__ << " " << __LINE__ << "\n";
    }
}

PerformScan::~PerformScan()
{
    if (_passed)
    {
        auto nextScan = std::make_shared<PerformScan>(
            _systemConfiguration, _missingConfigurations, _configurations,
            objServer, std::move(_callback));
        nextScan->passedProbes = std::move(passedProbes);
        nextScan->dbusProbeObjects = std::move(dbusProbeObjects);
        nextScan->run();

        if constexpr (DEBUG)
        {
            std::cerr << __func__ << " " << __LINE__ << "\n";
        }
    }
    else
    {
        _callback();

        if constexpr (DEBUG)
        {
            std::cerr << __func__ << " " << __LINE__ << "\n";
        }
    }
}

void startRemovedTimer(boost::asio::steady_timer& timer,
                       nlohmann::json& systemConfiguration)
{
    static bool scannedPowerOff = false;
    static bool scannedPowerOn = false;

    if (systemConfiguration.empty() || lastJson.empty())
    {
        return; // not ready yet
    }
    if (scannedPowerOn)
    {
        return;
    }

    if (!isPowerOn() && scannedPowerOff)
    {
        return;
    }

    timer.expires_after(std::chrono::seconds(10));
    timer.async_wait(
        [&systemConfiguration](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                // we were cancelled
                return;
            }

            bool powerOff = !isPowerOn();
            for (const auto& item : lastJson.items())
            {
                if (systemConfiguration.find(item.key()) ==
                    systemConfiguration.end())
                {
                    bool isDetectedPowerOn = false;
                    auto powerState = item.value().find("PowerState");
                    if (powerState != item.value().end())
                    {
                        auto ptr = powerState->get_ptr<const std::string*>();
                        if (ptr)
                        {
                            if (*ptr == "On" || *ptr == "BiosPost")
                            {
                                isDetectedPowerOn = true;
                            }
                        }
                    }
                    if (powerOff && isDetectedPowerOn)
                    {
                        // power not on yet, don't know if it's there or not
                        continue;
                    }
                    if (!powerOff && scannedPowerOff && isDetectedPowerOn)
                    {
                        // already logged it when power was off
                        continue;
                    }

                    logDeviceRemoved(item.value());
                }
            }
            scannedPowerOff = true;
            if (!powerOff)
            {
                scannedPowerOn = true;
            }
        });
}

// main properties changed entry
void propertiesChangedCallback(nlohmann::json& systemConfiguration,
                               sdbusplus::asio::object_server& objServer)
{
    static bool inProgress = false;
    static boost::asio::steady_timer timer(io);
    static size_t instance = 0;
    instance++;
    size_t count = instance;

    timer.expires_after(std::chrono::seconds(5));

    // setup an async wait as we normally get flooded with new requests
    timer.async_wait([&systemConfiguration, &objServer,
                      count](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            // we were cancelled
            return;
        }
        else if (ec)
        {
            std::cerr << "async wait error " << ec << "\n";
            return;
        }

        if (inProgress)
        {
            propertiesChangedCallback(systemConfiguration, objServer);
            return;
        }
        inProgress = true;

        nlohmann::json oldConfiguration = systemConfiguration;
        auto missingConfigurations = std::make_shared<nlohmann::json>();
        *missingConfigurations = systemConfiguration;

        std::list<nlohmann::json> configurations;
        if (!findJsonFiles(configurations))
        {
            std::cerr << "cannot find json files\n";
            inProgress = false;
            return;
        }

        auto perfScan = std::make_shared<PerformScan>(
            systemConfiguration, *missingConfigurations, configurations,
            objServer,
            [&systemConfiguration, &objServer, count, oldConfiguration,
             missingConfigurations]() {
                // this is something that since ac has been applied to the bmc
                // we saw, and we no longer see it
                bool powerOff = !isPowerOn();
                for (const auto& item : missingConfigurations->items())
                {
                    bool isDetectedPowerOn = false;
                    auto powerState = item.value().find("PowerState");
                    if (powerState != item.value().end())
                    {
                        auto ptr = powerState->get_ptr<const std::string*>();
                        if (ptr)
                        {
                            if (*ptr == "On" || *ptr == "BiosPost")
                            {
                                isDetectedPowerOn = true;
                            }
                        }
                    }
                    if (powerOff && isDetectedPowerOn)
                    {
                        // power not on yet, don't know if it's there or not
                        continue;
                    }
                    std::string name = item.value()["Name"].get<std::string>();
                    std::vector<std::weak_ptr<sdbusplus::asio::dbus_interface>>&
                        ifaces = inventory[name];
                    for (auto& iface : ifaces)
                    {
                        auto sharedPtr = iface.lock();
                        if (!sharedPtr)
                        {
                            continue; // was already deleted elsewhere
                        }
                        objServer.remove_interface(sharedPtr);
                    }
                    ifaces.clear();
                    systemConfiguration.erase(item.key());
                    logDeviceRemoved(item.value());
                }

                nlohmann::json newConfiguration = systemConfiguration;
                for (auto it = newConfiguration.begin();
                     it != newConfiguration.end();)
                {
                    auto findKey = oldConfiguration.find(it.key());
                    if (findKey != oldConfiguration.end())
                    {
                        it = newConfiguration.erase(it);
                    }
                    else
                    {
                        it++;
                    }
                }
                for (const auto& item : newConfiguration.items())
                {
                    logDeviceAdded(item.value());
                }

                inProgress = false;

                io.post([&, newConfiguration]() {
                    loadOverlays(newConfiguration);

                    io.post([&]() {
                        if (!writeJsonFiles(systemConfiguration))
                        {
                            std::cerr << "Error writing json files\n";
                        }
                    });
                    io.post([&, newConfiguration]() {
                        postToDbus(newConfiguration, systemConfiguration,
                                   objServer);
                        if (count != instance)
                        {
                            return;
                        }
                        startRemovedTimer(timer, systemConfiguration);
                    });
                });
            });
        perfScan->run();
    });
}

void registerCallback(nlohmann::json& systemConfiguration,
                      sdbusplus::asio::object_server& objServer,
                      const std::string& interface)
{
    static boost::container::flat_map<std::string, sdbusplus::bus::match::match>
        dbusMatches;

    auto find = dbusMatches.find(interface);
    if (find != dbusMatches.end())
    {
        return;
    }
    std::function<void(sdbusplus::message::message & message)> eventHandler =

        [&](sdbusplus::message::message&) {
            propertiesChangedCallback(systemConfiguration, objServer);
        };

    sdbusplus::bus::match::match match(
        static_cast<sdbusplus::bus::bus&>(*SYSTEM_BUS),
        "type='signal',member='PropertiesChanged',arg0='" + interface + "'",
        eventHandler);
    dbusMatches.emplace(interface, std::move(match));
}

int main()
{
    // setup connection to dbus
    SYSTEM_BUS = std::make_shared<sdbusplus::asio::connection>(io);
    SYSTEM_BUS->request_name("xyz.openbmc_project.EntityManager");

    sdbusplus::asio::object_server objServer(SYSTEM_BUS);

    std::shared_ptr<sdbusplus::asio::dbus_interface> entityIface =
        objServer.add_interface("/xyz/openbmc_project/EntityManager",
                                "xyz.openbmc_project.EntityManager");

    // to keep reference to the match / filter objects so they don't get
    // destroyed

    nlohmann::json systemConfiguration = nlohmann::json::object();

    io.post(
        [&]() { propertiesChangedCallback(systemConfiguration, objServer); });

    entityIface->register_method("ReScan", [&]() {
        propertiesChangedCallback(systemConfiguration, objServer);
    });
    entityIface->initialize();

    if (fwVersionIsSame())
    {
        if (std::filesystem::is_regular_file(currentConfiguration))
        {
            // this file could just be deleted, but it's nice for debug
            std::filesystem::create_directory(tempConfigDir);
            std::filesystem::remove(lastConfiguration);
            std::filesystem::copy(currentConfiguration, lastConfiguration);
            std::filesystem::remove(currentConfiguration);

            std::ifstream jsonStream(lastConfiguration);
            if (jsonStream.good())
            {
                auto data = nlohmann::json::parse(jsonStream, nullptr, false);
                if (data.is_discarded())
                {
                    std::cerr << "syntax error in " << lastConfiguration
                              << "\n";
                }
                else
                {
                    lastJson = std::move(data);
                }
            }
            else
            {
                std::cerr << "unable to open " << lastConfiguration << "\n";
            }
        }
    }
    else
    {
        // not an error, just logging at this level to make it in the journal
        std::cerr << "Clearing previous configuration\n";
        std::filesystem::remove(currentConfiguration);
    }

    // some boards only show up after power is on, we want to not say they are
    // removed until the same state happens
    setupPowerMatch(SYSTEM_BUS);

    io.run();

    return 0;
}
