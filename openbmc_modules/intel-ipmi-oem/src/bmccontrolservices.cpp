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

#include "oemcommands.hpp"

#include <boost/algorithm/string.hpp>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/log.hpp>

#include <variant>

namespace ipmi
{
void register_netfn_bmc_control_functions() __attribute__((constructor));

static constexpr uint8_t rmcpServiceBitPos = 3;
static constexpr uint8_t webServiceBitPos = 5;
static constexpr uint8_t solServiceBitPos = 6;
static constexpr uint8_t kvmServiceBitPos = 15;

static const std::unordered_map<uint8_t, std::string> bmcServices = {
    // {bit position for service, service object path}
    {rmcpServiceBitPos,
     "/xyz/openbmc_project/control/service/phosphor_2dipmi_2dnet"},
    {webServiceBitPos, "/xyz/openbmc_project/control/service/bmcweb"},
    {solServiceBitPos, "/xyz/openbmc_project/control/service/obmc_2dconsole"},
    {kvmServiceBitPos, "/xyz/openbmc_project/control/service/start_2dipkvm"},
};

static constexpr uint16_t maskBit15 = 0xF000;

static constexpr const char* objectManagerIntf =
    "org.freedesktop.DBus.ObjectManager";
static constexpr const char* dBusPropIntf = "org.freedesktop.DBus.Properties";
static constexpr const char* serviceConfigBasePath =
    "/xyz/openbmc_project/control/service";
static constexpr const char* serviceConfigAttrIntf =
    "xyz.openbmc_project.Control.Service.Attributes";
static constexpr const char* getMgdObjMethod = "GetManagedObjects";
static constexpr const char* propMasked = "Masked";

std::string getServiceConfigMgrName()
{
    static std::string serviceCfgMgr{};
    if (serviceCfgMgr.empty())
    {
        try
        {
            auto sdbusp = getSdBus();
            serviceCfgMgr = ipmi::getService(*sdbusp, objectManagerIntf,
                                             serviceConfigBasePath);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            serviceCfgMgr.clear();
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Error: In fetching disabling service manager name");
            return serviceCfgMgr;
        }
    }
    return serviceCfgMgr;
}

static inline void checkAndThrowError(boost::system::error_code& ec,
                                      const std::string& msg)
{
    if (ec)
    {
        std::string msgToLog = ec.message() + (msg.empty() ? "" : " - " + msg);
        phosphor::logging::log<phosphor::logging::level::ERR>(msgToLog.c_str());
        throw sdbusplus::exception::SdBusError(-EIO, msgToLog.c_str());
    }
    return;
}

static inline bool getEnabledValue(const DbusInterfaceMap& intfMap)
{
    for (const auto& intf : intfMap)
    {
        if (intf.first == serviceConfigAttrIntf)
        {
            auto it = intf.second.find(propMasked);
            if (it == intf.second.end())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error: in getting Masked property value");
                throw sdbusplus::exception::SdBusError(
                    -EIO, "ERROR in reading Masked property value");
            }
            // return !Masked value
            return !std::get<bool>(it->second);
        }
    }
    return false;
}

ipmi::RspType<> setBmcControlServices(boost::asio::yield_context yield,
                                      uint8_t state, uint16_t serviceValue)
{
    constexpr uint16_t servicesRsvdMask = 0x3F97;
    constexpr uint8_t enableService = 0x1;

    if ((state > enableService) || (serviceValue & servicesRsvdMask) ||
        !serviceValue)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    try
    {
        auto sdbusp = getSdBus();
        boost::system::error_code ec;
        auto objectMap = sdbusp->yield_method_call<ObjectValueTree>(
            yield, ec, getServiceConfigMgrName().c_str(), serviceConfigBasePath,
            objectManagerIntf, getMgdObjMethod);
        checkAndThrowError(ec, "GetMangagedObjects for service cfg failed");

        for (const auto& services : bmcServices)
        {
            // services.first holds the bit position of the service, check
            // whether it has to be updated.
            const uint16_t serviceMask = 1 << services.first;
            if (!(serviceValue & serviceMask))
            {
                continue;
            }
            for (const auto& obj : objectMap)
            {
                if (boost::algorithm::starts_with(obj.first.str,
                                                  services.second))
                {
                    if (state != getEnabledValue(obj.second))
                    {
                        ec.clear();
                        sdbusp->yield_method_call<>(
                            yield, ec, getServiceConfigMgrName().c_str(),
                            obj.first.str, dBusPropIntf, "Set",
                            serviceConfigAttrIntf, propMasked,
                            std::variant<bool>(!state));
                        checkAndThrowError(ec, "Set Masked property failed");
                        // Multiple instances may be present, so continue
                    }
                }
            }
        }
    }
    catch (sdbusplus::exception::SdBusError& e)
    {
        return ipmi::responseUnspecifiedError();
    }
    return ipmi::responseSuccess();
}

ipmi::RspType<uint16_t> getBmcControlServices(boost::asio::yield_context yield)
{
    uint16_t serviceValue = 0;
    try
    {
        auto sdbusp = getSdBus();
        boost::system::error_code ec;
        auto objectMap = sdbusp->yield_method_call<ObjectValueTree>(
            yield, ec, getServiceConfigMgrName().c_str(), serviceConfigBasePath,
            objectManagerIntf, getMgdObjMethod);
        checkAndThrowError(ec, "GetMangagedObjects for service cfg failed");

        for (const auto& services : bmcServices)
        {
            for (const auto& obj : objectMap)
            {
                if (boost::algorithm::starts_with(obj.first.str,
                                                  services.second))
                {
                    serviceValue |= getEnabledValue(obj.second)
                                    << services.first;
                    break;
                }
            }
        }
    }
    catch (sdbusplus::exception::SdBusError& e)
    {
        return ipmi::responseUnspecifiedError();
    }
    // Bit 14 should match bit 15 as single service maintains Video & USB
    // redirection
    serviceValue |= (serviceValue & maskBit15) >> 1;
    return ipmi::responseSuccess(serviceValue);
}

void register_netfn_bmc_control_functions()
{
    registerHandler(prioOpenBmcBase, intel::netFnGeneral,
                    intel::general::cmdControlBmcServices, Privilege::Admin,
                    setBmcControlServices);

    registerHandler(prioOpenBmcBase, intel::netFnGeneral,
                    intel::general::cmdGetBmcServiceStatus, Privilege::User,
                    getBmcControlServices);
}
} // namespace ipmi
