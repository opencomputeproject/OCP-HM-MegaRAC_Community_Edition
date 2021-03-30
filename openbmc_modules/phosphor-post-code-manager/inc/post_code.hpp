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
#pragma once
#include <fcntl.h>
#include <unistd.h>

#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <chrono>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Collection/DeleteAll/server.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/State/Boot/PostCode/server.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

#define MaxPostCodeCycles 100

const static constexpr char *PostCodePath =
    "/xyz/openbmc_project/state/boot/raw";
const static constexpr char *PropertiesIntf = "org.freedesktop.DBus.Properties";
const static constexpr char *PostCodeListPath =
    "/var/lib/phosphor-post-code-manager/";
const static constexpr char *CurrentBootCycleCountName =
    "CurrentBootCycleCount";
const static constexpr char *CurrentBootCycleIndexName =
    "CurrentBootCycleIndex";
const static constexpr char *HostStatePath = "/xyz/openbmc_project/state/host0";

struct EventDeleter
{
    void operator()(sd_event *event) const
    {
        event = sd_event_unref(event);
    }
};
using EventPtr = std::unique_ptr<sd_event, EventDeleter>;
namespace fs = std::experimental::filesystem;
namespace StateServer = sdbusplus::xyz::openbmc_project::State::server;

using post_code =
    sdbusplus::xyz::openbmc_project::State::Boot::server::PostCode;
using delete_all =
    sdbusplus::xyz::openbmc_project::Collection::server::DeleteAll;

struct PostCode : sdbusplus::server::object_t<post_code, delete_all>
{
    PostCode(sdbusplus::bus::bus &bus, const char *path, EventPtr &event) :
        sdbusplus::server::object_t<post_code, delete_all>(bus, path), bus(bus),
        propertiesChangedSignalRaw(
            bus,
            sdbusplus::bus::match::rules::type::signal() +
                sdbusplus::bus::match::rules::member("PropertiesChanged") +
                sdbusplus::bus::match::rules::path(PostCodePath) +
                sdbusplus::bus::match::rules::interface(PropertiesIntf),
            [this](sdbusplus::message::message &msg) {
                std::string objectName;
                std::map<std::string, std::variant<uint64_t>> msgData;
                msg.read(objectName, msgData);
                // Check if it was the Value property that changed.
                auto valPropMap = msgData.find("Value");
                {
                    if (valPropMap != msgData.end())
                    {
                        this->savePostCodes(
                            std::get<uint64_t>(valPropMap->second));
                    }
                }
            }),
        propertiesChangedSignalCurrentHostState(
            bus,
            sdbusplus::bus::match::rules::type::signal() +
                sdbusplus::bus::match::rules::member("PropertiesChanged") +
                sdbusplus::bus::match::rules::path(HostStatePath) +
                sdbusplus::bus::match::rules::interface(PropertiesIntf),
            [this](sdbusplus::message::message &msg) {
                std::string objectName;
                std::map<std::string, std::variant<std::string>> msgData;
                msg.read(objectName, msgData);
                // Check if it was the Value property that changed.
                auto valPropMap = msgData.find("CurrentHostState");
                {
                    if (valPropMap != msgData.end())
                    {
                        StateServer::Host::HostState currentHostState =
                            StateServer::Host::convertHostStateFromString(
                                std::get<std::string>(valPropMap->second));
                        if (currentHostState ==
                            StateServer::Host::HostState::Off)
                        {
                            if (this->postCodes.empty())
                            {
                                std::cerr << "HostState changed to OFF. Empty "
                                             "postcode log, keep boot cycle at "
                                          << this->currentBootCycleIndex
                                          << std::endl;
                            }
                            else
                            {
                                this->postCodes.clear();
                            }
                        }
                    }
                }
            })
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "PostCode is created");
        auto dir = fs::path(PostCodeListPath);
        fs::create_directories(dir);
        strPostCodeListPath = PostCodeListPath;
        strCurrentBootCycleIndexName = CurrentBootCycleIndexName;
        uint16_t index = 0;
        deserialize(
            fs::path(strPostCodeListPath + strCurrentBootCycleIndexName),
            index);
        currentBootCycleIndex = index;
        strCurrentBootCycleCountName = CurrentBootCycleCountName;
        uint16_t count = 0;
        deserialize(
            fs::path(strPostCodeListPath + strCurrentBootCycleCountName),
            count);
        currentBootCycleCount(count);
        maxBootCycleNum(MaxPostCodeCycles);
    }
    ~PostCode()
    {
    }

    std::vector<uint64_t> getPostCodes(uint16_t index) override;
    std::map<uint64_t, uint64_t>
        getPostCodesWithTimeStamp(uint16_t index) override;
    void deleteAll() override;

  private:
    void incrBootCycle();
    uint16_t getBootNum(const uint16_t index) const;

    sdbusplus::bus::bus &bus;
    std::chrono::time_point<std::chrono::steady_clock> firstPostCodeTimeSteady;
    uint64_t firstPostCodeUsSinceEpoch;
    std::map<uint64_t, uint64_t> postCodes;
    std::string strPostCodeListPath;
    std::string strCurrentBootCycleIndexName;
    uint16_t currentBootCycleIndex;
    std::string strCurrentBootCycleCountName;
    void savePostCodes(uint64_t code);
    sdbusplus::bus::match_t propertiesChangedSignalRaw;
    sdbusplus::bus::match_t propertiesChangedSignalCurrentHostState;
    fs::path serialize(const std::string &path);
    bool deserialize(const fs::path &path, uint16_t &index);
    bool deserializePostCodes(const fs::path &path,
                              std::map<uint64_t, uint64_t> &codes);
};
