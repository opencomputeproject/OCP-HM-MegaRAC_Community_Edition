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

#include "Overlay.hpp"

#include "Utils.hpp"
#include "devices.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/process/child.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>

constexpr const char* OUTPUT_DIR = "/tmp/overlays";
constexpr const char* TEMPLATE_CHAR = "$";
constexpr const char* HEX_FORMAT_STR = "0x";
constexpr const char* I2C_DEVS_DIR = "/sys/bus/i2c/devices";
constexpr const char* MUX_SYMLINK_DIR = "/dev/i2c-mux";

constexpr const bool DEBUG = false;

std::regex ILLEGAL_NAME_REGEX("[^A-Za-z0-9_]");

// helper function to make json types into string
std::string jsonToString(const nlohmann::json& in)
{
    if (in.type() == nlohmann::json::value_t::string)
    {
        return in.get<std::string>();
    }
    else if (in.type() == nlohmann::json::value_t::array)
    {
        // remove brackets and comma from array
        std::string array = in.dump();
        array = array.substr(1, array.size() - 2);
        boost::replace_all(array, ",", " ");
        return array;
    }
    return in.dump();
}

void linkMux(const std::string& muxName, size_t busIndex, size_t address,
             const nlohmann::json::array_t& channelNames)
{
    std::error_code ec;
    std::filesystem::path muxSymlinkDir(MUX_SYMLINK_DIR);
    std::filesystem::create_directory(muxSymlinkDir, ec);
    // ignore error codes here if the directory already exists
    ec.clear();
    std::filesystem::path linkDir = muxSymlinkDir / muxName;
    std::filesystem::create_directory(linkDir, ec);

    std::ostringstream hexAddress;
    hexAddress << std::hex << std::setfill('0') << std::setw(4) << address;

    std::filesystem::path devDir(I2C_DEVS_DIR);
    devDir /= std::to_string(busIndex) + "-" + hexAddress.str();

    for (std::size_t channelIndex = 0; channelIndex < channelNames.size();
         channelIndex++)
    {
        const std::string* channelName =
            channelNames[channelIndex].get_ptr<const std::string*>();
        if (channelName == nullptr)
        {
            continue;
        }
        if (channelName->empty())
        {
            continue;
        }

        std::filesystem::path channelPath =
            devDir / ("channel-" + std::to_string(channelIndex));
        if (!is_symlink(channelPath))
        {
            std::cerr << channelPath << "for mux channel " << *channelName
                      << " doesn't exist!\n";
            continue;
        }
        std::filesystem::path bus = std::filesystem::read_symlink(channelPath);

        std::filesystem::path fp("/dev" / bus.filename());
        std::filesystem::path link(linkDir / *channelName);

        std::filesystem::create_symlink(fp, link, ec);
        if (ec)
        {
            std::cerr << "Failure creating symlink for " << fp << " to " << link
                      << "\n";
        }
    }
}

void exportDevice(const std::string& type,
                  const devices::ExportTemplate& exportTemplate,
                  const nlohmann::json& configuration)
{

    std::string parameters = exportTemplate.parameters;
    std::string device = exportTemplate.device;
    std::string name = "unknown";
    const uint64_t* bus = nullptr;
    const uint64_t* address = nullptr;
    const nlohmann::json::array_t* channels = nullptr;

    for (auto keyPair = configuration.begin(); keyPair != configuration.end();
         keyPair++)
    {
        std::string subsituteString;

        if (keyPair.key() == "Name" &&
            keyPair.value().type() == nlohmann::json::value_t::string)
        {
            subsituteString = std::regex_replace(
                keyPair.value().get<std::string>(), ILLEGAL_NAME_REGEX, "_");
            name = subsituteString;
        }
        else
        {
            subsituteString = jsonToString(keyPair.value());
        }

        if (keyPair.key() == "Bus")
        {
            bus = keyPair.value().get_ptr<const uint64_t*>();
        }
        else if (keyPair.key() == "Address")
        {
            address = keyPair.value().get_ptr<const uint64_t*>();
        }
        else if (keyPair.key() == "ChannelNames")
        {
            channels =
                keyPair.value().get_ptr<const nlohmann::json::array_t*>();
        }
        boost::replace_all(parameters, TEMPLATE_CHAR + keyPair.key(),
                           subsituteString);
        boost::replace_all(device, TEMPLATE_CHAR + keyPair.key(),
                           subsituteString);
    }

    // if we found bus and address we can attempt to prevent errors
    if (bus != nullptr && address != nullptr)
    {
        std::ostringstream hex;
        hex << std::hex << *address;
        const std::string& addressHex = hex.str();
        std::string busStr = std::to_string(*bus);

        std::filesystem::path devicePath(device);
        std::filesystem::path parentPath = devicePath.parent_path();
        if (std::filesystem::is_directory(parentPath))
        {
            for (const auto& path :
                 std::filesystem::directory_iterator(parentPath))
            {
                if (!std::filesystem::is_directory(path))
                {
                    continue;
                }

                const std::string& directoryName = path.path().filename();
                if (boost::starts_with(directoryName, busStr) &&
                    boost::ends_with(directoryName, addressHex))
                {
                    return; // already exported
                }
            }
        }
    }

    std::ofstream deviceFile(device);
    if (!deviceFile.good())
    {
        std::cerr << "Error writing " << device << "\n";
        return;
    }
    deviceFile << parameters;
    deviceFile.close();
    if (boost::ends_with(type, "Mux") && bus && address && channels)
    {
        linkMux(name, static_cast<size_t>(*bus), static_cast<size_t>(*address),
                *channels);
    }
}

bool loadOverlays(const nlohmann::json& systemConfiguration)
{
    std::filesystem::create_directory(OUTPUT_DIR);
    for (auto entity = systemConfiguration.begin();
         entity != systemConfiguration.end(); entity++)
    {
        auto findExposes = entity.value().find("Exposes");
        if (findExposes == entity.value().end() ||
            findExposes->type() != nlohmann::json::value_t::array)
        {
            continue;
        }

        for (auto& configuration : *findExposes)
        {
            auto findStatus = configuration.find("Status");
            // status missing is assumed to be 'okay'
            if (findStatus != configuration.end() && *findStatus == "disabled")
            {
                continue;
            }
            auto findType = configuration.find("Type");
            if (findType == configuration.end() ||
                findType->type() != nlohmann::json::value_t::string)
            {
                continue;
            }
            std::string type = findType.value().get<std::string>();
            auto device = devices::exportTemplates.find(type.c_str());
            if (device != devices::exportTemplates.end())
            {
                exportDevice(type, device->second, configuration);
                continue;
            }

            // Because many devices are intentionally not exportable,
            // this error message is not printed in all situations.
            // If wondering why your device not appearing, add your type to
            // the exportTemplates array in the devices.hpp file.
            if constexpr (DEBUG)
            {
                std::cerr << "Device type " << type
                          << " not found in export map whitelist\n";
            }
        }
    }

    return true;
}
