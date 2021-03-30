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
#pragma once
#include <boost/asio.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <string>

static constexpr const char* sysdStartUnit = "StartUnit";
static constexpr const char* sysdStopUnit = "StopUnit";
static constexpr const char* sysdRestartUnit = "RestartUnit";
static constexpr const char* sysdReloadMethod = "Reload";
static constexpr const char* sysdGetJobMethod = "GetJob";
static constexpr const char* sysdReplaceMode = "replace";
static constexpr const char* dBusGetAllMethod = "GetAll";
static constexpr const char* dBusGetMethod = "Get";
static constexpr const char* sysdService = "org.freedesktop.systemd1";
static constexpr const char* sysdObjPath = "/org/freedesktop/systemd1";
static constexpr const char* sysdMgrIntf = "org.freedesktop.systemd1.Manager";
static constexpr const char* sysdUnitIntf = "org.freedesktop.systemd1.Unit";
static constexpr const char* sysdSocketIntf = "org.freedesktop.systemd1.Socket";
static constexpr const char* dBusPropIntf = "org.freedesktop.DBus.Properties";
static constexpr const char* stateMasked = "masked";
static constexpr const char* stateEnabled = "enabled";
static constexpr const char* stateDisabled = "disabled";
static constexpr const char* subStateRunning = "running";

static inline std::string addInstanceName(const std::string& instanceName,
                                          const std::string& suffix)
{
    return (instanceName.empty() ? "" : suffix + instanceName);
}

void systemdDaemonReload(
    const std::shared_ptr<sdbusplus::asio::connection>& conn,
    boost::asio::yield_context yield);

void systemdUnitAction(const std::shared_ptr<sdbusplus::asio::connection>& conn,
                       boost::asio::yield_context yield,
                       const std::string& unitName,
                       const std::string& actionMethod);

void systemdUnitFilesStateChange(
    const std::shared_ptr<sdbusplus::asio::connection>& conn,
    boost::asio::yield_context yield, const std::vector<std::string>& unitFiles,
    const std::string& unitState, bool maskedState, bool enabledState);
