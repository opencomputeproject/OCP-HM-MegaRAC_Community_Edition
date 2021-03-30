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

#include "Utils.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace fs = std::filesystem;

static bool powerStatusOn = false;
static bool biosHasPost = false;

static std::unique_ptr<sdbusplus::bus::match::match> powerMatch = nullptr;
static std::unique_ptr<sdbusplus::bus::match::match> postMatch = nullptr;

bool getSensorConfiguration(
    const std::string& type,
    const std::shared_ptr<sdbusplus::asio::connection>& dbusConnection,
    ManagedObjectType& resp, bool useCache)
{
    static ManagedObjectType managedObj;

    if (!useCache)
    {
        managedObj.clear();
        sdbusplus::message::message getManagedObjects =
            dbusConnection->new_method_call(
                entityManagerName, "/", "org.freedesktop.DBus.ObjectManager",
                "GetManagedObjects");
        bool err = false;
        try
        {
            sdbusplus::message::message reply =
                dbusConnection->call(getManagedObjects);
            reply.read(managedObj);
        }
        catch (const sdbusplus::exception::exception& e)
        {
            std::cerr << "While calling GetManagedObjects on service:"
                      << entityManagerName << " exception name:" << e.name()
                      << "and description:" << e.description()
                      << " was thrown\n";
            err = true;
        }

        if (err)
        {
            std::cerr << "Error communicating to entity manager\n";
            return false;
        }
    }
    for (const auto& pathPair : managedObj)
    {
        bool correctType = false;
        for (const auto& entry : pathPair.second)
        {
            if (boost::starts_with(entry.first, type))
            {
                correctType = true;
                break;
            }
        }
        if (correctType)
        {
            resp.emplace(pathPair);
        }
    }
    return true;
}

bool findFiles(const fs::path dirPath, const std::string& matchString,
               std::vector<fs::path>& foundPaths, unsigned int symlinkDepth)
{
    if (!fs::exists(dirPath))
        return false;

    std::regex search(matchString);
    std::smatch match;
    for (auto& p : fs::recursive_directory_iterator(dirPath))
    {
        std::string path = p.path().string();
        if (!is_directory(p))
        {
            if (std::regex_search(path, match, search))
                foundPaths.emplace_back(p.path());
        }
        else if (is_symlink(p) && symlinkDepth)
        {
            findFiles(p.path(), matchString, foundPaths, symlinkDepth - 1);
        }
    }
    return true;
}

bool isPowerOn(void)
{
    if (!powerMatch)
    {
        throw std::runtime_error("Power Match Not Created");
    }
    return powerStatusOn;
}

bool hasBiosPost(void)
{
    if (!postMatch)
    {
        throw std::runtime_error("Post Match Not Created");
    }
    return biosHasPost;
}

void setupPowerMatch(const std::shared_ptr<sdbusplus::asio::connection>& conn)
{
    static boost::asio::steady_timer timer(conn->get_io_context());
    // create a match for powergood changes, first time do a method call to
    // cache the correct value
    if (powerMatch)
    {
        return;
    }

    powerMatch = std::make_unique<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus&>(*conn),
        "type='signal',interface='" + std::string(properties::interface) +
            "',path='" + std::string(power::path) + "',arg0='" +
            std::string(power::interface) + "'",
        [](sdbusplus::message::message& message) {
            std::string objectName;
            boost::container::flat_map<std::string, std::variant<std::string>>
                values;
            message.read(objectName, values);
            auto findState = values.find(power::property);
            if (findState != values.end())
            {
                bool on = boost::ends_with(
                    std::get<std::string>(findState->second), "Running");
                if (!on)
                {
                    timer.cancel();
                    powerStatusOn = false;
                    return;
                }
                // on comes too quickly
                timer.expires_after(std::chrono::seconds(10));
                timer.async_wait([](boost::system::error_code ec) {
                    if (ec == boost::asio::error::operation_aborted)
                    {
                        return;
                    }
                    else if (ec)
                    {
                        std::cerr << "Timer error " << ec.message() << "\n";
                        return;
                    }
                    powerStatusOn = true;
                });
            }
        });

    postMatch = std::make_unique<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus&>(*conn),
        "type='signal',interface='" + std::string(properties::interface) +
            "',path='" + std::string(post::path) + "',arg0='" +
            std::string(post::interface) + "'",
        [](sdbusplus::message::message& message) {
            std::string objectName;
            boost::container::flat_map<std::string, std::variant<std::string>>
                values;
            message.read(objectName, values);
            auto findState = values.find(post::property);
            if (findState != values.end())
            {
                biosHasPost =
                    std::get<std::string>(findState->second) != "Inactive";
            }
        });

    conn->async_method_call(
        [](boost::system::error_code ec,
           const std::variant<std::string>& state) {
            if (ec)
            {
                // we commonly come up before power control, we'll capture the
                // property change later
                return;
            }
            powerStatusOn =
                boost::ends_with(std::get<std::string>(state), "Running");
        },
        power::busname, power::path, properties::interface, properties::get,
        power::interface, power::property);

    conn->async_method_call(
        [](boost::system::error_code ec,
           const std::variant<std::string>& state) {
            if (ec)
            {
                // we commonly come up before power control, we'll capture the
                // property change later
                return;
            }
            biosHasPost = std::get<std::string>(state) != "Inactive";
        },
        post::busname, post::path, properties::interface, properties::get,
        post::interface, post::property);
}

// replaces limits if MinReading and MaxReading are found.
void findLimits(std::pair<double, double>& limits,
                const SensorBaseConfiguration* data)
{
    if (!data)
    {
        return;
    }
    auto maxFind = data->second.find("MaxReading");
    auto minFind = data->second.find("MinReading");

    if (minFind != data->second.end())
    {
        limits.first = std::visit(VariantToDoubleVisitor(), minFind->second);
    }
    if (maxFind != data->second.end())
    {
        limits.second = std::visit(VariantToDoubleVisitor(), maxFind->second);
    }
}

void createAssociation(
    std::shared_ptr<sdbusplus::asio::dbus_interface>& association,
    const std::string& path)
{
    if (association)
    {
        std::filesystem::path p(path);

        std::vector<Association> associations;
        associations.push_back(
            Association("chassis", "all_sensors", p.parent_path().string()));
        association->register_property("Associations", associations);
        association->initialize();
    }
}

void setInventoryAssociation(
    std::shared_ptr<sdbusplus::asio::dbus_interface> association,
    const std::string& path,
    const std::vector<std::string>& chassisPaths = std::vector<std::string>())
{
    if (association)
    {
        std::filesystem::path p(path);
        std::vector<Association> associations;
        std::string objPath(p.parent_path().string());

        associations.push_back(Association("inventory", "sensors", objPath));
        associations.push_back(Association("chassis", "all_sensors", objPath));

        for (const std::string& chassisPath : chassisPaths)
        {
            associations.push_back(
                Association("chassis", "all_sensors", chassisPath));
        }

        association->register_property("Associations", associations);
        association->initialize();
    }
}

void createInventoryAssoc(
    std::shared_ptr<sdbusplus::asio::connection> conn,
    std::shared_ptr<sdbusplus::asio::dbus_interface> association,
    const std::string& path)
{
    if (!association)
    {
        return;
    }

    conn->async_method_call(
        [association, path](const boost::system::error_code ec,
                            const std::vector<std::string>& invSysObjPaths) {
            if (ec)
            {
                // In case of error, set the default associations and
                // initialize the association Interface.
                setInventoryAssociation(association, path);
                return;
            }
            setInventoryAssociation(association, path, invSysObjPaths);
        },
        mapper::busName, mapper::path, mapper::interface, "GetSubTreePaths",
        "/xyz/openbmc_project/inventory/system", 2,
        std::array<std::string, 1>{
            "xyz.openbmc_project.Inventory.Item.System"});
}

std::optional<double> readFile(const std::string& thresholdFile,
                               const double& scaleFactor)
{
    std::string line;
    std::ifstream labelFile(thresholdFile);
    if (labelFile.good())
    {
        std::getline(labelFile, line);
        labelFile.close();

        try
        {
            return std::stod(line) / scaleFactor;
        }
        catch (const std::invalid_argument&)
        {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<std::tuple<std::string, std::string, std::string>>
    splitFileName(const std::filesystem::path& filePath)
{
    if (filePath.has_filename())
    {
        const auto fileName = filePath.filename().string();

        size_t numberPos = std::strcspn(fileName.c_str(), "1234567890");
        size_t itemPos = std::strcspn(fileName.c_str(), "_");

        if (numberPos > 0 && itemPos > numberPos && fileName.size() > itemPos)
        {
            return std::make_optional(
                std::make_tuple(fileName.substr(0, numberPos),
                                fileName.substr(numberPos, itemPos - numberPos),
                                fileName.substr(itemPos + 1, fileName.size())));
        }
    }
    return std::nullopt;
}
