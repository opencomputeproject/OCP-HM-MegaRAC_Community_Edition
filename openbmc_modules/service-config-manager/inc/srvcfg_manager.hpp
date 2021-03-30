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
#include "utils.hpp"

#include <sdbusplus/timer.hpp>

namespace phosphor
{
namespace service
{

static constexpr const char* serviceConfigSrvName =
    "xyz.openbmc_project.Control.Service.Manager";
static constexpr const char* serviceConfigIntfName =
    "xyz.openbmc_project.Control.Service.Attributes";
static constexpr const char* sockAttrIntfName =
    "xyz.openbmc_project.Control.Service.SocketAttributes";
static constexpr const char* srcCfgMgrBasePath =
    "/xyz/openbmc_project/control/service";
static constexpr const char* srcCfgMgrIntf =
    "/xyz/openbmc_project.Control.Service.Manager";
static constexpr const char* sockAttrPropPort = "Port";
static constexpr const char* srvCfgPropMasked = "Masked";
static constexpr const char* srvCfgPropEnabled = "Enabled";
static constexpr const char* srvCfgPropRunning = "Running";

enum class UpdatedProp
{
    port = 1,
    maskedState,
    enabledState,
    runningState
};

using VariantType =
    std::variant<std::string, int64_t, uint64_t, double, int32_t, uint32_t,
                 int16_t, uint16_t, uint8_t, bool,
                 std::vector<std::tuple<std::string, std::string>>>;

class ServiceConfig
{
  public:
    ServiceConfig(sdbusplus::asio::object_server& srv_,
                  std::shared_ptr<sdbusplus::asio::connection>& conn_,
                  const std::string& objPath_, const std::string& baseUnitName,
                  const std::string& instanceName,
                  const std::string& serviceObjPath,
                  const std::string& socketObjPath);
    ~ServiceConfig() = default;

    std::shared_ptr<sdbusplus::asio::connection> conn;
    uint8_t updatedFlag;

    void stopAndApplyUnitConfig(boost::asio::yield_context yield);
    void restartUnitConfig(boost::asio::yield_context yield);
    void startServiceRestartTimer();

  private:
    sdbusplus::asio::object_server& server;
    std::shared_ptr<sdbusplus::asio::dbus_interface> srvCfgIface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> sockAttrIface;

    bool internalSet = false;
    std::string objPath;
    std::string baseUnitName;
    std::string instanceName;
    std::string instantiatedUnitName;
    std::string serviceObjectPath;
    std::string socketObjectPath;
    std::string overrideConfDir;

    // Properties
    std::string activeState;
    std::string subState;
    uint16_t portNum;
    std::vector<std::string> channelList;
    std::string protocol;
    std::string stateValue;
    bool unitMaskedState = false;
    bool unitEnabledState = false;
    bool unitRunningState = false;
    std::string subStateValue;

    bool isMaskedOut();
    void registerProperties();
    void queryAndUpdateProperties();
    void createSocketOverrideConf();
    void updateServiceProperties(
        const boost::container::flat_map<std::string, VariantType>&
            propertyMap);
    void updateSocketProperties(
        const boost::container::flat_map<std::string, VariantType>&
            propertyMap);
    std::string getSocketUnitName();
    std::string getServiceUnitName();
};

} // namespace service
} // namespace phosphor
