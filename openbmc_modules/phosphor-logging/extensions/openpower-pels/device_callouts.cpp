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
#include "device_callouts.hpp"

#include "paths.hpp"

#include <fstream>
#include <phosphor-logging/log.hpp>
#include <regex>

namespace openpower::pels::device_callouts
{

constexpr auto debugFilePath = "/etc/phosphor-logging/";
constexpr auto calloutFileSuffix = "_dev_callouts.json";

namespace fs = std::filesystem;
using namespace phosphor::logging;

namespace util
{

fs::path getJSONFilename(const std::vector<std::string>& compatibleList)
{
    auto basePath = getPELReadOnlyDataPath();
    fs::path fullPath;

    // Find an entry in the list of compatible system names that
    // matches a filename we have.

    for (const auto& name : compatibleList)
    {
        fs::path filename = name + calloutFileSuffix;

        // Check the debug path first
        fs::path path{fs::path{debugFilePath} / filename};

        if (fs::exists(path))
        {
            log<level::INFO>("Found device callout debug file");
            fullPath = path;
            break;
        }

        path = basePath / filename;

        if (fs::exists(path))
        {
            fullPath = path;
            break;
        }
    }

    if (fullPath.empty())
    {
        throw std::invalid_argument(
            "No JSON dev path callout file for this system");
    }

    return fullPath;
}

/**
 * @brief Reads the callout JSON into an object based on the
 *        compatible system names list.
 *
 * @param[in] compatibleList - The list of compatible names for this
 *                             system.
 *
 * @return nlohmann::json - The JSON object
 */
nlohmann::json loadJSON(const std::vector<std::string>& compatibleList)
{
    auto filename = getJSONFilename(compatibleList);
    std::ifstream file{filename};
    return nlohmann::json::parse(file);
}

std::tuple<size_t, uint8_t> getI2CSearchKeys(const std::string& devPath)
{
    std::smatch match;

    // Look for i2c-A/A-00BB
    // where A = bus number and BB = address
    std::regex regex{"i2c-[0-9]+/([0-9]+)-00([0-9a-f]{2})"};

    regex_search(devPath, match, regex);

    if (match.size() != 3)
    {
        std::string msg = "Could not get I2C bus and address from " + devPath;
        throw std::invalid_argument{msg.c_str()};
    }

    size_t bus = std::stoul(match[1].str(), nullptr, 0);

    // An I2C bus on a CFAM has everything greater than the 10s digit
    // as the CFAM number, so strip it off.  Like:
    //    112 = cfam1 bus 12
    //    1001 = cfam10 bus 1
    bus = bus % 100;

    uint8_t address = std::stoul(match[2].str(), nullptr, 16);

    return {bus, address};
}

std::string getFSISearchKeys(const std::string& devPath)
{
    std::string links;
    std::smatch match;
    auto search = devPath;

    // Look for slave@XX:
    // where XX = link number in hex
    std::regex regex{"slave@([0-9a-f]{2}):"};

    // Find all links in the path and separate them with hyphens.
    while (regex_search(search, match, regex))
    {
        // Convert to an int first to handle a hex number like "0a"
        // though in reality there won't be more than links 0 - 9.
        auto linkNum = std::stoul(match[1].str(), nullptr, 16);
        links += std::to_string(linkNum) + '-';

        search = match.suffix();
    }

    if (links.empty())
    {
        std::string msg = "Could not get FSI links from " + devPath;
        throw std::invalid_argument{msg.c_str()};
    }

    // Remove the trailing '-'
    links.pop_back();

    return links;
}

std::tuple<std::string, std::tuple<size_t, uint8_t>>
    getFSII2CSearchKeys(const std::string& devPath)
{
    // This combines the FSI and i2C search keys

    auto links = getFSISearchKeys(devPath);
    auto busAndAddr = getI2CSearchKeys(devPath);

    return {std::move(links), std::move(busAndAddr)};
}

size_t getSPISearchKeys(const std::string& devPath)
{
    std::smatch match;

    // Look for spi_master/spiX/ where X is the SPI bus/port number
    // Note: This doesn't distinguish between multiple chips on
    // the same port as no need for it yet.
    std::regex regex{"spi_master/spi(\\d+)/"};

    regex_search(devPath, match, regex);

    if (match.size() != 2)
    {
        std::string msg = "Could not get SPI bus from " + devPath;
        throw std::invalid_argument{msg.c_str()};
    }

    size_t port = std::stoul(match[1].str());

    return port;
}

std::tuple<std::string, size_t> getFSISPISearchKeys(const std::string& devPath)
{

    // Combine the FSI and SPI search keys.
    auto links = getFSISearchKeys(devPath);
    auto bus = getSPISearchKeys(devPath);

    return {std::move(links), std::move(bus)};
}

/**
 * @brief Pull the callouts out of the JSON callout array passed in
 *
 * Create a vector of Callout objects based on the JSON.
 *
 * This will also fill in the 'debug' member on the first callout
 * in the list, which could contain things like the I2C address and
 * bus extracted from the device path.
 *
 * The callouts are in the order they should be added to the PEL.
 *
 * @param[in] calloutJSON - The Callouts JSON array to extract from
 * @param[in] debug - The debug message to add to the first callout
 *
 * @return std::vector<Callout> - The Callout objects
 */
std::vector<Callout> extractCallouts(const nlohmann::json& calloutJSON,
                                     const std::string& debug)
{
    std::vector<Callout> callouts;
    bool addDebug = true;

    // The JSON element passed in is the array of callouts
    if (!calloutJSON.is_array())
    {
        throw std::runtime_error(
            "Dev path callout JSON entry doesn't contain a 'Callouts' array");
    }

    for (auto& callout : calloutJSON)
    {
        Callout c;

        // Add any debug data to the first callout
        if (addDebug && !debug.empty())
        {
            addDebug = false;
            c.debug = debug;
        }

        try
        {
            c.locationCode = callout.at("LocationCode").get<std::string>();
            c.name = callout.at("Name").get<std::string>();
            c.priority = callout.at("Priority").get<std::string>();

            if (callout.contains("MRU"))
            {
                c.mru = callout.at("MRU").get<std::string>();
            }
        }
        catch (const nlohmann::json::out_of_range& e)
        {
            std::string msg =
                "Callout entry missing either LocationCode, Name, or Priority "
                "properties: " +
                callout.dump();
            throw std::runtime_error(msg.c_str());
        }

        callouts.push_back(c);
    }

    return callouts;
}

/**
 * @brief Looks up the callouts in the JSON using the I2C keys.
 *
 * @param[in] i2cBus - The I2C bus
 * @param[in] i2cAddress - The I2C address
 * @param[in] calloutJSON - The JSON containing the callouts
 *
 * @return std::vector<Callout> - The callouts
 */
std::vector<device_callouts::Callout>
    calloutI2C(size_t i2cBus, uint8_t i2cAddress,
               const nlohmann::json& calloutJSON)
{
    auto busString = std::to_string(i2cBus);
    auto addrString = std::to_string(i2cAddress);

    try
    {
        const auto& callouts =
            calloutJSON.at("I2C").at(busString).at(addrString).at("Callouts");

        auto dest = calloutJSON.at("I2C")
                        .at(busString)
                        .at(addrString)
                        .at("Dest")
                        .get<std::string>();

        std::string msg = "I2C: bus: " + busString + " address: " + addrString +
                          " dest: " + dest;

        return extractCallouts(callouts, msg);
    }
    catch (const nlohmann::json::out_of_range& e)
    {
        std::string msg = "Problem looking up I2C callouts on " + busString +
                          " " + addrString + ": " + std::string{e.what()};
        throw std::invalid_argument(msg.c_str());
    }
}

/**
 * @brief Looks up the callouts in the JSON for this I2C path.
 *
 * @param[in] devPath - The device path
 * @param[in] calloutJSON - The JSON containing the callouts
 *
 * @return std::vector<Callout> - The callouts
 */
std::vector<device_callouts::Callout>
    calloutI2CUsingPath(const std::string& devPath,
                        const nlohmann::json& calloutJSON)
{
    auto [bus, address] = getI2CSearchKeys(devPath);

    return calloutI2C(bus, address, calloutJSON);
}

/**
 * @brief Looks up the callouts in the JSON for this FSI path.
 *
 * @param[in] devPath - The device path
 * @param[in] calloutJSON - The JSON containing the callouts
 *
 * @return std::vector<Callout> - The callouts
 */
std::vector<device_callouts::Callout>
    calloutFSI(const std::string& devPath, const nlohmann::json& calloutJSON)
{
    auto links = getFSISearchKeys(devPath);

    try
    {
        const auto& callouts = calloutJSON.at("FSI").at(links).at("Callouts");

        auto dest =
            calloutJSON.at("FSI").at(links).at("Dest").get<std::string>();

        std::string msg = "FSI: links: " + links + " dest: " + dest;

        return extractCallouts(callouts, msg);
    }
    catch (const nlohmann::json::out_of_range& e)
    {
        std::string msg = "Problem looking up FSI callouts on " + links + ": " +
                          std::string{e.what()};
        throw std::invalid_argument(msg.c_str());
    }
}

/**
 * @brief Looks up the callouts in the JSON for this FSI-I2C path.
 *
 * @param[in] devPath - The device path
 * @param[in] calloutJSON - The JSON containing the callouts
 *
 * @return std::vector<Callout> - The callouts
 */
std::vector<device_callouts::Callout>
    calloutFSII2C(const std::string& devPath, const nlohmann::json& calloutJSON)
{
    auto linksAndI2C = getFSII2CSearchKeys(devPath);
    auto links = std::get<std::string>(linksAndI2C);
    const auto& busAndAddr = std::get<1>(linksAndI2C);

    auto busString = std::to_string(std::get<size_t>(busAndAddr));
    auto addrString = std::to_string(std::get<uint8_t>(busAndAddr));

    try
    {
        auto& callouts = calloutJSON.at("FSI-I2C")
                             .at(links)
                             .at(busString)
                             .at(addrString)
                             .at("Callouts");

        auto dest = calloutJSON.at("FSI-I2C")
                        .at(links)
                        .at(busString)
                        .at(addrString)
                        .at("Dest")
                        .get<std::string>();

        std::string msg = "FSI-I2C: links: " + links + " bus: " + busString +
                          " addr: " + addrString + " dest: " + dest;

        return extractCallouts(callouts, msg);
    }
    catch (const nlohmann::json::out_of_range& e)
    {
        std::string msg = "Problem looking up FSI-I2C callouts on " + links +
                          " " + busString + " " + addrString + ": " + e.what();
        throw std::invalid_argument(msg.c_str());
    }
}

/**
 * @brief Looks up the callouts in the JSON for this FSI-SPI path.
 *
 * @param[in] devPath - The device path
 * @param[in] calloutJSON - The JSON containing the callouts
 *
 * @return std::vector<Callout> - The callouts
 */
std::vector<device_callouts::Callout>
    calloutFSISPI(const std::string& devPath, const nlohmann::json& calloutJSON)
{
    auto linksAndSPI = getFSISPISearchKeys(devPath);
    auto links = std::get<std::string>(linksAndSPI);
    auto busString = std::to_string(std::get<size_t>(linksAndSPI));

    try
    {
        auto& callouts =
            calloutJSON.at("FSI-SPI").at(links).at(busString).at("Callouts");

        auto dest = calloutJSON.at("FSI-SPI")
                        .at(links)
                        .at(busString)
                        .at("Dest")
                        .get<std::string>();

        std::string msg = "FSI-SPI: links: " + links + " bus: " + busString +
                          " dest: " + dest;

        return extractCallouts(callouts, msg);
    }
    catch (const nlohmann::json::out_of_range& e)
    {
        std::string msg = "Problem looking up FSI-SPI callouts on " + links +
                          " " + busString + ": " + std::string{e.what()};
        throw std::invalid_argument(msg.c_str());
    }
}

/**
 * @brief Returns the callouts from the JSON based on the input
 *        device path.
 *
 * @param[in] devPath - The device path
 * @param[in] json - The callout JSON
 *
 * @return std::vector<Callout> - The list of callouts
 */
std::vector<device_callouts::Callout> findCallouts(const std::string& devPath,
                                                   const nlohmann::json& json)
{
    std::vector<Callout> callouts;
    fs::path path;

    // Gives the /sys/devices/platform/ path
    try
    {
        path = fs::canonical(devPath);
    }
    catch (const fs::filesystem_error& e)
    {
        // Path not there, still try to do the callout
        path = devPath;
    }

    switch (util::getCalloutType(path))
    {
        case util::CalloutType::i2c:
            callouts = calloutI2CUsingPath(path, json);
            break;
        case util::CalloutType::fsi:
            callouts = calloutFSI(path, json);
            break;
        case util::CalloutType::fsii2c:
            callouts = calloutFSII2C(path, json);
            break;
        case util::CalloutType::fsispi:
            callouts = calloutFSISPI(path, json);
            break;
        default:
            std::string msg =
                "Could not get callout type from device path: " + path.string();
            throw std::invalid_argument{msg.c_str()};
            break;
    }

    return callouts;
}

CalloutType getCalloutType(const std::string& devPath)
{
    if ((devPath.find("fsi-master") != std::string::npos) &&
        (devPath.find("i2c-") != std::string::npos))
    {
        return CalloutType::fsii2c;
    }

    if ((devPath.find("fsi-master") != std::string::npos) &&
        (devPath.find("spi") != std::string::npos))
    {
        return CalloutType::fsispi;
    }

    // Treat anything else FSI related as plain FSI
    if (devPath.find("fsi-master") != std::string::npos)
    {
        return CalloutType::fsi;
    }

    if (devPath.find("i2c-bus/i2c-") != std::string::npos)
    {
        return CalloutType::i2c;
    }

    return CalloutType::unknown;
}

} // namespace util

std::vector<Callout> getCallouts(const std::string& devPath,
                                 const std::vector<std::string>& compatibleList)
{
    auto json = util::loadJSON(compatibleList);
    return util::findCallouts(devPath, json);
}

std::vector<Callout>
    getI2CCallouts(size_t i2cBus, uint8_t i2cAddress,
                   const std::vector<std::string>& compatibleList)
{
    auto json = util::loadJSON(compatibleList);
    return util::calloutI2C(i2cBus, i2cAddress, json);
}

} // namespace openpower::pels::device_callouts
