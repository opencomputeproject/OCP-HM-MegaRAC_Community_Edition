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

#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <multinodecommands.hpp>
#include <oemcommands.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message/types.hpp>

#include <string>

namespace ipmi
{
void registerMultiNodeFunctions() __attribute__((constructor));

std::optional<uint8_t> getMultiNodeInfo(std::string name)
{
    auto pdbus = getSdBus();
    try
    {
        std::string service =
            getService(*pdbus, multiNodeIntf, multiNodeObjPath);
        Value dbusValue = getDbusProperty(*pdbus, service, multiNodeObjPath,
                                          multiNodeIntf, name);
        return std::get<uint8_t>(dbusValue);
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "getMultiNodeInfo: can't get multi node info from dbus!",
            phosphor::logging::entry("ERR=%s", e.what()));
        return std::nullopt;
    }
}

std::optional<uint8_t> getMultiNodeRole()
{
    auto pdbus = getSdBus();
    try
    {
        std::string service =
            getService(*pdbus, multiNodeIntf, multiNodeObjPath);
        Value dbusValue = getDbusProperty(*pdbus, service, multiNodeObjPath,
                                          multiNodeIntf, "NodeRole");
        std::string valueStr = std::get<std::string>(dbusValue);
        uint8_t value;
        if (valueStr == "single")
            value = static_cast<uint8_t>(NodeRole::single);
        else if (valueStr == "master")
            value = static_cast<uint8_t>(NodeRole::master);
        else if (valueStr == "slave")
            value = static_cast<uint8_t>(NodeRole::slave);
        else if (valueStr == "arbitrating")
            value = static_cast<uint8_t>(NodeRole::arbitrating);
        return value;
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "getMultiNodeRole: can't get multi node role from dbus!",
            phosphor::logging::entry("ERR=%s", e.what()));
        return std::nullopt;
    }
}

ipmi::RspType<uint8_t> ipmiGetMultiNodePresence()

{
    std::optional<uint8_t> nodeInfo = getMultiNodeInfo("NodePresence");
    if (!nodeInfo)
    {
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(*nodeInfo);
}

ipmi::RspType<uint8_t> ipmiGetMultiNodeId()
{
    std::optional<uint8_t> nodeInfo = getMultiNodeInfo("NodeId");
    if (!nodeInfo)
    {
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(*nodeInfo);
}

ipmi::RspType<uint8_t> ipmiGetMultiNodeRole()
{
    std::optional<uint8_t> nodeInfo = getMultiNodeRole();
    if (!nodeInfo)
    {
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(*nodeInfo);
}

void registerMultiNodeFunctions(void)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Registering MultiNode commands");

    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnGeneral,
                          ipmi::intel::general::cmdGetMultiNodePresence,
                          ipmi::Privilege::User, ipmiGetMultiNodePresence);

    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnGeneral,
                          ipmi::intel::general::cmdGetMultiNodeId,
                          ipmi::Privilege::User, ipmiGetMultiNodeId);

    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnGeneral,
                          ipmi::intel::general::cmdGetMultiNodeRole,
                          ipmi::Privilege::User, ipmiGetMultiNodeRole);
}

} // namespace ipmi
