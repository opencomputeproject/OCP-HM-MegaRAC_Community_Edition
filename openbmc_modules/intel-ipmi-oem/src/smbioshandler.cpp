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

#include <commandutils.hpp>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <smbioshandler.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

using level = phosphor::logging::level;

constexpr const char* DBUS_PROPERTIES = "org.freedesktop.DBus.Properties";
constexpr const char* MDRV1_PATH = "/xyz/openbmc_project/Smbios/MDR_V1";
constexpr const char* MDRV1_INTERFACE = "xyz.openbmc_project.Smbios.MDR_V1";
static constexpr uint8_t maxDataLen = 254;

static void register_netfn_smbios_functions() __attribute__((constructor));

ipmi_ret_t cmd_region_status(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                             ipmi_request_t request, ipmi_response_t response,
                             ipmi_data_len_t data_len, ipmi_context_t context)
{
    auto requestData = reinterpret_cast<const RegionStatusRequest*>(request);
    std::vector<uint8_t> status;

    if (*data_len != sizeof(RegionStatusRequest))
    {
        *data_len = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    uint8_t regionId = requestData->regionId - 1;
    *data_len = 0;

    if (regionId >= maxMDRId)
    {
        phosphor::logging::log<level::ERR>("Invalid region");
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, MDRV1_INTERFACE, MDRV1_PATH);

    auto method = bus->new_method_call(service.c_str(), MDRV1_PATH,
                                       MDRV1_INTERFACE, "RegionStatus");
    method.append(regionId);
    auto reply = bus->call(method);
    if (reply.is_method_error())
    {
        phosphor::logging::log<level::ERR>(
            "Error get region status",
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", MDRV1_PATH));
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    reply.read(status);

    if (status.size() != sizeof(MDRState))
    {
        phosphor::logging::log<level::ERR>(
            "Error get region status, return length invalid");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    *data_len = static_cast<size_t>(status.size());
    auto dataOut = reinterpret_cast<uint8_t*>(response);
    std::copy(&status[0], &status[*data_len], dataOut);
    return IPMI_CC_OK;
}

int sdplus_mdrv1_get_property(const std::string& name,
                              std::variant<uint8_t, uint16_t>& value,
                              std::string& service)
{
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    auto method = bus->new_method_call(service.c_str(), MDRV1_PATH,
                                       DBUS_PROPERTIES, "Get");
    method.append(MDRV1_INTERFACE, name);
    auto reply = bus->call(method);
    if (reply.is_method_error())
    {
        phosphor::logging::log<level::ERR>(
            "Error getting property, sdbusplus call failed");
        return -1;
    }
    reply.read(value);

    return 0;
}

static int set_regionId(uint8_t regionId, std::string& service)
{
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    auto method = bus->new_method_call(service.c_str(), MDRV1_PATH,
                                       DBUS_PROPERTIES, "Set");
    std::variant<uint8_t> value{regionId};
    method.append(MDRV1_INTERFACE, "RegionId", value);
    auto region = bus->call(method);
    if (region.is_method_error())
    {
        phosphor::logging::log<level::ERR>(
            "Error setting regionID, sdbusplus call failed");
        return -1;
    }
    return 0;
}

ipmi_ret_t cmd_region_complete(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                               ipmi_request_t request, ipmi_response_t response,
                               ipmi_data_len_t data_len, ipmi_context_t context)
{
    auto requestData = reinterpret_cast<const RegionCompleteRequest*>(request);
    uint8_t status;

    std::variant<uint8_t, uint16_t> value;

    if (*data_len != sizeof(RegionCompleteRequest))
    {
        *data_len = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    uint8_t regionId = requestData->regionId - 1;
    *data_len = 0;

    if (regionId >= maxMDRId)
    {
        phosphor::logging::log<level::ERR>("Invalid region");
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, MDRV1_INTERFACE, MDRV1_PATH);

    if (set_regionId(regionId, service) < 0)
    {
        phosphor::logging::log<level::ERR>("Error setting regionId");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    if (0 > sdplus_mdrv1_get_property("LockPolicy", value, service))
    {
        phosphor::logging::log<level::ERR>("Error getting lockPolicy");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    if (regionLockUnlocked == std::get<uint8_t>(value))
    {
        return IPMI_CC_PARAMETER_NOT_SUPPORT_IN_PRESENT_STATE;
    }

    if (0 > sdplus_mdrv1_get_property("SessionId", value, service))
    {
        phosphor::logging::log<level::ERR>("Error getting sessionId");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    if (requestData->sessionId != std::get<uint8_t>(value))
    {
        return IPMI_CC_OEM_SET_IN_PROCESS;
    }

    auto method = bus->new_method_call(service.c_str(), MDRV1_PATH,
                                       MDRV1_INTERFACE, "RegionComplete");

    method.append(regionId);

    auto reply = bus->call(method);
    if (reply.is_method_error())
    {
        phosphor::logging::log<level::ERR>(
            "Error set region complete",
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", MDRV1_PATH));
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    reply.read(status);

    if (status != 0)
        phosphor::logging::log<level::ERR>(
            "Error set region complete, unexpected error");
    return IPMI_CC_UNSPECIFIED_ERROR;

    return IPMI_CC_OK;
}

ipmi_ret_t cmd_region_read(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                           ipmi_request_t request, ipmi_response_t response,
                           ipmi_data_len_t data_len, ipmi_context_t context)
{
    auto requestData = reinterpret_cast<const RegionReadRequest*>(request);
    auto responseData = reinterpret_cast<RegionReadResponse*>(response);
    std::variant<uint8_t, uint16_t> regUsedVal;
    std::variant<uint8_t, uint16_t> lockPolicyVal;
    std::vector<uint8_t> res;

    if (*data_len < sizeof(RegionReadRequest))
    {
        *data_len = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    uint8_t regionId = requestData->regionId - 1;

    *data_len = 0;

    if (regionId >= maxMDRId)
    {
        phosphor::logging::log<level::ERR>("Invalid region");
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, MDRV1_INTERFACE, MDRV1_PATH);
    // TODO to make sure the interface can get correct LockPolicy even
    // regionId changed by another task.
    if (set_regionId(regionId, service) < 0)
    {
        phosphor::logging::log<level::ERR>("Error setting regionId");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    if (0 > sdplus_mdrv1_get_property("RegionUsed", regUsedVal, service))
    {
        phosphor::logging::log<level::ERR>("Error getting regionUsed");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    if ((requestData->length >= maxDataLen) ||
        (requestData->offset + requestData->length >
         std::get<uint16_t>(regUsedVal)))
    {
        phosphor::logging::log<level::INFO>(
            "Invalid data request",
            phosphor::logging::entry("OFFSET=%d", requestData->offset),
            phosphor::logging::entry("LENGTH=%d", requestData->length),
            phosphor::logging::entry("REGUSED=%d",
                                     std::get<uint16_t>(regUsedVal)));
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    if (0 > sdplus_mdrv1_get_property("LockPolicy", lockPolicyVal, service))
    {
        phosphor::logging::log<level::ERR>("Error getting lockPolicy");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    if (regionLockUnlocked != std::get<uint8_t>(lockPolicyVal))
    {
        return IPMI_CC_PARAMETER_NOT_SUPPORT_IN_PRESENT_STATE;
    }

    auto method = bus->new_method_call(service.c_str(), MDRV1_PATH,
                                       MDRV1_INTERFACE, "RegionRead");

    method.append(regionId, requestData->length, requestData->offset);

    auto reply = bus->call(method);
    if (reply.is_method_error())
    {
        phosphor::logging::log<level::ERR>(
            "Error read region data",
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", MDRV1_PATH));
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    reply.read(res);

    *data_len = responseData->length = res[0];
    responseData->updateCount = res[1];

    if ((*data_len == 0) || (*data_len >= maxDataLen))
    {
        phosphor::logging::log<level::ERR>(
            "Data length send from service is invalid");
        *data_len = 0;
        return IPMI_CC_RESPONSE_ERROR;
    }

    *data_len += 2 * sizeof(uint8_t);
    std::copy(&res[2], &res[*data_len], responseData->data);
    return IPMI_CC_OK;
}

ipmi_ret_t cmd_region_write(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                            ipmi_request_t request, ipmi_response_t response,
                            ipmi_data_len_t data_len, ipmi_context_t context)
{
    auto requestData = reinterpret_cast<const RegionWriteRequest*>(request);
    uint8_t regionId = requestData->regionId - 1;
    std::string res;
    std::vector<uint8_t> writeData;
    uint16_t index;
    uint8_t tmp[255];

    size_t minInputLen = &requestData->data[0] - &requestData->sessionId + 1;
    if (*data_len < minInputLen)
    { // this command need at least 6 bytes input
        *data_len = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    std::variant<uint8_t, uint16_t> value;

    *data_len = 0;

    if (regionId >= maxMDRId)
    {
        phosphor::logging::log<level::ERR>("Invalid region");
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, MDRV1_INTERFACE, MDRV1_PATH);

    if (set_regionId(regionId, service) < 0)
    {
        phosphor::logging::log<level::ERR>("Error setting regionId");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    if (0 > sdplus_mdrv1_get_property("LockPolicy", value, service))
    {
        phosphor::logging::log<level::ERR>("Error getting lockPolicy");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    if (regionLockUnlocked == std::get<uint8_t>(value))
    {
        return IPMI_CC_PARAMETER_NOT_SUPPORT_IN_PRESENT_STATE;
    }

    if (0 > sdplus_mdrv1_get_property("SessionId", value, service))
    {
        phosphor::logging::log<level::ERR>("Error getting sessionId");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    if (requestData->sessionId != std::get<uint8_t>(value))
    {
        return IPMI_CC_OEM_SET_IN_PROCESS;
    }

    std::copy(&(requestData->length), &(requestData->data[requestData->length]),
              tmp);
    writeData.push_back(regionId);
    for (index = 0; index < minInputLen + requestData->length - 2; index++)
    {
        writeData.push_back(tmp[index]);
    }

    auto method = bus->new_method_call(service.c_str(), MDRV1_PATH,
                                       MDRV1_INTERFACE, "RegionWrite");

    method.append(writeData);

    auto reply = bus->call(method);
    if (reply.is_method_error())
    {
        phosphor::logging::log<level::ERR>(
            "Error write region data",
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", MDRV1_PATH));
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    reply.read(res);

    if (res == "NoData")
    {
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }
    else if (res != "Success")
    {
        phosphor::logging::log<level::ERR>(
            "Error write region data, unexpected error");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return IPMI_CC_OK;
}

ipmi_ret_t cmd_region_lock(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                           ipmi_request_t request, ipmi_response_t response,
                           ipmi_data_len_t data_len, ipmi_context_t context)
{
    auto requestData = reinterpret_cast<const RegionLockRequest*>(request);
    uint8_t regionId = requestData->regionId - 1;
    std::variant<uint8_t, uint16_t> value;
    auto res = reinterpret_cast<uint8_t*>(response);
    uint8_t lockResponse;

    if (*data_len != sizeof(RegionLockRequest))
    {
        *data_len = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    *data_len = 0;

    if (regionId >= maxMDRId)
    {
        phosphor::logging::log<level::ERR>("Invalid region");
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, MDRV1_INTERFACE, MDRV1_PATH);

    if (set_regionId(regionId, service) < 0)
    {
        phosphor::logging::log<level::ERR>("Error setting regionId");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    if (0 > sdplus_mdrv1_get_property("LockPolicy", value, service))
    {
        phosphor::logging::log<level::ERR>("Error getting lockPolicy");
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    if (requestData->lockPolicy == regionLockUnlocked)
    {
        if (regionLockUnlocked == std::get<uint8_t>(value))
        {
            return IPMI_CC_PARAMETER_NOT_SUPPORT_IN_PRESENT_STATE;
        }
    }
    if (regionLockUnlocked != std::get<uint8_t>(value))
    {
        if (0 > sdplus_mdrv1_get_property("SessionId", value, service))
        {
            phosphor::logging::log<level::ERR>("Error getting sessionId");
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
        if (requestData->sessionId != std::get<uint8_t>(value))
        {
            if (requestData->lockPolicy != regionLockStrict)
            {
                return IPMI_CC_OEM_SET_IN_PROCESS;
            }
        }
    }
    auto method = bus->new_method_call(service.c_str(), MDRV1_PATH,
                                       MDRV1_INTERFACE, "RegionLock");

    method.append(requestData->sessionId, regionId, requestData->lockPolicy,
                  requestData->msTimeout);

    auto reply = bus->call(method);
    if (reply.is_method_error())
    {
        phosphor::logging::log<level::ERR>(
            "Error lock region ",
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", MDRV1_PATH));
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    reply.read(lockResponse);

    *data_len = sizeof(lockResponse);
    *res = lockResponse;
    return IPMI_CC_OK;
}

static void register_netfn_smbios_functions(void)
{
#ifdef MDR_V1_SUPPORT
    // MDR V1 Command
    // <Get MDR Status Command>
    ipmi_register_callback(ipmi::intel::netFnApp,
                           ipmi::intel::app::cmdMdrStatus, NULL,
                           cmd_region_status, PRIVILEGE_OPERATOR);

    // <Update Complete Status Command>
    ipmi_register_callback(ipmi::intel::netFnApp,
                           ipmi::intel::app::cmdMdrComplete, NULL,
                           cmd_region_complete, PRIVILEGE_OPERATOR);

    // <Read MDR Command>
    ipmi_register_callback(ipmi::intel::netFnApp, ipmi::intel::app::cmdMdrRead,
                           NULL, cmd_region_read, PRIVILEGE_OPERATOR);

    // <Write MDR Command>
    ipmi_register_callback(ipmi::intel::netFnApp, ipmi::intel::app::cmdMdrWrite,
                           NULL, cmd_region_write, PRIVILEGE_OPERATOR);

    // <Lock MDR Command>
    ipmi_register_callback(ipmi::intel::netFnApp, ipmi::intel::app::cmdMdrLock,
                           NULL, cmd_region_lock, PRIVILEGE_OPERATOR);
#endif
}
