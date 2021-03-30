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
#pragma once

#include "sdbusplus.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/source/signal.hpp>

#include <filesystem>
#include <fstream>

namespace phosphor::fan
{

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace phosphor::logging;

constexpr auto confOverridePath = "/etc/phosphor-fan-presence";
constexpr auto confBasePath = "/usr/share/phosphor-fan-presence";
constexpr auto confDbusPath = "/xyz/openbmc_project/inventory/system/chassis";
constexpr auto confDbusIntf =
    "xyz.openbmc_project.Inventory.Decorator.Compatible";
constexpr auto confDbusProp = "Names";

class JsonConfig
{
  public:
    /**
     * Get the json configuration file. The first location found to contain
     * the json config file for the given fan application is used from the
     * following locations in order.
     * 1.) From the confOverridePath location
     * 2.) From config file found using a property value as a relative
     * path extension on the base path from the dbus object where:
     *     path = Path set in confDbusPath
     *     interface = Interface set in confDbusIntf
     *     property = Property set in confDbusProp
     * 3.) *DEFAULT* - From the confBasePath location
     *
     * @brief Get the configuration file to be used
     *
     * @param[in] bus - The dbus bus object
     * @param[in] appName - The phosphor-fan-presence application name
     * @param[in] fileName - Application's configuration file's name
     *
     * @return filesystem path
     *     The filesystem path to the configuration file to use
     */
    static const fs::path getConfFile(sdbusplus::bus::bus& bus,
                                      const std::string& appName,
                                      const std::string& fileName)
    {
        // Check override location
        fs::path confFile = fs::path{confOverridePath} / appName / fileName;
        if (fs::exists(confFile))
        {
            return confFile;
        }

        try
        {
            // Retrieve json config relative path location from dbus
            auto confDbusValue =
                util::SDBusPlus::getProperty<std::vector<std::string>>(
                    bus, confDbusPath, confDbusIntf, confDbusProp);
            // Look for a config file at each entry relative to the base
            // path and use the first one found
            auto it = std::find_if(
                confDbusValue.begin(), confDbusValue.end(),
                [&confFile, &appName, &fileName](auto const& entry) {
                    confFile =
                        fs::path{confBasePath} / appName / entry / fileName;
                    return fs::exists(confFile);
                });
            if (it == confDbusValue.end())
            {
                // Property exists, but no config file found. Use default base
                // path
                confFile = fs::path{confBasePath} / appName / fileName;
            }
        }
        catch (const util::DBusError&)
        {
            // Property unavailable, attempt default base path
            confFile = fs::path{confBasePath} / appName / fileName;
        }

        if (!fs::exists(confFile))
        {
            log<level::ERR>("No JSON config file found",
                            entry("DEFAULT_FILE=%s", confFile.c_str()));
            throw std::runtime_error("No JSON config file found");
        }

        return confFile;
    }

    /**
     * @brief Load the JSON config file
     *
     * @param[in] confFile - File system path of the configuration file to load
     *
     * @return Parsed JSON object
     *     The parsed JSON configuration file object
     */
    static const json load(const fs::path& confFile)
    {
        std::ifstream file;
        json jsonConf;

        if (fs::exists(confFile))
        {
            log<level::INFO>("Loading configuration",
                             entry("JSON_FILE=%s", confFile.c_str()));
            file.open(confFile);
            try
            {
                jsonConf = json::parse(file);
            }
            catch (std::exception& e)
            {
                log<level::ERR>("Failed to parse JSON config file",
                                entry("JSON_FILE=%s", confFile.c_str()),
                                entry("JSON_ERROR=%s", e.what()));
                throw std::runtime_error("Failed to parse JSON config file");
            }
        }
        else
        {
            log<level::ERR>("Unable to open JSON config file",
                            entry("JSON_FILE=%s", confFile.c_str()));
            throw std::runtime_error("Unable to open JSON config file");
        }

        return jsonConf;
    }
};

} // namespace phosphor::fan
