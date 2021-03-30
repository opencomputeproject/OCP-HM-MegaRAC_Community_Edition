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

#include "types.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Led/Physical/server.hpp"

#include <systemd/sd-journal.h>

#include <appcommands.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <com/intel/Control/OCOTShutdownPolicy/server.hpp>
#include <commandutils.hpp>
#include <gpiod.hpp>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <nlohmann/json.hpp>
#include <oemcommands.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message/types.hpp>
#include <xyz/openbmc_project/Chassis/Control/NMISource/server.hpp>
#include <xyz/openbmc_project/Control/Boot/Mode/server.hpp>
#include <xyz/openbmc_project/Control/Boot/Source/server.hpp>
#include <xyz/openbmc_project/Control/PowerSupplyRedundancy/server.hpp>
#include <xyz/openbmc_project/Control/Security/RestrictionMode/server.hpp>
#include <xyz/openbmc_project/Control/Security/SpecialMode/server.hpp>

#include <array>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <variant>
#include <vector>

namespace ipmi
{
static void registerOEMFunctions() __attribute__((constructor));

static constexpr size_t maxFRUStringLength = 0x3F;

static constexpr auto ethernetIntf =
    "xyz.openbmc_project.Network.EthernetInterface";
static constexpr auto networkIPIntf = "xyz.openbmc_project.Network.IP";
static constexpr auto networkService = "xyz.openbmc_project.Network";
static constexpr auto networkRoot = "/xyz/openbmc_project/network";

static constexpr const char* oemNmiSourceIntf =
    "xyz.openbmc_project.Chassis.Control.NMISource";
static constexpr const char* oemNmiSourceObjPath =
    "/xyz/openbmc_project/Chassis/Control/NMISource";
static constexpr const char* oemNmiBmcSourceObjPathProp = "BMCSource";
static constexpr const char* oemNmiEnabledObjPathProp = "Enabled";

static constexpr const char* dimmOffsetFile = "/var/lib/ipmi/ipmi_dimms.json";
static constexpr const char* multiNodeObjPath =
    "/xyz/openbmc_project/MultiNode/Status";
static constexpr const char* multiNodeIntf =
    "xyz.openbmc_project.Chassis.MultiNode";

enum class NmiSource : uint8_t
{
    none = 0,
    frontPanelButton = 1,
    watchdog = 2,
    chassisCmd = 3,
    memoryError = 4,
    pciBusError = 5,
    pch = 6,
    chipset = 7,
};

enum class SpecialUserIndex : uint8_t
{
    rootUser = 0,
    atScaleDebugUser = 1
};

static constexpr const char* restricionModeService =
    "xyz.openbmc_project.RestrictionMode.Manager";
static constexpr const char* restricionModeBasePath =
    "/xyz/openbmc_project/control/security/restriction_mode";
static constexpr const char* restricionModeIntf =
    "xyz.openbmc_project.Control.Security.RestrictionMode";
static constexpr const char* restricionModeProperty = "RestrictionMode";

static constexpr const char* specialModeService =
    "xyz.openbmc_project.SpecialMode";
static constexpr const char* specialModeBasePath =
    "/xyz/openbmc_project/security/special_mode";
static constexpr const char* specialModeIntf =
    "xyz.openbmc_project.Security.SpecialMode";
static constexpr const char* specialModeProperty = "SpecialMode";

static constexpr const char* dBusPropertyIntf =
    "org.freedesktop.DBus.Properties";
static constexpr const char* dBusPropertyGetMethod = "Get";
static constexpr const char* dBusPropertySetMethod = "Set";

// return code: 0 successful
int8_t getChassisSerialNumber(sdbusplus::bus::bus& bus, std::string& serial)
{
    std::string objpath = "/xyz/openbmc_project/FruDevice";
    std::string intf = "xyz.openbmc_project.FruDeviceManager";
    std::string service = getService(bus, intf, objpath);
    ObjectValueTree valueTree = getManagedObjects(bus, service, "/");
    if (valueTree.empty())
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "No object implements interface",
            phosphor::logging::entry("INTF=%s", intf.c_str()));
        return -1;
    }

    for (const auto& item : valueTree)
    {
        auto interface = item.second.find("xyz.openbmc_project.FruDevice");
        if (interface == item.second.end())
        {
            continue;
        }

        auto property = interface->second.find("CHASSIS_SERIAL_NUMBER");
        if (property == interface->second.end())
        {
            continue;
        }

        try
        {
            Value variant = property->second;
            std::string& result = std::get<std::string>(variant);
            if (result.size() > maxFRUStringLength)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "FRU serial number exceed maximum length");
                return -1;
            }
            serial = result;
            return 0;
        }
        catch (std::bad_variant_access& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
            return -1;
        }
    }
    return -1;
}

// Returns the Chassis Identifier (serial #)
ipmi_ret_t ipmiOEMGetChassisIdentifier(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                       ipmi_request_t request,
                                       ipmi_response_t response,
                                       ipmi_data_len_t dataLen,
                                       ipmi_context_t context)
{
    std::string serial;
    if (*dataLen != 0) // invalid request if there are extra parameters
    {
        *dataLen = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    if (getChassisSerialNumber(*dbus, serial) == 0)
    {
        *dataLen = serial.size(); // length will never exceed response length
                                  // as it is checked in getChassisSerialNumber
        char* resp = static_cast<char*>(response);
        serial.copy(resp, *dataLen);
        return IPMI_CC_OK;
    }
    *dataLen = 0;
    return IPMI_CC_RESPONSE_ERROR;
}

ipmi_ret_t ipmiOEMSetSystemGUID(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                ipmi_request_t request,
                                ipmi_response_t response,
                                ipmi_data_len_t dataLen, ipmi_context_t context)
{
    static constexpr size_t safeBufferLength = 50;
    char buf[safeBufferLength] = {0};
    GUIDData* Data = reinterpret_cast<GUIDData*>(request);

    if (*dataLen != sizeof(GUIDData)) // 16bytes
    {
        *dataLen = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    *dataLen = 0;

    snprintf(
        buf, safeBufferLength,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        Data->timeLow4, Data->timeLow3, Data->timeLow2, Data->timeLow1,
        Data->timeMid2, Data->timeMid1, Data->timeHigh2, Data->timeHigh1,
        Data->clock2, Data->clock1, Data->node6, Data->node5, Data->node4,
        Data->node3, Data->node2, Data->node1);
    // UUID is in RFC4122 format. Ex: 61a39523-78f2-11e5-9862-e6402cfc3223
    std::string guid = buf;

    std::string objpath = "/xyz/openbmc_project/control/host0/systemGUID";
    std::string intf = "xyz.openbmc_project.Common.UUID";
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    std::string service = getService(*dbus, intf, objpath);
    setDbusProperty(*dbus, service, objpath, intf, "UUID", guid);
    return IPMI_CC_OK;
}

ipmi::RspType<> ipmiOEMDisableBMCSystemReset(bool disableResetOnSMI,
                                             uint7_t reserved1)
{
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();

    try
    {
        auto service =
            ipmi::getService(*busp, bmcResetDisablesIntf, bmcResetDisablesPath);
        ipmi::setDbusProperty(*busp, service, bmcResetDisablesPath,
                              bmcResetDisablesIntf, "ResetOnSMI",
                              !disableResetOnSMI);
    }
    catch (std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to set BMC reset disables",
            phosphor::logging::entry("EXCEPTION=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

ipmi::RspType<bool,   // disableResetOnSMI
              uint7_t // reserved
              >
    ipmiOEMGetBMCResetDisables()
{
    bool disableResetOnSMI = true;

    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    try
    {
        auto service =
            ipmi::getService(*busp, bmcResetDisablesIntf, bmcResetDisablesPath);
        Value variant =
            ipmi::getDbusProperty(*busp, service, bmcResetDisablesPath,
                                  bmcResetDisablesIntf, "ResetOnSMI");
        disableResetOnSMI = !std::get<bool>(variant);
    }
    catch (std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get BMC reset disables",
            phosphor::logging::entry("EXCEPTION=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(disableResetOnSMI, 0);
}

ipmi_ret_t ipmiOEMSetBIOSID(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                            ipmi_request_t request, ipmi_response_t response,
                            ipmi_data_len_t dataLen, ipmi_context_t context)
{
    DeviceInfo* data = reinterpret_cast<DeviceInfo*>(request);

    if ((*dataLen < 2) || (*dataLen != (1 + data->biosIDLength)))
    {
        *dataLen = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }
    std::string idString((char*)data->biosId, data->biosIDLength);

    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    std::string service = getService(*dbus, biosVersionIntf, biosActiveObjPath);
    setDbusProperty(*dbus, service, biosActiveObjPath, biosVersionIntf,
                    biosVersionProp, idString);
    uint8_t* bytesWritten = static_cast<uint8_t*>(response);
    *bytesWritten =
        data->biosIDLength; // how many bytes are written into storage
    *dataLen = 1;
    return IPMI_CC_OK;
}

bool getSwVerInfo(ipmi::Context::ptr ctx, uint8_t& bmcMajor, uint8_t& bmcMinor,
                  uint8_t& meMajor, uint8_t& meMinor)
{
    // step 1 : get BMC Major and Minor numbers from its DBUS property
    std::string bmcVersion;
    if (getActiveSoftwareVersionInfo(ctx, versionPurposeBMC, bmcVersion))
    {
        return false;
    }

    std::optional<MetaRevision> rev = convertIntelVersion(bmcVersion);
    if (rev.has_value())
    {
        MetaRevision revision = rev.value();
        bmcMajor = revision.major;

        revision.minor = (revision.minor > 99 ? 99 : revision.minor);
        bmcMinor = revision.minor % 10 + (revision.minor / 10) * 16;
    }

    // step 2 : get ME Major and Minor numbers from its DBUS property
    std::string meVersion;
    if (getActiveSoftwareVersionInfo(ctx, versionPurposeME, meVersion))
    {
        return false;
    }
    std::regex pattern1("(\\d+?).(\\d+?).(\\d+?).(\\d+?).(\\d+?)");
    constexpr size_t matchedPhosphor = 6;
    std::smatch results;
    if (std::regex_match(meVersion, results, pattern1))
    {
        if (results.size() == matchedPhosphor)
        {
            meMajor = static_cast<uint8_t>(std::stoi(results[1]));
            meMinor = static_cast<uint8_t>(std::stoi(results[2]));
        }
    }
    return true;
}

ipmi::RspType<
    std::variant<std::string,
                 std::tuple<uint8_t, std::array<uint8_t, 2>,
                            std::array<uint8_t, 2>, std::array<uint8_t, 2>,
                            std::array<uint8_t, 2>, std::array<uint8_t, 2>>,
                 std::tuple<uint8_t, std::array<uint8_t, 2>>>>
    ipmiOEMGetDeviceInfo(ipmi::Context::ptr ctx, uint8_t entityType,
                         std::optional<uint8_t> countToRead,
                         std::optional<uint8_t> offset)
{
    if (entityType > static_cast<uint8_t>(OEMDevEntityType::sdrVer))
    {
        return ipmi::responseInvalidFieldRequest();
    }

    // handle OEM command items
    switch (OEMDevEntityType(entityType))
    {
        case OEMDevEntityType::biosId:
        {
            // Byte 2&3, Only used with selecting BIOS
            if (!countToRead || !offset)
            {
                return ipmi::responseReqDataLenInvalid();
            }

            std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
            std::string service =
                getService(*dbus, biosVersionIntf, biosActiveObjPath);
            try
            {
                Value variant =
                    getDbusProperty(*dbus, service, biosActiveObjPath,
                                    biosVersionIntf, biosVersionProp);
                std::string& idString = std::get<std::string>(variant);
                if (*offset >= idString.size())
                {
                    return ipmi::responseParmOutOfRange();
                }
                size_t length = 0;
                if (*countToRead > (idString.size() - *offset))
                {
                    length = idString.size() - *offset;
                }
                else
                {
                    length = *countToRead;
                }

                std::string readBuf = {0};
                readBuf.resize(length);
                std::copy_n(idString.begin() + *offset, length,
                            (readBuf.begin()));
                return ipmi::responseSuccess(readBuf);
            }
            catch (std::bad_variant_access& e)
            {
                return ipmi::responseUnspecifiedError();
            }
        }
        break;

        case OEMDevEntityType::devVer:
        {
            // Byte 2&3, Only used with selecting BIOS
            if (countToRead || offset)
            {
                return ipmi::responseReqDataLenInvalid();
            }

            constexpr const size_t verLen = 2;
            constexpr const size_t verTotalLen = 10;
            std::array<uint8_t, verLen> bmcBuf = {0xff, 0xff};
            std::array<uint8_t, verLen> hsc0Buf = {0xff, 0xff};
            std::array<uint8_t, verLen> hsc1Buf = {0xff, 0xff};
            std::array<uint8_t, verLen> meBuf = {0xff, 0xff};
            std::array<uint8_t, verLen> hsc2Buf = {0xff, 0xff};
            // data0/1: BMC version number; data6/7: ME version number
            // the others: HSC0/1/2 version number, not avaible.
            if (!getSwVerInfo(ctx, bmcBuf[0], bmcBuf[1], meBuf[0], meBuf[1]))
            {
                return ipmi::responseUnspecifiedError();
            }
            return ipmi::responseSuccess(
                std::tuple<
                    uint8_t, std::array<uint8_t, verLen>,
                    std::array<uint8_t, verLen>, std::array<uint8_t, verLen>,
                    std::array<uint8_t, verLen>, std::array<uint8_t, verLen>>{
                    verTotalLen, bmcBuf, hsc0Buf, hsc1Buf, meBuf, hsc2Buf});
        }
        break;

        case OEMDevEntityType::sdrVer:
        {
            // Byte 2&3, Only used with selecting BIOS
            if (countToRead || offset)
            {
                return ipmi::responseReqDataLenInvalid();
            }

            constexpr const size_t sdrLen = 2;
            std::array<uint8_t, sdrLen> readBuf = {0x01, 0x0};
            return ipmi::responseSuccess(
                std::tuple<uint8_t, std::array<uint8_t, sdrLen>>{sdrLen,
                                                                 readBuf});
        }
        break;

        default:
            return ipmi::responseInvalidFieldRequest();
    }
}

ipmi_ret_t ipmiOEMGetAICFRU(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                            ipmi_request_t request, ipmi_response_t response,
                            ipmi_data_len_t dataLen, ipmi_context_t context)
{
    if (*dataLen != 0)
    {
        *dataLen = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    *dataLen = 1;
    uint8_t* res = reinterpret_cast<uint8_t*>(response);
    // temporary fix. We don't support AIC FRU now. Just tell BIOS that no
    // AIC is available so that BIOS will not timeout repeatly which leads to
    // slow booting.
    *res = 0; // Byte1=Count of SlotPosition/FruID records.
    return IPMI_CC_OK;
}

ipmi_ret_t ipmiOEMGetPowerRestoreDelay(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                       ipmi_request_t request,
                                       ipmi_response_t response,
                                       ipmi_data_len_t dataLen,
                                       ipmi_context_t context)
{
    GetPowerRestoreDelayRes* resp =
        reinterpret_cast<GetPowerRestoreDelayRes*>(response);

    if (*dataLen != 0)
    {
        *dataLen = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    std::string service =
        getService(*dbus, powerRestoreDelayIntf, powerRestoreDelayObjPath);
    Value variant =
        getDbusProperty(*dbus, service, powerRestoreDelayObjPath,
                        powerRestoreDelayIntf, powerRestoreDelayProp);

    uint16_t delay = std::get<uint16_t>(variant);
    resp->byteLSB = delay;
    resp->byteMSB = delay >> 8;

    *dataLen = sizeof(GetPowerRestoreDelayRes);

    return IPMI_CC_OK;
}

static uint8_t bcdToDec(uint8_t val)
{
    return ((val / 16 * 10) + (val % 16));
}

// Allows an update utility or system BIOS to send the status of an embedded
// firmware update attempt to the BMC. After received, BMC will create a logging
// record.
ipmi::RspType<> ipmiOEMSendEmbeddedFwUpdStatus(uint8_t status, uint8_t target,
                                               uint8_t majorRevision,
                                               uint8_t minorRevision,
                                               uint32_t auxInfo)
{
    std::string firmware;
    int instance = (target & targetInstanceMask) >> targetInstanceShift;
    target = (target & selEvtTargetMask) >> selEvtTargetShift;

    /* make sure the status is 0, 1, or 2 as per the spec */
    if (status > 2)
    {
        return ipmi::response(ipmi::ccInvalidFieldRequest);
    }
    /* make sure the target is 0, 1, 2, or 4 as per the spec */
    if (target > 4 || target == 3)
    {
        return ipmi::response(ipmi::ccInvalidFieldRequest);
    }
    /*orignal OEM command is to record OEM SEL.
    But openbmc does not support OEM SEL, so we redirect it to redfish event
    logging. */
    std::string buildInfo;
    std::string action;
    switch (FWUpdateTarget(target))
    {
        case FWUpdateTarget::targetBMC:
            firmware = "BMC";
            buildInfo = "major: " + std::to_string(majorRevision) + " minor: " +
                        std::to_string(bcdToDec(minorRevision)) + // BCD encoded
                        " BuildID: " + std::to_string(auxInfo);
            buildInfo += std::to_string(auxInfo);
            break;
        case FWUpdateTarget::targetBIOS:
            firmware = "BIOS";
            buildInfo =
                "major: " +
                std::to_string(bcdToDec(majorRevision)) + // BCD encoded
                " minor: " +
                std::to_string(bcdToDec(minorRevision)) + // BCD encoded
                " ReleaseNumber: " +                      // ASCII encoded
                std::to_string(static_cast<uint8_t>(auxInfo >> 0) - '0') +
                std::to_string(static_cast<uint8_t>(auxInfo >> 8) - '0') +
                std::to_string(static_cast<uint8_t>(auxInfo >> 16) - '0') +
                std::to_string(static_cast<uint8_t>(auxInfo >> 24) - '0');
            break;
        case FWUpdateTarget::targetME:
            firmware = "ME";
            buildInfo =
                "major: " + std::to_string(majorRevision) + " minor1: " +
                std::to_string(bcdToDec(minorRevision)) + // BCD encoded
                " minor2: " +
                std::to_string(bcdToDec(static_cast<uint8_t>(auxInfo >> 0))) +
                " build1: " +
                std::to_string(bcdToDec(static_cast<uint8_t>(auxInfo >> 8))) +
                " build2: " +
                std::to_string(bcdToDec(static_cast<uint8_t>(auxInfo >> 16)));
            break;
        case FWUpdateTarget::targetOEMEWS:
            firmware = "EWS";
            buildInfo = "major: " + std::to_string(majorRevision) + " minor: " +
                        std::to_string(bcdToDec(minorRevision)) + // BCD encoded
                        " BuildID: " + std::to_string(auxInfo);
            break;
    }

    static const std::string openBMCMessageRegistryVersion("0.1");
    std::string redfishMsgID = "OpenBMC." + openBMCMessageRegistryVersion;

    switch (status)
    {
        case 0x0:
            action = "update started";
            redfishMsgID += ".FirmwareUpdateStarted";
            break;
        case 0x1:
            action = "update completed successfully";
            redfishMsgID += ".FirmwareUpdateCompleted";
            break;
        case 0x2:
            action = "update failure";
            redfishMsgID += ".FirmwareUpdateFailed";
            break;
        default:
            action = "unknown";
            break;
    }

    std::string firmwareInstanceStr =
        firmware + " instance: " + std::to_string(instance);
    std::string message("[firmware update] " + firmwareInstanceStr +
                        " status: <" + action + "> " + buildInfo);

    sd_journal_send("MESSAGE=%s", message.c_str(), "PRIORITY=%i", LOG_INFO,
                    "REDFISH_MESSAGE_ID=%s", redfishMsgID.c_str(),
                    "REDFISH_MESSAGE_ARGS=%s,%s", firmwareInstanceStr.c_str(),
                    buildInfo.c_str(), NULL);
    return ipmi::responseSuccess();
}

ipmi::RspType<uint8_t, std::vector<uint8_t>>
    ipmiOEMSlotIpmb(ipmi::Context::ptr ctx, uint6_t reserved1,
                    uint2_t slotNumber, uint3_t baseBoardSlotNum,
                    uint3_t riserSlotNum, uint2_t reserved2, uint8_t slaveAddr,
                    uint8_t netFn, uint8_t cmd,
                    std::optional<std::vector<uint8_t>> writeData)
{
    if (reserved1 || reserved2)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    boost::system::error_code ec;
    using ipmbResponse = std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t,
                                    std::vector<uint8_t>>;
    ipmbResponse res = ctx->bus->yield_method_call<ipmbResponse>(
        ctx->yield, ec, "xyz.openbmc_project.Ipmi.Channel.Ipmb",
        "/xyz/openbmc_project/Ipmi/Channel/Ipmb", "org.openbmc.Ipmb",
        "SlotIpmbRequest", static_cast<uint8_t>(slotNumber),
        static_cast<uint8_t>(baseBoardSlotNum), slaveAddr, netFn, cmd,
        *writeData);
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to call dbus method SlotIpmbRequest");
        return ipmi::responseUnspecifiedError();
    }

    std::vector<uint8_t> dataReceived(0);
    int status = -1;
    uint8_t resNetFn = 0, resLun = 0, resCmd = 0, cc = 0;

    std::tie(status, resNetFn, resLun, resCmd, cc, dataReceived) = res;

    if (status)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get response from SlotIpmbRequest");
        return ipmi::responseResponseError();
    }
    return ipmi::responseSuccess(cc, dataReceived);
}

ipmi_ret_t ipmiOEMSetPowerRestoreDelay(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                       ipmi_request_t request,
                                       ipmi_response_t response,
                                       ipmi_data_len_t dataLen,
                                       ipmi_context_t context)
{
    SetPowerRestoreDelayReq* data =
        reinterpret_cast<SetPowerRestoreDelayReq*>(request);
    uint16_t delay = 0;

    if (*dataLen != sizeof(SetPowerRestoreDelayReq))
    {
        *dataLen = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }
    delay = data->byteMSB;
    delay = (delay << 8) | data->byteLSB;
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    std::string service =
        getService(*dbus, powerRestoreDelayIntf, powerRestoreDelayObjPath);
    setDbusProperty(*dbus, service, powerRestoreDelayObjPath,
                    powerRestoreDelayIntf, powerRestoreDelayProp, delay);
    *dataLen = 0;

    return IPMI_CC_OK;
}

static bool cpuPresent(const std::string& cpuName)
{
    static constexpr const char* cpuPresencePathPrefix =
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/";
    static constexpr const char* cpuPresenceIntf =
        "xyz.openbmc_project.Inventory.Item";
    std::string cpuPresencePath = cpuPresencePathPrefix + cpuName;
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    try
    {
        auto service =
            ipmi::getService(*busp, cpuPresenceIntf, cpuPresencePath);

        ipmi::Value result = ipmi::getDbusProperty(
            *busp, service, cpuPresencePath, cpuPresenceIntf, "Present");
        return std::get<bool>(result);
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Cannot find processor presence",
            phosphor::logging::entry("NAME=%s", cpuName.c_str()));
        return false;
    }
}

ipmi::RspType<bool,    // CATERR Reset Enabled
              bool,    // ERR2 Reset Enabled
              uint6_t, // reserved
              uint8_t, // reserved, returns 0x3F
              uint6_t, // CPU1 CATERR Count
              uint2_t, // CPU1 Status
              uint6_t, // CPU2 CATERR Count
              uint2_t, // CPU2 Status
              uint6_t, // CPU3 CATERR Count
              uint2_t, // CPU3 Status
              uint6_t, // CPU4 CATERR Count
              uint2_t, // CPU4 Status
              uint8_t  // Crashdump Count
              >
    ipmiOEMGetProcessorErrConfig()
{
    bool resetOnCATERR = false;
    bool resetOnERR2 = false;
    uint6_t cpu1CATERRCount = 0;
    uint6_t cpu2CATERRCount = 0;
    uint6_t cpu3CATERRCount = 0;
    uint6_t cpu4CATERRCount = 0;
    uint8_t crashdumpCount = 0;
    uint2_t cpu1Status =
        cpuPresent("CPU_1") ? CPUStatus::enabled : CPUStatus::notPresent;
    uint2_t cpu2Status =
        cpuPresent("CPU_2") ? CPUStatus::enabled : CPUStatus::notPresent;
    uint2_t cpu3Status =
        cpuPresent("CPU_3") ? CPUStatus::enabled : CPUStatus::notPresent;
    uint2_t cpu4Status =
        cpuPresent("CPU_4") ? CPUStatus::enabled : CPUStatus::notPresent;

    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    try
    {
        auto service = ipmi::getService(*busp, processorErrConfigIntf,
                                        processorErrConfigObjPath);

        ipmi::PropertyMap result = ipmi::getAllDbusProperties(
            *busp, service, processorErrConfigObjPath, processorErrConfigIntf);
        resetOnCATERR = std::get<bool>(result.at("ResetOnCATERR"));
        resetOnERR2 = std::get<bool>(result.at("ResetOnERR2"));
        cpu1CATERRCount = std::get<uint8_t>(result.at("ErrorCountCPU1"));
        cpu2CATERRCount = std::get<uint8_t>(result.at("ErrorCountCPU2"));
        cpu3CATERRCount = std::get<uint8_t>(result.at("ErrorCountCPU3"));
        cpu4CATERRCount = std::get<uint8_t>(result.at("ErrorCountCPU4"));
        crashdumpCount = std::get<uint8_t>(result.at("CrashdumpCount"));
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to fetch processor error config",
            phosphor::logging::entry("ERROR=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(resetOnCATERR, resetOnERR2, 0, 0x3F,
                                 cpu1CATERRCount, cpu1Status, cpu2CATERRCount,
                                 cpu2Status, cpu3CATERRCount, cpu3Status,
                                 cpu4CATERRCount, cpu4Status, crashdumpCount);
}

ipmi::RspType<> ipmiOEMSetProcessorErrConfig(
    bool resetOnCATERR, bool resetOnERR2, uint6_t reserved1, uint8_t reserved2,
    std::optional<bool> clearCPUErrorCount,
    std::optional<bool> clearCrashdumpCount, std::optional<uint6_t> reserved3)
{
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();

    try
    {
        auto service = ipmi::getService(*busp, processorErrConfigIntf,
                                        processorErrConfigObjPath);
        ipmi::setDbusProperty(*busp, service, processorErrConfigObjPath,
                              processorErrConfigIntf, "ResetOnCATERR",
                              resetOnCATERR);
        ipmi::setDbusProperty(*busp, service, processorErrConfigObjPath,
                              processorErrConfigIntf, "ResetOnERR2",
                              resetOnERR2);
        if (clearCPUErrorCount.value_or(false))
        {
            ipmi::setDbusProperty(*busp, service, processorErrConfigObjPath,
                                  processorErrConfigIntf, "ErrorCountCPU1",
                                  static_cast<uint8_t>(0));
            ipmi::setDbusProperty(*busp, service, processorErrConfigObjPath,
                                  processorErrConfigIntf, "ErrorCountCPU2",
                                  static_cast<uint8_t>(0));
            ipmi::setDbusProperty(*busp, service, processorErrConfigObjPath,
                                  processorErrConfigIntf, "ErrorCountCPU3",
                                  static_cast<uint8_t>(0));
            ipmi::setDbusProperty(*busp, service, processorErrConfigObjPath,
                                  processorErrConfigIntf, "ErrorCountCPU4",
                                  static_cast<uint8_t>(0));
        }
        if (clearCrashdumpCount.value_or(false))
        {
            ipmi::setDbusProperty(*busp, service, processorErrConfigObjPath,
                                  processorErrConfigIntf, "CrashdumpCount",
                                  static_cast<uint8_t>(0));
        }
    }
    catch (std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to set processor error config",
            phosphor::logging::entry("EXCEPTION=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

ipmi_ret_t ipmiOEMGetShutdownPolicy(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                    ipmi_request_t request,
                                    ipmi_response_t response,
                                    ipmi_data_len_t dataLen,
                                    ipmi_context_t context)
{
    GetOEMShutdownPolicyRes* resp =
        reinterpret_cast<GetOEMShutdownPolicyRes*>(response);

    if (*dataLen != 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "oem_get_shutdown_policy: invalid input len!");
        *dataLen = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    *dataLen = 0;

    try
    {
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        std::string service =
            getService(*dbus, oemShutdownPolicyIntf, oemShutdownPolicyObjPath);
        Value variant = getDbusProperty(
            *dbus, service, oemShutdownPolicyObjPath, oemShutdownPolicyIntf,
            oemShutdownPolicyObjPathProp);

        if (sdbusplus::com::intel::Control::server::OCOTShutdownPolicy::
                convertPolicyFromString(std::get<std::string>(variant)) ==
            sdbusplus::com::intel::Control::server::OCOTShutdownPolicy::Policy::
                NoShutdownOnOCOT)
        {
            resp->policy = 0;
        }
        else if (sdbusplus::com::intel::Control::server::OCOTShutdownPolicy::
                     convertPolicyFromString(std::get<std::string>(variant)) ==
                 sdbusplus::com::intel::Control::server::OCOTShutdownPolicy::
                     Policy::ShutdownOnOCOT)
        {
            resp->policy = 1;
        }
        else
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "oem_set_shutdown_policy: invalid property!",
                phosphor::logging::entry(
                    "PROP=%s", std::get<std::string>(variant).c_str()));
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
        // TODO needs to check if it is multi-node products,
        // policy is only supported on node 3/4
        resp->policySupport = shutdownPolicySupported;
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.description());
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    *dataLen = sizeof(GetOEMShutdownPolicyRes);
    return IPMI_CC_OK;
}

ipmi_ret_t ipmiOEMSetShutdownPolicy(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                    ipmi_request_t request,
                                    ipmi_response_t response,
                                    ipmi_data_len_t dataLen,
                                    ipmi_context_t context)
{
    uint8_t* req = reinterpret_cast<uint8_t*>(request);
    sdbusplus::com::intel::Control::server::OCOTShutdownPolicy::Policy policy =
        sdbusplus::com::intel::Control::server::OCOTShutdownPolicy::Policy::
            NoShutdownOnOCOT;

    // TODO needs to check if it is multi-node products,
    // policy is only supported on node 3/4
    if (*dataLen != 1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "oem_set_shutdown_policy: invalid input len!");
        *dataLen = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    *dataLen = 0;
    if ((*req != noShutdownOnOCOT) && (*req != shutdownOnOCOT))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "oem_set_shutdown_policy: invalid input!");
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    if (*req == noShutdownOnOCOT)
    {
        policy = sdbusplus::com::intel::Control::server::OCOTShutdownPolicy::
            Policy::NoShutdownOnOCOT;
    }
    else
    {
        policy = sdbusplus::com::intel::Control::server::OCOTShutdownPolicy::
            Policy::ShutdownOnOCOT;
    }

    try
    {
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        std::string service =
            getService(*dbus, oemShutdownPolicyIntf, oemShutdownPolicyObjPath);
        setDbusProperty(
            *dbus, service, oemShutdownPolicyObjPath, oemShutdownPolicyIntf,
            oemShutdownPolicyObjPathProp,
            sdbusplus::com::intel::Control::server::convertForMessage(policy));
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.description());
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return IPMI_CC_OK;
}

/** @brief implementation for check the DHCP or not in IPv4
 *  @param[in] Channel - Channel number
 *  @returns true or false.
 */
static bool isDHCPEnabled(uint8_t Channel)
{
    try
    {
        auto ethdevice = getChannelName(Channel);
        if (ethdevice.empty())
        {
            return false;
        }
        auto ethIP = ethdevice + "/ipv4";
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        auto ethernetObj =
            getDbusObject(*dbus, networkIPIntf, networkRoot, ethIP);
        auto value = getDbusProperty(*dbus, networkService, ethernetObj.first,
                                     networkIPIntf, "Origin");
        if (std::get<std::string>(value) ==
            "xyz.openbmc_project.Network.IP.AddressOrigin.DHCP")
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.description());
        return true;
    }
}

/** @brief implementes for check the DHCP or not in IPv6
 *  @param[in] Channel - Channel number
 *  @returns true or false.
 */
static bool isDHCPIPv6Enabled(uint8_t Channel)
{

    try
    {
        auto ethdevice = getChannelName(Channel);
        if (ethdevice.empty())
        {
            return false;
        }
        auto ethIP = ethdevice + "/ipv6";
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        auto objectInfo =
            getDbusObject(*dbus, networkIPIntf, networkRoot, ethIP);
        auto properties = getAllDbusProperties(*dbus, objectInfo.second,
                                               objectInfo.first, networkIPIntf);
        if (std::get<std::string>(properties["Origin"]) ==
            "xyz.openbmc_project.Network.IP.AddressOrigin.DHCP")
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.description());
        return true;
    }
}

/** @brief implementes the creating of default new user
 *  @param[in] userName - new username in 16 bytes.
 *  @param[in] userPassword - new password in 20 bytes
 *  @returns ipmi completion code.
 */
ipmi::RspType<> ipmiOEMSetUser2Activation(
    std::array<uint8_t, ipmi::ipmiMaxUserName>& userName,
    std::array<uint8_t, ipmi::maxIpmi20PasswordSize>& userPassword)
{
    bool userState = false;
    // Check for System Interface not exist and LAN should be static
    for (uint8_t channel = 0; channel < maxIpmiChannels; channel++)
    {
        ChannelInfo chInfo;
        try
        {
            getChannelInfo(channel, chInfo);
        }
        catch (sdbusplus::exception_t& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMSetUser2Activation: Failed to get Channel Info",
                phosphor::logging::entry("MSG: %s", e.description()));
            return ipmi::response(ipmi::ccUnspecifiedError);
        }
        if (chInfo.mediumType ==
            static_cast<uint8_t>(EChannelMediumType::systemInterface))
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMSetUser2Activation: system interface  exist .");
            return ipmi::response(ipmi::ccCommandNotAvailable);
        }
        else
        {

            if (chInfo.mediumType ==
                static_cast<uint8_t>(EChannelMediumType::lan8032))
            {
                if (isDHCPIPv6Enabled(channel) || isDHCPEnabled(channel))
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "ipmiOEMSetUser2Activation: DHCP enabled .");
                    return ipmi::response(ipmi::ccCommandNotAvailable);
                }
            }
        }
    }
    uint8_t maxChUsers = 0, enabledUsers = 0, fixedUsers = 0;
    if (ipmi::ccSuccess ==
        ipmiUserGetAllCounts(maxChUsers, enabledUsers, fixedUsers))
    {
        if (enabledUsers > 1)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMSetUser2Activation: more than one user is enabled.");
            return ipmi::response(ipmi::ccCommandNotAvailable);
        }
        // Check the user 2 is enabled or not
        ipmiUserCheckEnabled(ipmiDefaultUserId, userState);
        if (userState == true)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMSetUser2Activation: user 2 already enabled .");
            return ipmi::response(ipmi::ccCommandNotAvailable);
        }
    }
    else
    {
        return ipmi::response(ipmi::ccUnspecifiedError);
    }

#if BYTE_ORDER == LITTLE_ENDIAN
    PrivAccess privAccess = {PRIVILEGE_ADMIN, true, true, true, 0};
#endif
#if BYTE_ORDER == BIG_ENDIAN
    PrivAccess privAccess = {0, true, true, true, PRIVILEGE_ADMIN};
#endif

    // ipmiUserSetUserName correctly handles char*, possibly non-null
    // terminated strings using ipmiMaxUserName size
    auto userNameRaw = reinterpret_cast<const char*>(userName.data());

    if (ipmi::ccSuccess == ipmiUserSetUserName(ipmiDefaultUserId, userNameRaw))
    {
        if (ipmi::ccSuccess ==
            ipmiUserSetUserPassword(
                ipmiDefaultUserId,
                reinterpret_cast<const char*>(userPassword.data())))
        {
            if (ipmi::ccSuccess ==
                ipmiUserSetPrivilegeAccess(
                    ipmiDefaultUserId,
                    static_cast<uint8_t>(ipmi::EChannelID::chanLan1),
                    privAccess, true))
            {
                phosphor::logging::log<phosphor::logging::level::INFO>(
                    "ipmiOEMSetUser2Activation: user created successfully ");
                return ipmi::responseSuccess();
            }
        }
        // we need to delete  the default user id which added in this command as
        // password / priv setting is failed.
        ipmiUserSetUserName(ipmiDefaultUserId, "");
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMSetUser2Activation: password / priv setting is failed.");
    }
    else
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMSetUser2Activation: Setting username failed.");
    }

    return ipmi::response(ipmi::ccCommandNotAvailable);
}

/** @brief implementes executing the linux command
 *  @param[in] linux command
 *  @returns status
 */

static uint8_t executeCmd(const char* path)
{
    boost::process::child execProg(path);
    execProg.wait();

    int retCode = execProg.exit_code();
    if (retCode)
    {
        return ipmi::ccUnspecifiedError;
    }
    return ipmi::ccSuccess;
}

/** @brief implementes ASD Security event logging
 *  @param[in] Event message string
 *  @param[in] Event Severity
 *  @returns status
 */

static void atScaleDebugEventlog(std::string msg, int severity)
{
    std::string eventStr = "OpenBMC.0.1." + msg;
    sd_journal_send("MESSAGE=Security Event: %s", eventStr.c_str(),
                    "PRIORITY=%i", severity, "REDFISH_MESSAGE_ID=%s",
                    eventStr.c_str(), NULL);
}

/** @brief implementes setting password for special user
 *  @param[in] specialUserIndex
 *  @param[in] userPassword - new password in 20 bytes
 *  @returns ipmi completion code.
 */
ipmi::RspType<> ipmiOEMSetSpecialUserPassword(ipmi::Context::ptr ctx,
                                              uint8_t specialUserIndex,
                                              std::vector<uint8_t> userPassword)
{
    ChannelInfo chInfo;
    ipmi_ret_t status = ipmi::ccSuccess;

    try
    {
        getChannelInfo(ctx->channel, chInfo);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMSetSpecialUserPassword: Failed to get Channel Info",
            phosphor::logging::entry("MSG: %s", e.description()));
        return ipmi::responseUnspecifiedError();
    }
    if (chInfo.mediumType !=
        static_cast<uint8_t>(EChannelMediumType::systemInterface))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMSetSpecialUserPassword: Error - supported only in KCS "
            "interface");
        return ipmi::responseCommandNotAvailable();
    }

    // 0 for root user  and 1 for AtScaleDebug is allowed
    if (specialUserIndex >
        static_cast<uint8_t>(SpecialUserIndex::atScaleDebugUser))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMSetSpecialUserPassword: Invalid user account");
        return ipmi::responseParmOutOfRange();
    }
    if (userPassword.size() != 0)
    {
        constexpr uint8_t minPasswordSizeRequired = 6;
        std::string passwd;
        if (userPassword.size() < minPasswordSizeRequired ||
            userPassword.size() > ipmi::maxIpmi20PasswordSize)
        {
            return ipmi::responseReqDataLenInvalid();
        }
        passwd.assign(reinterpret_cast<const char*>(userPassword.data()),
                      userPassword.size());
        if (specialUserIndex ==
            static_cast<uint8_t>(SpecialUserIndex::atScaleDebugUser))
        {
            status = ipmiSetSpecialUserPassword("asdbg", passwd);

            atScaleDebugEventlog("AtScaleDebugSpecialUserEnabled", LOG_CRIT);
        }
        else
        {
            status = ipmiSetSpecialUserPassword("root", passwd);
        }
        return ipmi::response(status);
    }
    else
    {
        if (specialUserIndex ==
            static_cast<uint8_t>(SpecialUserIndex::rootUser))
        {
            status = executeCmd("passwd -d root");
        }
        else
        {

            status = executeCmd("passwd -d asdbg");

            if (status == 0)
            {
                atScaleDebugEventlog("AtScaleDebugSpecialUserDisabled",
                                     LOG_INFO);
            }
        }
        return ipmi::response(status);
    }
}

namespace ledAction
{
using namespace sdbusplus::xyz::openbmc_project::Led::server;
std::map<Physical::Action, uint8_t> actionDbusToIpmi = {
    {Physical::Action::Off, 0},
    {Physical::Action::On, 2},
    {Physical::Action::Blink, 1}};

std::map<uint8_t, std::string> offsetObjPath = {
    {2, statusAmberObjPath}, {4, statusGreenObjPath}, {6, identifyLEDObjPath}};

} // namespace ledAction

int8_t getLEDState(sdbusplus::bus::bus& bus, const std::string& intf,
                   const std::string& objPath, uint8_t& state)
{
    try
    {
        std::string service = getService(bus, intf, objPath);
        Value stateValue =
            getDbusProperty(bus, service, objPath, intf, "State");
        std::string strState = std::get<std::string>(stateValue);
        state = ledAction::actionDbusToIpmi.at(
            sdbusplus::xyz::openbmc_project::Led::server::Physical::
                convertActionFromString(strState));
    }
    catch (sdbusplus::exception::SdBusError& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        return -1;
    }
    return 0;
}

ipmi::RspType<uint8_t> ipmiOEMGetLEDStatus()
{
    uint8_t ledstate = 0;
    phosphor::logging::log<phosphor::logging::level::DEBUG>("GET led status");
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    for (auto it = ledAction::offsetObjPath.begin();
         it != ledAction::offsetObjPath.end(); ++it)
    {
        uint8_t state = 0;
        if (getLEDState(*dbus, ledIntf, it->second, state) == -1)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "oem_get_led_status: fail to get ID LED status!");
            return ipmi::responseUnspecifiedError();
        }
        ledstate |= state << it->first;
    }
    return ipmi::responseSuccess(ledstate);
}

ipmi_ret_t ipmiOEMCfgHostSerialPortSpeed(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                         ipmi_request_t request,
                                         ipmi_response_t response,
                                         ipmi_data_len_t dataLen,
                                         ipmi_context_t context)
{
    CfgHostSerialReq* req = reinterpret_cast<CfgHostSerialReq*>(request);
    uint8_t* resp = reinterpret_cast<uint8_t*>(response);

    if (*dataLen == 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "CfgHostSerial: invalid input len!",
            phosphor::logging::entry("LEN=%d", *dataLen));
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    switch (req->command)
    {
        case getHostSerialCfgCmd:
        {
            if (*dataLen != 1)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "CfgHostSerial: invalid input len!");
                *dataLen = 0;
                return IPMI_CC_REQ_DATA_LEN_INVALID;
            }

            *dataLen = 0;

            boost::process::ipstream is;
            std::vector<std::string> data;
            std::string line;
            boost::process::child c1(fwGetEnvCmd, "-n", fwHostSerailCfgEnvName,
                                     boost::process::std_out > is);

            while (c1.running() && std::getline(is, line) && !line.empty())
            {
                data.push_back(line);
            }

            c1.wait();
            if (c1.exit_code())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "CfgHostSerial:: error on execute",
                    phosphor::logging::entry("EXECUTE=%s", fwSetEnvCmd));
                // Using the default value
                *resp = 0;
            }
            else
            {
                if (data.size() != 1)
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "CfgHostSerial:: error on read env");
                    return IPMI_CC_UNSPECIFIED_ERROR;
                }
                try
                {
                    unsigned long tmp = std::stoul(data[0]);
                    if (tmp > std::numeric_limits<uint8_t>::max())
                    {
                        throw std::out_of_range("Out of range");
                    }
                    *resp = static_cast<uint8_t>(tmp);
                }
                catch (const std::invalid_argument& e)
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "invalid config ",
                        phosphor::logging::entry("ERR=%s", e.what()));
                    return IPMI_CC_UNSPECIFIED_ERROR;
                }
                catch (const std::out_of_range& e)
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "out_of_range config ",
                        phosphor::logging::entry("ERR=%s", e.what()));
                    return IPMI_CC_UNSPECIFIED_ERROR;
                }
            }

            *dataLen = 1;
            break;
        }
        case setHostSerialCfgCmd:
        {
            if (*dataLen != sizeof(CfgHostSerialReq))
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "CfgHostSerial: invalid input len!");
                *dataLen = 0;
                return IPMI_CC_REQ_DATA_LEN_INVALID;
            }

            *dataLen = 0;

            if (req->parameter > HostSerialCfgParamMax)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "CfgHostSerial: invalid input!");
                return IPMI_CC_INVALID_FIELD_REQUEST;
            }

            boost::process::child c1(fwSetEnvCmd, fwHostSerailCfgEnvName,
                                     std::to_string(req->parameter));

            c1.wait();
            if (c1.exit_code())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "CfgHostSerial:: error on execute",
                    phosphor::logging::entry("EXECUTE=%s", fwGetEnvCmd));
                return IPMI_CC_UNSPECIFIED_ERROR;
            }
            break;
        }
        default:
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "CfgHostSerial: invalid input!");
            *dataLen = 0;
            return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    return IPMI_CC_OK;
}

constexpr const char* thermalModeInterface =
    "xyz.openbmc_project.Control.ThermalMode";
constexpr const char* thermalModePath =
    "/xyz/openbmc_project/control/thermal_mode";

bool getFanProfileInterface(
    sdbusplus::bus::bus& bus,
    boost::container::flat_map<
        std::string, std::variant<std::vector<std::string>, std::string>>& resp)
{
    auto call = bus.new_method_call(settingsBusName, thermalModePath, PROP_INTF,
                                    "GetAll");
    call.append(thermalModeInterface);
    try
    {
        auto data = bus.call(call);
        data.read(resp);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "getFanProfileInterface: can't get thermal mode!",
            phosphor::logging::entry("ERR=%s", e.what()));
        return false;
    }
    return true;
}

/**@brief implements the OEM set fan config.
 * @param selectedFanProfile - fan profile to enable
 * @param reserved1
 * @param performanceMode - Performance/Acoustic mode
 * @param reserved2
 * @param setPerformanceMode - set Performance/Acoustic mode
 * @param setFanProfile - set fan profile
 *
 * @return IPMI completion code.
 **/
ipmi::RspType<> ipmiOEMSetFanConfig(uint8_t selectedFanProfile,

                                    uint2_t reserved1, bool performanceMode,
                                    uint3_t reserved2, bool setPerformanceMode,
                                    bool setFanProfile,
                                    std::optional<uint8_t> dimmGroupId,
                                    std::optional<uint32_t> dimmPresenceBitmap)
{
    if (reserved1 || reserved2)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    if (dimmGroupId)
    {
        if (*dimmGroupId >= maxCPUNum)
        {
            return ipmi::responseInvalidFieldRequest();
        }
        if (!cpuPresent("CPU_" + std::to_string(*dimmGroupId + 1)))
        {
            return ipmi::responseInvalidFieldRequest();
        }
    }

    // todo: tell bios to only send first 2 bytes
    boost::container::flat_map<
        std::string, std::variant<std::vector<std::string>, std::string>>
        profileData;
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    if (!getFanProfileInterface(*dbus, profileData))
    {
        return ipmi::responseUnspecifiedError();
    }

    std::vector<std::string>* supported =
        std::get_if<std::vector<std::string>>(&profileData["Supported"]);
    if (supported == nullptr)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    std::string mode;
    if (setPerformanceMode)
    {
        if (performanceMode)
        {

            if (std::find(supported->begin(), supported->end(),
                          "Performance") != supported->end())
            {
                mode = "Performance";
            }
        }
        else
        {
            if (std::find(supported->begin(), supported->end(), "Acoustic") !=
                supported->end())
            {
                mode = "Acoustic";
            }
        }
        if (mode.empty())
        {
            return ipmi::responseInvalidFieldRequest();
        }

        try
        {
            setDbusProperty(*dbus, settingsBusName, thermalModePath,
                            thermalModeInterface, "Current", mode);
        }
        catch (sdbusplus::exception_t& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMSetFanConfig: can't set thermal mode!",
                phosphor::logging::entry("EXCEPTION=%s", e.what()));
            return ipmi::responseResponseError();
        }
    }

    return ipmi::responseSuccess();
}

ipmi::RspType<uint8_t, // profile support map
              uint8_t, // fan control profile enable
              uint8_t, // flags
              uint32_t // dimm presence bit map
              >
    ipmiOEMGetFanConfig(uint8_t dimmGroupId)
{
    if (dimmGroupId >= maxCPUNum)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    bool cpuStatus = cpuPresent("CPU_" + std::to_string(dimmGroupId + 1));

    if (!cpuStatus)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    boost::container::flat_map<
        std::string, std::variant<std::vector<std::string>, std::string>>
        profileData;

    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    if (!getFanProfileInterface(*dbus, profileData))
    {
        return ipmi::responseResponseError();
    }

    std::string* current = std::get_if<std::string>(&profileData["Current"]);

    if (current == nullptr)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMGetFanConfig: can't get current mode!");
        return ipmi::responseResponseError();
    }
    bool performance = (*current == "Performance");

    uint8_t flags = 0;
    if (performance)
    {
        flags |= 1 << 2;
    }

    constexpr uint8_t fanControlDefaultProfile = 0x80;
    constexpr uint8_t fanControlProfileState = 0x00;
    constexpr uint32_t dimmPresenceBitmap = 0x00;

    return ipmi::responseSuccess(fanControlDefaultProfile,
                                 fanControlProfileState, flags,
                                 dimmPresenceBitmap);
}
constexpr const char* cfmLimitSettingPath =
    "/xyz/openbmc_project/control/cfm_limit";
constexpr const char* cfmLimitIface = "xyz.openbmc_project.Control.CFMLimit";
constexpr const size_t legacyExitAirSensorNumber = 0x2e;
constexpr const size_t legacyPCHSensorNumber = 0x22;
constexpr const char* exitAirPathName = "Exit_Air";
constexpr const char* pchPathName = "SSB_Temp";
constexpr const char* pidConfigurationIface =
    "xyz.openbmc_project.Configuration.Pid";

static std::string getConfigPath(const std::string& name)
{
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    auto method =
        dbus->new_method_call("xyz.openbmc_project.ObjectMapper",
                              "/xyz/openbmc_project/object_mapper",
                              "xyz.openbmc_project.ObjectMapper", "GetSubTree");

    method.append("/", 0, std::array<const char*, 1>{pidConfigurationIface});
    std::string path;
    GetSubTreeType resp;
    try
    {
        auto reply = dbus->call(method);
        reply.read(resp);
    }
    catch (sdbusplus::exception_t&)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMGetFscParameter: mapper error");
    };
    auto config =
        std::find_if(resp.begin(), resp.end(), [&name](const auto& pair) {
            return pair.first.find(name) != std::string::npos;
        });
    if (config != resp.end())
    {
        path = std::move(config->first);
    }
    return path;
}

// flat map to make alphabetical
static boost::container::flat_map<std::string, PropertyMap> getPidConfigs()
{
    boost::container::flat_map<std::string, PropertyMap> ret;
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    auto method =
        dbus->new_method_call("xyz.openbmc_project.ObjectMapper",
                              "/xyz/openbmc_project/object_mapper",
                              "xyz.openbmc_project.ObjectMapper", "GetSubTree");

    method.append("/", 0, std::array<const char*, 1>{pidConfigurationIface});
    GetSubTreeType resp;

    try
    {
        auto reply = dbus->call(method);
        reply.read(resp);
    }
    catch (sdbusplus::exception_t&)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "getFanConfigPaths: mapper error");
    };
    for (const auto& [path, objects] : resp)
    {
        if (objects.empty())
        {
            continue; // should be impossible
        }

        try
        {
            ret.emplace(path,
                        getAllDbusProperties(*dbus, objects[0].first, path,
                                             pidConfigurationIface));
        }
        catch (sdbusplus::exception_t& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "getPidConfigs: can't get DbusProperties!",
                phosphor::logging::entry("ERR=%s", e.what()));
        }
    }
    return ret;
}

ipmi::RspType<uint8_t> ipmiOEMGetFanSpeedOffset(void)
{
    boost::container::flat_map<std::string, PropertyMap> data = getPidConfigs();
    if (data.empty())
    {
        return ipmi::responseResponseError();
    }
    uint8_t minOffset = std::numeric_limits<uint8_t>::max();
    for (const auto& [_, pid] : data)
    {
        auto findClass = pid.find("Class");
        if (findClass == pid.end())
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMGetFscParameter: found illegal pid "
                "configurations");
            return ipmi::responseResponseError();
        }
        std::string type = std::get<std::string>(findClass->second);
        if (type == "fan")
        {
            auto findOutLimit = pid.find("OutLimitMin");
            if (findOutLimit == pid.end())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "ipmiOEMGetFscParameter: found illegal pid "
                    "configurations");
                return ipmi::responseResponseError();
            }
            // get the min out of all the offsets
            minOffset = std::min(
                minOffset,
                static_cast<uint8_t>(std::get<double>(findOutLimit->second)));
        }
    }
    if (minOffset == std::numeric_limits<uint8_t>::max())
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMGetFscParameter: found no fan configurations!");
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(minOffset);
}

ipmi::RspType<> ipmiOEMSetFanSpeedOffset(uint8_t offset)
{
    boost::container::flat_map<std::string, PropertyMap> data = getPidConfigs();
    if (data.empty())
    {

        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMSetFanSpeedOffset: found no pid configurations!");
        return ipmi::responseResponseError();
    }

    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    bool found = false;
    for (const auto& [path, pid] : data)
    {
        auto findClass = pid.find("Class");
        if (findClass == pid.end())
        {

            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMSetFanSpeedOffset: found illegal pid "
                "configurations");
            return ipmi::responseResponseError();
        }
        std::string type = std::get<std::string>(findClass->second);
        if (type == "fan")
        {
            auto findOutLimit = pid.find("OutLimitMin");
            if (findOutLimit == pid.end())
            {

                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "ipmiOEMSetFanSpeedOffset: found illegal pid "
                    "configurations");
                return ipmi::responseResponseError();
            }
            ipmi::setDbusProperty(*dbus, "xyz.openbmc_project.EntityManager",
                                  path, pidConfigurationIface, "OutLimitMin",
                                  static_cast<double>(offset));
            found = true;
        }
    }
    if (!found)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMSetFanSpeedOffset: set no fan offsets");
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess();
}

ipmi::RspType<> ipmiOEMSetFscParameter(uint8_t command, uint8_t param1,
                                       uint8_t param2)
{
    constexpr const size_t disableLimiting = 0x0;

    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    if (command == static_cast<uint8_t>(setFscParamFlags::tcontrol))
    {
        std::string pathName;
        if (param1 == legacyExitAirSensorNumber)
        {
            pathName = exitAirPathName;
        }
        else if (param1 == legacyPCHSensorNumber)
        {
            pathName = pchPathName;
        }
        else
        {
            return ipmi::responseParmOutOfRange();
        }
        std::string path = getConfigPath(pathName);
        ipmi::setDbusProperty(*dbus, "xyz.openbmc_project.EntityManager", path,
                              pidConfigurationIface, "SetPoint",
                              static_cast<double>(param2));
        return ipmi::responseSuccess();
    }
    else if (command == static_cast<uint8_t>(setFscParamFlags::cfm))
    {
        uint16_t cfm = param1 | (static_cast<uint16_t>(param2) << 8);

        // must be greater than 50 based on eps
        if (cfm < 50 && cfm != disableLimiting)
        {
            return ipmi::responseParmOutOfRange();
        }

        try
        {
            ipmi::setDbusProperty(*dbus, settingsBusName, cfmLimitSettingPath,
                                  cfmLimitIface, "Limit",
                                  static_cast<double>(cfm));
        }
        catch (sdbusplus::exception_t& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMSetFscParameter: can't set cfm setting!",
                phosphor::logging::entry("ERR=%s", e.what()));
            return ipmi::responseResponseError();
        }
        return ipmi::responseSuccess();
    }
    else if (command == static_cast<uint8_t>(setFscParamFlags::maxPwm))
    {
        constexpr const size_t maxDomainCount = 8;
        uint8_t requestedDomainMask = param1;
        boost::container::flat_map data = getPidConfigs();
        if (data.empty())
        {

            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMSetFscParameter: found no pid configurations!");
            return ipmi::responseResponseError();
        }
        size_t count = 0;
        for (const auto& [path, pid] : data)
        {
            auto findClass = pid.find("Class");
            if (findClass == pid.end())
            {

                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "ipmiOEMSetFscParameter: found illegal pid "
                    "configurations");
                return ipmi::responseResponseError();
            }
            std::string type = std::get<std::string>(findClass->second);
            if (type == "fan")
            {
                if (requestedDomainMask & (1 << count))
                {
                    ipmi::setDbusProperty(
                        *dbus, "xyz.openbmc_project.EntityManager", path,
                        pidConfigurationIface, "OutLimitMax",
                        static_cast<double>(param2));
                }
                count++;
            }
        }
        return ipmi::responseSuccess();
    }
    else
    {
        // todo other command parts possibly
        // tcontrol is handled in peci now
        // fan speed offset not implemented yet
        // domain pwm limit not implemented
        return ipmi::responseParmOutOfRange();
    }
}

ipmi::RspType<
    std::variant<uint8_t, std::array<uint8_t, 2>, std::array<uint16_t, 2>>>
    ipmiOEMGetFscParameter(uint8_t command, std::optional<uint8_t> param)
{
    constexpr uint8_t legacyDefaultSetpoint = -128;

    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    if (command == static_cast<uint8_t>(setFscParamFlags::tcontrol))
    {
        if (!param)
        {
            return ipmi::responseReqDataLenInvalid();
        }

        std::string pathName;

        if (*param == legacyExitAirSensorNumber)
        {
            pathName = exitAirPathName;
        }
        else if (*param == legacyPCHSensorNumber)
        {
            pathName = pchPathName;
        }
        else
        {
            return ipmi::responseParmOutOfRange();
        }

        uint8_t setpoint = legacyDefaultSetpoint;
        std::string path = getConfigPath(pathName);
        if (path.size())
        {
            Value val = ipmi::getDbusProperty(
                *dbus, "xyz.openbmc_project.EntityManager", path,
                pidConfigurationIface, "SetPoint");
            setpoint = std::floor(std::get<double>(val) + 0.5);
        }

        // old implementation used to return the "default" and current, we
        // don't make the default readily available so just make both the
        // same

        return ipmi::responseSuccess(
            std::array<uint8_t, 2>{setpoint, setpoint});
    }
    else if (command == static_cast<uint8_t>(setFscParamFlags::maxPwm))
    {
        constexpr const size_t maxDomainCount = 8;

        if (!param)
        {
            return ipmi::responseReqDataLenInvalid();
        }
        uint8_t requestedDomain = *param;
        if (requestedDomain >= maxDomainCount)
        {
            return ipmi::responseInvalidFieldRequest();
        }

        boost::container::flat_map data = getPidConfigs();
        if (data.empty())
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMGetFscParameter: found no pid configurations!");
            return ipmi::responseResponseError();
        }
        size_t count = 0;
        for (const auto& [_, pid] : data)
        {
            auto findClass = pid.find("Class");
            if (findClass == pid.end())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "ipmiOEMGetFscParameter: found illegal pid "
                    "configurations");
                return ipmi::responseResponseError();
            }
            std::string type = std::get<std::string>(findClass->second);
            if (type == "fan")
            {
                if (requestedDomain == count)
                {
                    auto findOutLimit = pid.find("OutLimitMax");
                    if (findOutLimit == pid.end())
                    {
                        phosphor::logging::log<phosphor::logging::level::ERR>(
                            "ipmiOEMGetFscParameter: found illegal pid "
                            "configurations");
                        return ipmi::responseResponseError();
                    }

                    return ipmi::responseSuccess(
                        static_cast<uint8_t>(std::floor(
                            std::get<double>(findOutLimit->second) + 0.5)));
                }
                else
                {
                    count++;
                }
            }
        }

        return ipmi::responseInvalidFieldRequest();
    }
    else if (command == static_cast<uint8_t>(setFscParamFlags::cfm))
    {

        /*
        DataLen should be 1, but host is sending us an extra bit. As the
        previous behavior didn't seem to prevent this, ignore the check for
        now.

        if (param)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMGetFscParameter: invalid input len!");
            return IPMI_CC_REQ_DATA_LEN_INVALID;
        }
        */
        Value cfmLimit;
        Value cfmMaximum;
        try
        {
            cfmLimit = ipmi::getDbusProperty(*dbus, settingsBusName,
                                             cfmLimitSettingPath, cfmLimitIface,
                                             "Limit");
            cfmMaximum = ipmi::getDbusProperty(
                *dbus, "xyz.openbmc_project.ExitAirTempSensor",
                "/xyz/openbmc_project/control/MaxCFM", cfmLimitIface, "Limit");
        }
        catch (sdbusplus::exception_t& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMGetFscParameter: can't get cfm setting!",
                phosphor::logging::entry("ERR=%s", e.what()));
            return ipmi::responseResponseError();
        }

        double cfmMax = std::get<double>(cfmMaximum);
        double cfmLim = std::get<double>(cfmLimit);

        cfmLim = std::floor(cfmLim + 0.5);
        cfmMax = std::floor(cfmMax + 0.5);
        uint16_t cfmLimResp = static_cast<uint16_t>(cfmLim);
        uint16_t cfmMaxResp = static_cast<uint16_t>(cfmMax);

        return ipmi::responseSuccess(
            std::array<uint16_t, 2>{cfmLimResp, cfmMaxResp});
    }

    else
    {
        // todo other command parts possibly
        // domain pwm limit not implemented
        return ipmi::responseParmOutOfRange();
    }
}

using crConfigVariant =
    std::variant<bool, uint8_t, uint32_t, std::vector<uint8_t>, std::string>;

int setCRConfig(ipmi::Context::ptr ctx, const std::string& property,
                const crConfigVariant& value,
                std::chrono::microseconds timeout = ipmi::IPMI_DBUS_TIMEOUT)
{
    boost::system::error_code ec;
    ctx->bus->yield_method_call<void>(
        ctx->yield, ec, "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/power_supply_redundancy",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Control.PowerSupplyRedundancy", property, value);
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to set dbus property to cold redundancy");
        return -1;
    }

    return 0;
}

int getCRConfig(ipmi::Context::ptr ctx, const std::string& property,
                crConfigVariant& value,
                const std::string& service = "xyz.openbmc_project.Settings",
                std::chrono::microseconds timeout = ipmi::IPMI_DBUS_TIMEOUT)
{
    boost::system::error_code ec;
    value = ctx->bus->yield_method_call<crConfigVariant>(
        ctx->yield, ec, service,
        "/xyz/openbmc_project/control/power_supply_redundancy",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.PowerSupplyRedundancy", property);
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get dbus property to cold redundancy");
        return -1;
    }
    return 0;
}

uint8_t getPSUCount(void)
{
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    ipmi::Value num;
    try
    {
        num = ipmi::getDbusProperty(
            *dbus, "xyz.openbmc_project.PSURedundancy",
            "/xyz/openbmc_project/control/power_supply_redundancy",
            "xyz.openbmc_project.Control.PowerSupplyRedundancy", "PSUNumber");
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get PSUNumber property from dbus interface");
        return 0;
    }
    uint8_t* pNum = std::get_if<uint8_t>(&num);
    if (!pNum)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error to get PSU Number");
        return 0;
    }
    return *pNum;
}

bool validateCRAlgo(std::vector<uint8_t>& conf, uint8_t num)
{
    if (conf.size() < num)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid PSU Ranking");
        return false;
    }
    std::set<uint8_t> confSet;
    for (uint8_t i = 0; i < num; i++)
    {
        if (conf[i] > num)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "PSU Ranking is larger than current PSU number");
            return false;
        }
        confSet.emplace(conf[i]);
    }

    if (confSet.size() != num)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "duplicate PSU Ranking");
        return false;
    }
    return true;
}

enum class crParameter
{
    crStatus = 0,
    crFeature = 1,
    rotationFeature = 2,
    rotationAlgo = 3,
    rotationPeriod = 4,
    numOfPSU = 5,
    rotationRankOrderEffective = 6
};

constexpr ipmi::Cc ccParameterNotSupported = 0x80;
static const constexpr uint32_t oneDay = 0x15180;
static const constexpr uint32_t oneMonth = 0xf53700;
static const constexpr uint8_t userSpecific = 0x01;
static const constexpr uint8_t crSetCompleted = 0;
ipmi::RspType<uint8_t> ipmiOEMSetCRConfig(ipmi::Context::ptr ctx,
                                          uint8_t parameter,
                                          ipmi::message::Payload& payload)
{
    switch (static_cast<crParameter>(parameter))
    {
        case crParameter::crFeature:
        {
            uint8_t param1;
            if (payload.unpack(param1) || !payload.fullyUnpacked())
            {
                return ipmi::responseReqDataLenInvalid();
            }
            // ColdRedundancy Enable can only be true or flase
            if (param1 > 1)
            {
                return ipmi::responseInvalidFieldRequest();
            }
            if (setCRConfig(ctx, "ColdRedundancyEnabled",
                            static_cast<bool>(param1)))
            {
                return ipmi::responseResponseError();
            }
            break;
        }
        case crParameter::rotationFeature:
        {
            uint8_t param1;
            if (payload.unpack(param1) || !payload.fullyUnpacked())
            {
                return ipmi::responseReqDataLenInvalid();
            }
            // Rotation Enable can only be true or false
            if (param1 > 1)
            {
                return ipmi::responseInvalidFieldRequest();
            }
            if (setCRConfig(ctx, "RotationEnabled", static_cast<bool>(param1)))
            {
                return ipmi::responseResponseError();
            }
            break;
        }
        case crParameter::rotationAlgo:
        {
            // Rotation Algorithm can only be 0-BMC Specific or 1-User Specific
            std::string algoName;
            uint8_t param1;
            if (payload.unpack(param1))
            {
                return ipmi::responseReqDataLenInvalid();
            }
            switch (param1)
            {
                case 0:
                    algoName = "xyz.openbmc_project.Control."
                               "PowerSupplyRedundancy.Algo.bmcSpecific";
                    break;
                case 1:
                    algoName = "xyz.openbmc_project.Control."
                               "PowerSupplyRedundancy.Algo.userSpecific";
                    break;
                default:
                    return ipmi::responseInvalidFieldRequest();
            }
            if (setCRConfig(ctx, "RotationAlgorithm", algoName))
            {
                return ipmi::responseResponseError();
            }

            uint8_t numberOfPSU = getPSUCount();
            if (!numberOfPSU)
            {
                return ipmi::responseResponseError();
            }
            std::vector<uint8_t> rankOrder;

            if (param1 == userSpecific)
            {
                if (payload.unpack(rankOrder) || !payload.fullyUnpacked())
                {
                    ipmi::responseReqDataLenInvalid();
                }
                if (rankOrder.size() != numberOfPSU)
                {
                    return ipmi::responseReqDataLenInvalid();
                }

                if (!validateCRAlgo(rankOrder, numberOfPSU))
                {
                    return ipmi::responseInvalidFieldRequest();
                }
            }
            else
            {
                if (rankOrder.size() > 0)
                {
                    return ipmi::responseReqDataLenInvalid();
                }
                for (uint8_t i = 1; i <= numberOfPSU; i++)
                {
                    rankOrder.emplace_back(i);
                }
            }
            if (setCRConfig(ctx, "RotationRankOrder", rankOrder))
            {
                return ipmi::responseResponseError();
            }
            break;
        }
        case crParameter::rotationPeriod:
        {
            // Minimum Rotation period is  One day (86400 seconds) and Max
            // Rotation Period is 6 month (0xf53700 seconds)
            uint32_t period;
            if (payload.unpack(period) || !payload.fullyUnpacked())
            {
                return ipmi::responseReqDataLenInvalid();
            }
            if ((period < oneDay) || (period > oneMonth))
            {
                return ipmi::responseInvalidFieldRequest();
            }
            if (setCRConfig(ctx, "PeriodOfRotation", period))
            {
                return ipmi::responseResponseError();
            }
            break;
        }
        default:
        {
            return ipmi::response(ccParameterNotSupported);
        }
    }

    // TODO Halfwidth needs to set SetInProgress
    if (setCRConfig(ctx, "ColdRedundancyStatus",
                    std::string("xyz.openbmc_project.Control."
                                "PowerSupplyRedundancy.Status.completed")))
    {
        return ipmi::responseResponseError();
    }
    return ipmi::responseSuccess(crSetCompleted);
}

ipmi::RspType<uint8_t, std::variant<uint8_t, uint32_t, std::vector<uint8_t>>>
    ipmiOEMGetCRConfig(ipmi::Context::ptr ctx, uint8_t parameter)
{
    crConfigVariant value;
    switch (static_cast<crParameter>(parameter))
    {
        case crParameter::crStatus:
        {
            if (getCRConfig(ctx, "ColdRedundancyStatus", value))
            {
                return ipmi::responseResponseError();
            }
            std::string* pStatus = std::get_if<std::string>(&value);
            if (!pStatus)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error to get ColdRedundancyStatus property");
                return ipmi::responseResponseError();
            }
            namespace server = sdbusplus::xyz::openbmc_project::Control::server;
            auto status =
                server::PowerSupplyRedundancy::convertStatusFromString(
                    *pStatus);
            switch (status)
            {
                case server::PowerSupplyRedundancy::Status::inProgress:
                    return ipmi::responseSuccess(parameter,
                                                 static_cast<uint8_t>(0));

                case server::PowerSupplyRedundancy::Status::completed:
                    return ipmi::responseSuccess(parameter,
                                                 static_cast<uint8_t>(1));
                default:
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "Error to get valid status");
                    return ipmi::responseResponseError();
            }
        }
        case crParameter::crFeature:
        {
            if (getCRConfig(ctx, "ColdRedundancyEnabled", value))
            {
                return ipmi::responseResponseError();
            }
            bool* pResponse = std::get_if<bool>(&value);
            if (!pResponse)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error to get ColdRedundancyEnable property");
                return ipmi::responseResponseError();
            }

            return ipmi::responseSuccess(parameter,
                                         static_cast<uint8_t>(*pResponse));
        }
        case crParameter::rotationFeature:
        {
            if (getCRConfig(ctx, "RotationEnabled", value))
            {
                return ipmi::responseResponseError();
            }
            bool* pResponse = std::get_if<bool>(&value);
            if (!pResponse)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error to get RotationEnabled property");
                return ipmi::responseResponseError();
            }
            return ipmi::responseSuccess(parameter,
                                         static_cast<uint8_t>(*pResponse));
        }
        case crParameter::rotationAlgo:
        {
            if (getCRConfig(ctx, "RotationAlgorithm", value))
            {
                return ipmi::responseResponseError();
            }

            std::string* pAlgo = std::get_if<std::string>(&value);
            if (!pAlgo)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error to get RotationAlgorithm property");
                return ipmi::responseResponseError();
            }
            std::vector<uint8_t> response;
            namespace server = sdbusplus::xyz::openbmc_project::Control::server;
            auto algo =
                server::PowerSupplyRedundancy::convertAlgoFromString(*pAlgo);

            switch (algo)
            {
                case server::PowerSupplyRedundancy::Algo::bmcSpecific:
                    response.push_back(0);
                    break;
                case server::PowerSupplyRedundancy::Algo::userSpecific:
                    response.push_back(1);
                    break;
                default:
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "Error to get valid algo");
                    return ipmi::responseResponseError();
            }

            if (getCRConfig(ctx, "RotationRankOrder", value))
            {
                return ipmi::responseResponseError();
            }
            std::vector<uint8_t>* pResponse =
                std::get_if<std::vector<uint8_t>>(&value);
            if (!pResponse)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error to get RotationRankOrder property");
                return ipmi::responseResponseError();
            }

            std::copy(pResponse->begin(), pResponse->end(),
                      std::back_inserter(response));

            return ipmi::responseSuccess(parameter, response);
        }
        case crParameter::rotationPeriod:
        {
            if (getCRConfig(ctx, "PeriodOfRotation", value))
            {
                return ipmi::responseResponseError();
            }
            uint32_t* pResponse = std::get_if<uint32_t>(&value);
            if (!pResponse)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error to get RotationAlgorithm property");
                return ipmi::responseResponseError();
            }
            return ipmi::responseSuccess(parameter, *pResponse);
        }
        case crParameter::numOfPSU:
        {
            uint8_t numberOfPSU = getPSUCount();
            if (!numberOfPSU)
            {
                return ipmi::responseResponseError();
            }
            return ipmi::responseSuccess(parameter, numberOfPSU);
        }
        case crParameter::rotationRankOrderEffective:
        {
            if (getCRConfig(ctx, "RotationRankOrder", value,
                            "xyz.openbmc_project.PSURedundancy"))
            {
                return ipmi::responseResponseError();
            }
            std::vector<uint8_t>* pResponse =
                std::get_if<std::vector<uint8_t>>(&value);
            if (!pResponse)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error to get effective RotationRankOrder property");
                return ipmi::responseResponseError();
            }
            return ipmi::responseSuccess(parameter, *pResponse);
        }
        default:
        {
            return ipmi::response(ccParameterNotSupported);
        }
    }
}

ipmi::RspType<> ipmiOEMSetFaultIndication(uint8_t sourceId, uint8_t faultType,
                                          uint8_t faultState,
                                          uint8_t faultGroup,
                                          std::array<uint8_t, 8>& ledStateData)
{
    constexpr auto maxFaultType = static_cast<size_t>(RemoteFaultType::max);
    static const std::array<std::string, maxFaultType> faultNames = {
        "faultFan",       "faultTemp",     "faultPower",
        "faultDriveSlot", "faultSoftware", "faultMemory"};

    constexpr uint8_t maxFaultSource = 0x4;
    constexpr uint8_t skipLEDs = 0xFF;
    constexpr uint8_t pinSize = 64;
    constexpr uint8_t groupSize = 16;
    constexpr uint8_t groupNum = 5; // 4 for fault memory, 1 for faultFan

    // same pin names need to be defined in dts file
    static const std::array<std::array<std::string, groupSize>, groupNum>
        faultLedPinNames = {{
            "LED_CPU1_CH1_DIMM1_FAULT",
            "LED_CPU1_CH1_DIMM2_FAULT",
            "LED_CPU1_CH2_DIMM1_FAULT",
            "LED_CPU1_CH2_DIMM2_FAULT",
            "LED_CPU1_CH3_DIMM1_FAULT",
            "LED_CPU1_CH3_DIMM2_FAULT",
            "LED_CPU1_CH4_DIMM1_FAULT",
            "LED_CPU1_CH4_DIMM2_FAULT",
            "LED_CPU1_CH5_DIMM1_FAULT",
            "LED_CPU1_CH5_DIMM2_FAULT",
            "LED_CPU1_CH6_DIMM1_FAULT",
            "LED_CPU1_CH6_DIMM2_FAULT",
            "",
            "",
            "",
            "", // end of group1
            "LED_CPU2_CH1_DIMM1_FAULT",
            "LED_CPU2_CH1_DIMM2_FAULT",
            "LED_CPU2_CH2_DIMM1_FAULT",
            "LED_CPU2_CH2_DIMM2_FAULT",
            "LED_CPU2_CH3_DIMM1_FAULT",
            "LED_CPU2_CH3_DIMM2_FAULT",
            "LED_CPU2_CH4_DIMM1_FAULT",
            "LED_CPU2_CH4_DIMM2_FAULT",
            "LED_CPU2_CH5_DIMM1_FAULT",
            "LED_CPU2_CH5_DIMM2_FAULT",
            "LED_CPU2_CH6_DIMM1_FAULT",
            "LED_CPU2_CH6_DIMM2_FAULT",
            "",
            "",
            "",
            "", // endof group2
            "LED_CPU3_CH1_DIMM1_FAULT",
            "LED_CPU3_CH1_DIMM2_FAULT",
            "LED_CPU3_CH2_DIMM1_FAULT",
            "LED_CPU3_CH2_DIMM2_FAULT",
            "LED_CPU3_CH3_DIMM1_FAULT",
            "LED_CPU3_CH3_DIMM2_FAULT",
            "LED_CPU3_CH4_DIMM1_FAULT",
            "LED_CPU3_CH4_DIMM2_FAULT",
            "LED_CPU3_CH5_DIMM1_FAULT",
            "LED_CPU3_CH5_DIMM2_FAULT",
            "LED_CPU3_CH6_DIMM1_FAULT",
            "LED_CPU3_CH6_DIMM2_FAULT",
            "",
            "",
            "",
            "", // end of group3
            "LED_CPU4_CH1_DIMM1_FAULT",
            "LED_CPU4_CH1_DIMM2_FAULT",
            "LED_CPU4_CH2_DIMM1_FAULT",
            "LED_CPU4_CH2_DIMM2_FAULT",
            "LED_CPU4_CH3_DIMM1_FAULT",
            "LED_CPU4_CH3_DIMM2_FAULT",
            "LED_CPU4_CH4_DIMM1_FAULT",
            "LED_CPU4_CH4_DIMM2_FAULT",
            "LED_CPU4_CH5_DIMM1_FAULT",
            "LED_CPU4_CH5_DIMM2_FAULT",
            "LED_CPU4_CH6_DIMM1_FAULT",
            "LED_CPU4_CH6_DIMM2_FAULT",
            "",
            "",
            "",
            "", // end of group4
            "LED_FAN1_FAULT",
            "LED_FAN2_FAULT",
            "LED_FAN3_FAULT",
            "LED_FAN4_FAULT",
            "LED_FAN5_FAULT",
            "LED_FAN6_FAULT",
            "LED_FAN7_FAULT",
            "LED_FAN8_FAULT",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "" // end of group5
        }};

    // Validate the source, fault type --
    // (Byte 1) sourceId: Unspecified, Hot-Swap Controller 0, Hot-Swap
    // Controller 1, BIOS (Byte 2) fault type: fan, temperature, power,
    // driveslot, software, memory (Byte 3) FaultState: OK, Degraded,
    // Non-Critical, Critical, Non-Recoverable, (Byte 4) is faultGroup,
    // definition differs based on fault type (Byte 2)
    //          Type Fan=> Group: 0=FanGroupID, FF-not used
    //                  Byte 5-11 00h, not used
    //                  Byte12 FanLedState [7:0]-Fans 7:0
    //          Type Memory=> Group: 0 = DIMM GroupID, FF-not used
    //                  Byte 5:12 - DIMM LED state (64bit field, LS Byte first)
    //                  [63:48] = CPU4 channels 7:0, 2 bits per channel
    //                  [47:32] = CPU3 channels 7:0, 2 bits per channel
    //                  [31:16] = CPU2 channels 7:0, 2 bits per channel
    //                  [15:0] =  CPU1 channels 7:0, 2 bits per channel
    //          Type Other=> Component Fault LED Group ID, not used set to 0xFF
    //                  Byte[5:12]: reserved 0x00h
    if ((sourceId >= maxFaultSource) ||
        (faultType >= static_cast<int8_t>(RemoteFaultType::max)) ||
        (faultState >= static_cast<int8_t>(RemoteFaultState::maxFaultState)) ||
        (faultGroup >= static_cast<int8_t>(DimmFaultType::maxFaultGroup)))
    {
        return ipmi::responseParmOutOfRange();
    }

    size_t pinGroupOffset = 0;
    size_t pinGroupMax = pinSize / groupSize;
    if (RemoteFaultType::fan == RemoteFaultType(faultType))
    {
        pinGroupOffset = 4;
        pinGroupMax = groupNum - pinSize / groupSize;
    }

    switch (RemoteFaultType(faultType))
    {
        case (RemoteFaultType::fan):
        case (RemoteFaultType::memory):
        {
            if (faultGroup == skipLEDs)
            {
                return ipmi::responseSuccess();
            }
            // calculate led state bit filed count, each byte has 8bits
            // the maximum bits will be 8 * 8 bits
            constexpr uint8_t size = sizeof(ledStateData) * 8;

            // assemble ledState
            uint64_t ledState = 0;
            bool hasError = false;
            for (int i = 0; i < sizeof(ledStateData); i++)
            {
                ledState = (uint64_t)(ledState << 8);
                ledState = (uint64_t)(ledState | (uint64_t)ledStateData[i]);
            }
            std::bitset<size> ledStateBits(ledState);

            for (int group = 0; group < pinGroupMax; group++)
            {
                for (int i = 0; i < groupSize; i++)
                { // skip non-existing pins
                    if (0 == faultLedPinNames[group + pinGroupOffset][i].size())
                    {
                        continue;
                    }

                    gpiod::line line = gpiod::find_line(
                        faultLedPinNames[group + pinGroupOffset][i]);
                    if (!line)
                    {
                        phosphor::logging::log<phosphor::logging::level::ERR>(
                            "Not Find Led Gpio Device!",
                            phosphor::logging::entry(
                                "DEVICE=%s",
                                faultLedPinNames[group + pinGroupOffset][i]
                                    .c_str()));
                        hasError = true;
                        continue;
                    }

                    bool activeHigh =
                        (line.active_state() == gpiod::line::ACTIVE_HIGH);
                    try
                    {
                        line.request(
                            {"faultLed", gpiod::line_request::DIRECTION_OUTPUT,
                             activeHigh
                                 ? 0
                                 : gpiod::line_request::FLAG_ACTIVE_LOW});
                        line.set_value(ledStateBits[i + group * groupSize]);
                    }
                    catch (std::system_error&)
                    {
                        phosphor::logging::log<phosphor::logging::level::ERR>(
                            "Error write Led Gpio Device!",
                            phosphor::logging::entry(
                                "DEVICE=%s",
                                faultLedPinNames[group + pinGroupOffset][i]
                                    .c_str()));
                        hasError = true;
                        continue;
                    }
                } // for int i
            }
            if (hasError)
            {
                return ipmi::responseResponseError();
            }
            break;
        }
        default:
        {
            // now only support two fault types
            return ipmi::responseParmOutOfRange();
        }
    } // switch
    return ipmi::responseSuccess();
}

ipmi::RspType<uint8_t> ipmiOEMReadBoardProductId()
{
    uint8_t prodId = 0;
    try
    {
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        const DbusObjectInfo& object = getDbusObject(
            *dbus, "xyz.openbmc_project.Inventory.Item.Board",
            "/xyz/openbmc_project/inventory/system/board/", "Baseboard");
        const Value& propValue = getDbusProperty(
            *dbus, object.second, object.first,
            "xyz.openbmc_project.Inventory.Item.Board.Motherboard",
            "ProductId");
        prodId = static_cast<uint8_t>(std::get<uint64_t>(propValue));
    }
    catch (std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiOEMReadBoardProductId: Product ID read failed!",
            phosphor::logging::entry("ERR=%s", e.what()));
    }
    return ipmi::responseSuccess(prodId);
}

/** @brief implements the get security mode command
 *  @param ctx - ctx pointer
 *
 *  @returns IPMI completion code with following data
 *   - restriction mode value - As specified in
 * xyz.openbmc_project.Control.Security.RestrictionMode.interface.yaml
 *   - special mode value - As specified in
 * xyz.openbmc_project.Control.Security.SpecialMode.interface.yaml
 */
ipmi::RspType<uint8_t, uint8_t> ipmiGetSecurityMode(ipmi::Context::ptr ctx)
{
    namespace securityNameSpace =
        sdbusplus::xyz::openbmc_project::Control::Security::server;
    uint8_t restrictionModeValue = 0;
    uint8_t specialModeValue = 0;

    boost::system::error_code ec;
    auto varRestrMode = ctx->bus->yield_method_call<std::variant<std::string>>(
        ctx->yield, ec, restricionModeService, restricionModeBasePath,
        dBusPropertyIntf, dBusPropertyGetMethod, restricionModeIntf,
        restricionModeProperty);
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiGetSecurityMode: failed to get RestrictionMode property",
            phosphor::logging::entry("ERROR=%s", ec.message().c_str()));
        return ipmi::responseUnspecifiedError();
    }
    restrictionModeValue = static_cast<uint8_t>(
        securityNameSpace::RestrictionMode::convertModesFromString(
            std::get<std::string>(varRestrMode)));
    auto varSpecialMode =
        ctx->bus->yield_method_call<std::variant<std::string>>(
            ctx->yield, ec, specialModeService, specialModeBasePath,
            dBusPropertyIntf, dBusPropertyGetMethod, specialModeIntf,
            specialModeProperty);
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiGetSecurityMode: failed to get SpecialMode property",
            phosphor::logging::entry("ERROR=%s", ec.message().c_str()));
        // fall through, let us not worry about SpecialMode property, which is
        // not required in user scenario
    }
    else
    {
        specialModeValue = static_cast<uint8_t>(
            securityNameSpace::SpecialMode::convertModesFromString(
                std::get<std::string>(varSpecialMode)));
    }
    return ipmi::responseSuccess(restrictionModeValue, specialModeValue);
}

/** @brief implements the set security mode command
 *  Command allows to upgrade the restriction mode and won't allow
 *  to downgrade from system interface
 *  @param ctx - ctx pointer
 *  @param restrictionMode - restriction mode value to be set.
 *
 *  @returns IPMI completion code
 */
ipmi::RspType<> ipmiSetSecurityMode(ipmi::Context::ptr ctx,
                                    uint8_t restrictionMode,
                                    std::optional<uint8_t> specialMode)
{
#ifndef BMC_VALIDATION_UNSECURE_FEATURE
    if (specialMode)
    {
        return ipmi::responseReqDataLenInvalid();
    }
#endif
    namespace securityNameSpace =
        sdbusplus::xyz::openbmc_project::Control::Security::server;

    ChannelInfo chInfo;
    if (getChannelInfo(ctx->channel, chInfo) != ccSuccess)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiSetSecurityMode: Failed to get Channel Info",
            phosphor::logging::entry("CHANNEL=%d", ctx->channel));
        return ipmi::responseUnspecifiedError();
    }
    auto reqMode =
        static_cast<securityNameSpace::RestrictionMode::Modes>(restrictionMode);

    if ((reqMode < securityNameSpace::RestrictionMode::Modes::Provisioning) ||
        (reqMode >
         securityNameSpace::RestrictionMode::Modes::ProvisionedHostDisabled))
    {
        return ipmi::responseInvalidFieldRequest();
    }

    boost::system::error_code ec;
    auto varRestrMode = ctx->bus->yield_method_call<std::variant<std::string>>(
        ctx->yield, ec, restricionModeService, restricionModeBasePath,
        dBusPropertyIntf, dBusPropertyGetMethod, restricionModeIntf,
        restricionModeProperty);
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiSetSecurityMode: failed to get RestrictionMode property",
            phosphor::logging::entry("ERROR=%s", ec.message().c_str()));
        return ipmi::responseUnspecifiedError();
    }
    auto currentRestrictionMode =
        securityNameSpace::RestrictionMode::convertModesFromString(
            std::get<std::string>(varRestrMode));

    if (chInfo.mediumType !=
            static_cast<uint8_t>(EChannelMediumType::lan8032) &&
        currentRestrictionMode > reqMode)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiSetSecurityMode - Downgrading security mode not supported "
            "through system interface",
            phosphor::logging::entry(
                "CUR_MODE=%d", static_cast<uint8_t>(currentRestrictionMode)),
            phosphor::logging::entry("REQ_MODE=%d", restrictionMode));
        return ipmi::responseCommandNotAvailable();
    }

    ec.clear();
    ctx->bus->yield_method_call<>(
        ctx->yield, ec, restricionModeService, restricionModeBasePath,
        dBusPropertyIntf, dBusPropertySetMethod, restricionModeIntf,
        restricionModeProperty,
        static_cast<std::variant<std::string>>(
            securityNameSpace::convertForMessage(reqMode)));

    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmiSetSecurityMode: failed to set RestrictionMode property",
            phosphor::logging::entry("ERROR=%s", ec.message().c_str()));
        return ipmi::responseUnspecifiedError();
    }

#ifdef BMC_VALIDATION_UNSECURE_FEATURE
    if (specialMode)
    {
        ec.clear();
        ctx->bus->yield_method_call<>(
            ctx->yield, ec, specialModeService, specialModeBasePath,
            dBusPropertyIntf, dBusPropertySetMethod, specialModeIntf,
            specialModeProperty,
            static_cast<std::variant<std::string>>(
                securityNameSpace::convertForMessage(
                    static_cast<securityNameSpace::SpecialMode::Modes>(
                        specialMode.value()))));

        if (ec)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiSetSecurityMode: failed to set SpecialMode property",
                phosphor::logging::entry("ERROR=%s", ec.message().c_str()));
            return ipmi::responseUnspecifiedError();
        }
    }
#endif
    return ipmi::responseSuccess();
}

ipmi::RspType<uint8_t /* restore status */>
    ipmiRestoreConfiguration(const std::array<uint8_t, 3>& clr, uint8_t cmd)
{
    static constexpr std::array<uint8_t, 3> expClr = {'C', 'L', 'R'};

    if (clr != expClr)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    constexpr uint8_t cmdStatus = 0;
    constexpr uint8_t cmdDefaultRestore = 0xaa;
    constexpr uint8_t cmdFullRestore = 0xbb;
    constexpr uint8_t cmdFormat = 0xcc;

    constexpr const char* restoreOpFname = "/tmp/.rwfs/.restore_op";

    switch (cmd)
    {
        case cmdStatus:
            break;
        case cmdDefaultRestore:
        case cmdFullRestore:
        case cmdFormat:
        {
            // write file to rwfs root
            int value = (cmd - 1) & 0x03; // map aa, bb, cc => 1, 2, 3
            std::ofstream restoreFile(restoreOpFname);
            if (!restoreFile)
            {
                return ipmi::responseUnspecifiedError();
            }
            restoreFile << value << "\n";
            break;
        }
        default:
            return ipmi::responseInvalidFieldRequest();
    }

    constexpr uint8_t restorePending = 0;
    constexpr uint8_t restoreComplete = 1;

    uint8_t restoreStatus = std::filesystem::exists(restoreOpFname)
                                ? restorePending
                                : restoreComplete;
    return ipmi::responseSuccess(restoreStatus);
}

ipmi::RspType<uint8_t> ipmiOEMGetNmiSource(void)
{
    uint8_t bmcSource;
    namespace nmi = sdbusplus::xyz::openbmc_project::Chassis::Control::server;

    try
    {
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        std::string service =
            getService(*dbus, oemNmiSourceIntf, oemNmiSourceObjPath);
        Value variant =
            getDbusProperty(*dbus, service, oemNmiSourceObjPath,
                            oemNmiSourceIntf, oemNmiBmcSourceObjPathProp);

        switch (nmi::NMISource::convertBMCSourceSignalFromString(
            std::get<std::string>(variant)))
        {
            case nmi::NMISource::BMCSourceSignal::None:
                bmcSource = static_cast<uint8_t>(NmiSource::none);
                break;
            case nmi::NMISource::BMCSourceSignal::FrontPanelButton:
                bmcSource = static_cast<uint8_t>(NmiSource::frontPanelButton);
                break;
            case nmi::NMISource::BMCSourceSignal::Watchdog:
                bmcSource = static_cast<uint8_t>(NmiSource::watchdog);
                break;
            case nmi::NMISource::BMCSourceSignal::ChassisCmd:
                bmcSource = static_cast<uint8_t>(NmiSource::chassisCmd);
                break;
            case nmi::NMISource::BMCSourceSignal::MemoryError:
                bmcSource = static_cast<uint8_t>(NmiSource::memoryError);
                break;
            case nmi::NMISource::BMCSourceSignal::PciBusError:
                bmcSource = static_cast<uint8_t>(NmiSource::pciBusError);
                break;
            case nmi::NMISource::BMCSourceSignal::PCH:
                bmcSource = static_cast<uint8_t>(NmiSource::pch);
                break;
            case nmi::NMISource::BMCSourceSignal::Chipset:
                bmcSource = static_cast<uint8_t>(NmiSource::chipset);
                break;
            default:
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "NMI source: invalid property!",
                    phosphor::logging::entry(
                        "PROP=%s", std::get<std::string>(variant).c_str()));
                return ipmi::responseResponseError();
        }
    }
    catch (sdbusplus::exception::SdBusError& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(bmcSource);
}

ipmi::RspType<> ipmiOEMSetNmiSource(uint8_t sourceId)
{
    namespace nmi = sdbusplus::xyz::openbmc_project::Chassis::Control::server;

    nmi::NMISource::BMCSourceSignal bmcSourceSignal =
        nmi::NMISource::BMCSourceSignal::None;

    switch (NmiSource(sourceId))
    {
        case NmiSource::none:
            bmcSourceSignal = nmi::NMISource::BMCSourceSignal::None;
            break;
        case NmiSource::frontPanelButton:
            bmcSourceSignal = nmi::NMISource::BMCSourceSignal::FrontPanelButton;
            break;
        case NmiSource::watchdog:
            bmcSourceSignal = nmi::NMISource::BMCSourceSignal::Watchdog;
            break;
        case NmiSource::chassisCmd:
            bmcSourceSignal = nmi::NMISource::BMCSourceSignal::ChassisCmd;
            break;
        case NmiSource::memoryError:
            bmcSourceSignal = nmi::NMISource::BMCSourceSignal::MemoryError;
            break;
        case NmiSource::pciBusError:
            bmcSourceSignal = nmi::NMISource::BMCSourceSignal::PciBusError;
            break;
        case NmiSource::pch:
            bmcSourceSignal = nmi::NMISource::BMCSourceSignal::PCH;
            break;
        case NmiSource::chipset:
            bmcSourceSignal = nmi::NMISource::BMCSourceSignal::Chipset;
            break;
        default:
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "NMI source: invalid property!");
            return ipmi::responseResponseError();
    }

    try
    {
        // keep NMI signal source
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        std::string service =
            getService(*dbus, oemNmiSourceIntf, oemNmiSourceObjPath);
        setDbusProperty(*dbus, service, oemNmiSourceObjPath, oemNmiSourceIntf,
                        oemNmiBmcSourceObjPathProp,
                        nmi::convertForMessage(bmcSourceSignal));
        // set Enabled property to inform NMI source handling
        // to trigger a NMI_OUT BSOD.
        // if it's triggered by NMI source property changed,
        // NMI_OUT BSOD could be missed if the same source occurs twice in a row
        if (bmcSourceSignal != nmi::NMISource::BMCSourceSignal::None)
        {
            setDbusProperty(*dbus, service, oemNmiSourceObjPath,
                            oemNmiSourceIntf, oemNmiEnabledObjPathProp,
                            static_cast<bool>(true));
        }
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess();
}

namespace dimmOffset
{
constexpr const char* dimmPower = "DimmPower";
constexpr const char* staticCltt = "StaticCltt";
constexpr const char* offsetPath = "/xyz/openbmc_project/Inventory/Item/Dimm";
constexpr const char* offsetInterface =
    "xyz.openbmc_project.Inventory.Item.Dimm.Offset";
constexpr const char* property = "DimmOffset";

}; // namespace dimmOffset

ipmi::RspType<>
    ipmiOEMSetDimmOffset(uint8_t type,
                         const std::vector<std::tuple<uint8_t, uint8_t>>& data)
{
    if (type != static_cast<uint8_t>(dimmOffsetTypes::dimmPower) &&
        type != static_cast<uint8_t>(dimmOffsetTypes::staticCltt))
    {
        return ipmi::responseInvalidFieldRequest();
    }

    if (data.empty())
    {
        return ipmi::responseInvalidFieldRequest();
    }
    nlohmann::json json;

    std::ifstream jsonStream(dimmOffsetFile);
    if (jsonStream.good())
    {
        json = nlohmann::json::parse(jsonStream, nullptr, false);
        if (json.is_discarded())
        {
            json = nlohmann::json();
        }
        jsonStream.close();
    }

    std::string typeName;
    if (type == static_cast<uint8_t>(dimmOffsetTypes::dimmPower))
    {
        typeName = dimmOffset::dimmPower;
    }
    else
    {
        typeName = dimmOffset::staticCltt;
    }

    nlohmann::json& field = json[typeName];

    for (const auto& [index, value] : data)
    {
        field[index] = value;
    }

    for (nlohmann::json& val : field)
    {
        if (val == nullptr)
        {
            val = static_cast<uint8_t>(0);
        }
    }

    std::ofstream output(dimmOffsetFile);
    if (!output.good())
    {
        std::cerr << "Error writing json file\n";
        return ipmi::responseResponseError();
    }

    output << json.dump(4);

    if (type == static_cast<uint8_t>(dimmOffsetTypes::staticCltt))
    {
        std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();

        std::variant<std::vector<uint8_t>> offsets =
            field.get<std::vector<uint8_t>>();
        auto call = bus->new_method_call(
            settingsBusName, dimmOffset::offsetPath, PROP_INTF, "Set");
        call.append(dimmOffset::offsetInterface, dimmOffset::property, offsets);
        try
        {
            bus->call(call);
        }
        catch (sdbusplus::exception_t& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmiOEMSetDimmOffset: can't set dimm offsets!",
                phosphor::logging::entry("ERR=%s", e.what()));
            return ipmi::responseResponseError();
        }
    }

    return ipmi::responseSuccess();
}

ipmi::RspType<uint8_t> ipmiOEMGetDimmOffset(uint8_t type, uint8_t index)
{

    if (type != static_cast<uint8_t>(dimmOffsetTypes::dimmPower) &&
        type != static_cast<uint8_t>(dimmOffsetTypes::staticCltt))
    {
        return ipmi::responseInvalidFieldRequest();
    }

    std::ifstream jsonStream(dimmOffsetFile);

    auto json = nlohmann::json::parse(jsonStream, nullptr, false);
    if (json.is_discarded())
    {
        std::cerr << "File error in " << dimmOffsetFile << "\n";
        return ipmi::responseResponseError();
    }

    std::string typeName;
    if (type == static_cast<uint8_t>(dimmOffsetTypes::dimmPower))
    {
        typeName = dimmOffset::dimmPower;
    }
    else
    {
        typeName = dimmOffset::staticCltt;
    }

    auto it = json.find(typeName);
    if (it == json.end())
    {
        return ipmi::responseInvalidFieldRequest();
    }

    if (it->size() <= index)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    uint8_t resp = it->at(index).get<uint8_t>();
    return ipmi::responseSuccess(resp);
}

namespace boot_options
{

using namespace sdbusplus::xyz::openbmc_project::Control::Boot::server;
using IpmiValue = uint8_t;
constexpr auto ipmiDefault = 0;

std::map<IpmiValue, Source::Sources> sourceIpmiToDbus = {
    {0x01, Source::Sources::Network},
    {0x02, Source::Sources::Disk},
    {0x05, Source::Sources::ExternalMedia},
    {0x0f, Source::Sources::RemovableMedia},
    {ipmiDefault, Source::Sources::Default}};

std::map<IpmiValue, Mode::Modes> modeIpmiToDbus = {
    {0x06, Mode::Modes::Setup}, {ipmiDefault, Mode::Modes::Regular}};

std::map<Source::Sources, IpmiValue> sourceDbusToIpmi = {
    {Source::Sources::Network, 0x01},
    {Source::Sources::Disk, 0x02},
    {Source::Sources::ExternalMedia, 0x05},
    {Source::Sources::RemovableMedia, 0x0f},
    {Source::Sources::Default, ipmiDefault}};

std::map<Mode::Modes, IpmiValue> modeDbusToIpmi = {
    {Mode::Modes::Setup, 0x06}, {Mode::Modes::Regular, ipmiDefault}};

static constexpr auto bootModeIntf = "xyz.openbmc_project.Control.Boot.Mode";
static constexpr auto bootSourceIntf =
    "xyz.openbmc_project.Control.Boot.Source";
static constexpr auto enabledIntf = "xyz.openbmc_project.Object.Enable";
static constexpr auto persistentObjPath =
    "/xyz/openbmc_project/control/host0/boot";
static constexpr auto oneTimePath =
    "/xyz/openbmc_project/control/host0/boot/one_time";
static constexpr auto bootSourceProp = "BootSource";
static constexpr auto bootModeProp = "BootMode";
static constexpr auto oneTimeBootEnableProp = "Enabled";
static constexpr auto httpBootMode =
    "xyz.openbmc_project.Control.Boot.Source.Sources.Http";

enum class BootOptionParameter : size_t
{
    setInProgress = 0x0,
    bootFlags = 0x5,
};
static constexpr uint8_t setComplete = 0x0;
static constexpr uint8_t setInProgress = 0x1;
static uint8_t transferStatus = setComplete;
static constexpr uint8_t setParmVersion = 0x01;
static constexpr uint8_t setParmBootFlagsPermanent = 0x40;
static constexpr uint8_t setParmBootFlagsValidOneTime = 0x80;
static constexpr uint8_t setParmBootFlagsValidPermanent = 0xC0;
static constexpr uint8_t httpBoot = 0xd;
static constexpr uint8_t bootSourceMask = 0x3c;

} // namespace boot_options

ipmi::RspType<uint8_t,               // version
              uint8_t,               // param
              uint8_t,               // data0, dependent on parameter
              std::optional<uint8_t> // data1, dependent on parameter
              >
    ipmiOemGetEfiBootOptions(uint8_t parameter, uint8_t set, uint8_t block)
{
    using namespace boot_options;
    uint8_t bootOption = 0;

    if (parameter == static_cast<uint8_t>(BootOptionParameter::setInProgress))
    {
        return ipmi::responseSuccess(setParmVersion, parameter, transferStatus,
                                     std::nullopt);
    }

    if (parameter != static_cast<uint8_t>(BootOptionParameter::bootFlags))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unsupported parameter");
        return ipmi::responseResponseError();
    }

    try
    {
        auto oneTimeEnabled = false;
        // read one time Enabled property
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        std::string service = getService(*dbus, enabledIntf, oneTimePath);
        Value variant = getDbusProperty(*dbus, service, oneTimePath,
                                        enabledIntf, oneTimeBootEnableProp);
        oneTimeEnabled = std::get<bool>(variant);

        // get BootSource and BootMode properties
        // according to oneTimeEnable
        auto bootObjPath = oneTimePath;
        if (oneTimeEnabled == false)
        {
            bootObjPath = persistentObjPath;
        }

        service = getService(*dbus, bootModeIntf, bootObjPath);
        variant = getDbusProperty(*dbus, service, bootObjPath, bootModeIntf,
                                  bootModeProp);

        auto bootMode =
            Mode::convertModesFromString(std::get<std::string>(variant));

        service = getService(*dbus, bootSourceIntf, bootObjPath);
        variant = getDbusProperty(*dbus, service, bootObjPath, bootSourceIntf,
                                  bootSourceProp);

        if (std::get<std::string>(variant) == httpBootMode)
        {
            bootOption = httpBoot;
        }
        else
        {
            auto bootSource = Source::convertSourcesFromString(
                std::get<std::string>(variant));
            bootOption = sourceDbusToIpmi.at(bootSource);
            if (Source::Sources::Default == bootSource)
            {
                bootOption = modeDbusToIpmi.at(bootMode);
            }
        }

        uint8_t oneTime = oneTimeEnabled ? setParmBootFlagsValidOneTime
                                         : setParmBootFlagsValidPermanent;
        bootOption <<= 2; // shift for responseconstexpr
        return ipmi::responseSuccess(setParmVersion, parameter, oneTime,
                                     bootOption);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        return ipmi::responseResponseError();
    }
}

ipmi::RspType<> ipmiOemSetEfiBootOptions(uint8_t bootFlag, uint8_t bootParam,
                                         std::optional<uint8_t> bootOption)
{
    using namespace boot_options;
    auto oneTimeEnabled = false;

    if (bootFlag == static_cast<uint8_t>(BootOptionParameter::setInProgress))
    {
        if (bootOption)
        {
            return ipmi::responseReqDataLenInvalid();
        }

        if (transferStatus == setInProgress)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "boot option set in progress!");
            return ipmi::responseResponseError();
        }

        transferStatus = bootParam;
        return ipmi::responseSuccess();
    }

    if (bootFlag != (uint8_t)BootOptionParameter::bootFlags)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unsupported parameter");
        return ipmi::responseResponseError();
    }

    if (!bootOption)
    {
        return ipmi::responseReqDataLenInvalid();
    }

    if (((bootOption.value() & bootSourceMask) >> 2) !=
        httpBoot) // not http boot, exit
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "wrong boot option parameter!");
        return ipmi::responseParmOutOfRange();
    }

    try
    {
        bool permanent = (bootParam & setParmBootFlagsPermanent) ==
                         setParmBootFlagsPermanent;

        // read one time Enabled property
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        std::string service = getService(*dbus, enabledIntf, oneTimePath);
        Value variant = getDbusProperty(*dbus, service, oneTimePath,
                                        enabledIntf, oneTimeBootEnableProp);
        oneTimeEnabled = std::get<bool>(variant);

        /*
         * Check if the current boot setting is onetime or permanent, if the
         * request in the command is otherwise, then set the "Enabled"
         * property in one_time object path to 'True' to indicate onetime
         * and 'False' to indicate permanent.
         *
         * Once the onetime/permanent setting is applied, then the bootMode
         * and bootSource is updated for the corresponding object.
         */
        if (permanent == oneTimeEnabled)
        {
            setDbusProperty(*dbus, service, oneTimePath, enabledIntf,
                            oneTimeBootEnableProp, !permanent);
        }

        // set BootSource and BootMode properties
        // according to oneTimeEnable or persistent
        auto bootObjPath = oneTimePath;
        if (oneTimeEnabled == false)
        {
            bootObjPath = persistentObjPath;
        }
        std::string bootMode =
            "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular";
        std::string bootSource = httpBootMode;

        service = getService(*dbus, bootModeIntf, bootObjPath);
        setDbusProperty(*dbus, service, bootObjPath, bootModeIntf, bootModeProp,
                        bootMode);

        service = getService(*dbus, bootSourceIntf, bootObjPath);
        setDbusProperty(*dbus, service, bootObjPath, bootSourceIntf,
                        bootSourceProp, bootSource);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess();
}

using BasicVariantType =
    std::variant<std::vector<std::string>, std::vector<uint64_t>, std::string,
                 int64_t, uint64_t, double, int32_t, uint32_t, int16_t,
                 uint16_t, uint8_t, bool>;
using PropertyMapType =
    boost::container::flat_map<std::string, BasicVariantType>;
static constexpr const std::array<const char*, 1> psuPresenceTypes = {
    "xyz.openbmc_project.Configuration.PSUPresence"};
int getPSUAddress(ipmi::Context::ptr ctx, uint8_t& bus,
                  std::vector<uint64_t>& addrTable)
{
    boost::system::error_code ec;
    GetSubTreeType subtree = ctx->bus->yield_method_call<GetSubTreeType>(
        ctx->yield, ec, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory/system", 3, psuPresenceTypes);
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to set dbus property to cold redundancy");
        return -1;
    }
    for (const auto& object : subtree)
    {
        std::string pathName = object.first;
        for (const auto& serviceIface : object.second)
        {
            std::string serviceName = serviceIface.first;

            ec.clear();
            PropertyMapType propMap =
                ctx->bus->yield_method_call<PropertyMapType>(
                    ctx->yield, ec, serviceName, pathName,
                    "org.freedesktop.DBus.Properties", "GetAll",
                    "xyz.openbmc_project.Configuration.PSUPresence");
            if (ec)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Failed to set dbus property to cold redundancy");
                return -1;
            }
            auto psuBus = std::get_if<uint64_t>(&propMap["Bus"]);
            auto psuAddress =
                std::get_if<std::vector<uint64_t>>(&propMap["Address"]);

            if (psuBus == nullptr || psuAddress == nullptr)
            {
                std::cerr << "error finding necessary "
                             "entry in configuration\n";
                return -1;
            }
            bus = static_cast<uint8_t>(*psuBus);
            addrTable = *psuAddress;
            return 0;
        }
    }
    return -1;
}

static const constexpr uint8_t addrOffset = 8;
static const constexpr uint8_t psuRevision = 0xd9;
static const constexpr uint8_t defaultPSUBus = 7;
// Second Minor, Primary Minor, Major
static const constexpr size_t verLen = 3;
ipmi::RspType<std::vector<uint8_t>> ipmiOEMGetPSUVersion(ipmi::Context::ptr ctx)
{
    uint8_t bus = defaultPSUBus;
    std::vector<uint64_t> addrTable;
    std::vector<uint8_t> result;
    if (getPSUAddress(ctx, bus, addrTable))
    {
        std::cerr << "Failed to get PSU bus and address\n";
        return ipmi::responseResponseError();
    }

    for (const auto& slaveAddr : addrTable)
    {
        std::vector<uint8_t> writeData = {psuRevision};
        std::vector<uint8_t> readBuf(verLen);
        uint8_t addr = static_cast<uint8_t>(slaveAddr) + addrOffset;
        std::string i2cBus = "/dev/i2c-" + std::to_string(bus);

        auto retI2C = ipmi::i2cWriteRead(i2cBus, addr, writeData, readBuf);
        if (retI2C != ipmi::ccSuccess)
        {
            for (size_t idx = 0; idx < verLen; idx++)
            {
                result.emplace_back(0x00);
            }
        }
        else
        {
            for (const uint8_t& data : readBuf)
            {
                result.emplace_back(data);
            }
        }
    }

    return ipmi::responseSuccess(result);
}

std::optional<uint8_t> getMultiNodeInfoPresence(ipmi::Context::ptr ctx,
                                                const std::string& name)
{
    Value dbusValue = 0;
    std::string serviceName;

    boost::system::error_code ec =
        ipmi::getService(ctx, multiNodeIntf, multiNodeObjPath, serviceName);

    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to perform Multinode getService.");
        return std::nullopt;
    }

    ec = ipmi::getDbusProperty(ctx, serviceName, multiNodeObjPath,
                               multiNodeIntf, name, dbusValue);
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to perform Multinode get property");
        return std::nullopt;
    }

    auto multiNodeVal = std::get_if<uint8_t>(&dbusValue);
    if (!multiNodeVal)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "getMultiNodeInfoPresence: error to get multinode");
        return std::nullopt;
    }
    return *multiNodeVal;
}

/** @brief implements OEM get reading command
 *  @param domain ID
 *  @param reading Type
 *    - 00h = platform Power Consumption
 *    - 01h = inlet Air Temp
 *    - 02h = icc_TDC from PECI
 *  @param reserved, write as 0000h
 *
 *  @returns IPMI completion code plus response data
 *  - response
 *     - domain ID
 *     - reading Type
 *       - 00h = platform Power Consumption
 *       - 01h = inlet Air Temp
 *       - 02h = icc_TDC from PECI
 *     - reading
 */
ipmi::RspType<uint4_t, // domain ID
              uint4_t, // reading Type
              uint16_t // reading Value
              >
    ipmiOEMGetReading(ipmi::Context::ptr ctx, uint4_t domainId,
                      uint4_t readingType, uint16_t reserved)
{
    constexpr uint8_t platformPower = 0;
    constexpr uint8_t inletAirTemp = 1;
    constexpr uint8_t iccTdc = 2;

    if ((static_cast<uint8_t>(readingType) > iccTdc) || domainId || reserved)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    // This command should run only from multi-node product.
    // For all other platforms this command will return invalid.

    std::optional<uint8_t> nodeInfo =
        getMultiNodeInfoPresence(ctx, "NodePresence");
    if (!nodeInfo || !*nodeInfo)
    {
        return ipmi::responseInvalidCommand();
    }

    uint16_t oemReadingValue = 0;
    if (static_cast<uint8_t>(readingType) == inletAirTemp)
    {
        double value = 0;
        boost::system::error_code ec = ipmi::getDbusProperty(
            ctx, "xyz.openbmc_project.HwmonTempSensor",
            "/xyz/openbmc_project/sensors/temperature/Inlet_BRD_Temp",
            "xyz.openbmc_project.Sensor.Value", "Value", value);
        if (ec)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to get BMC Get OEM temperature",
                phosphor::logging::entry("EXCEPTION=%s", ec.message().c_str()));
            return ipmi::responseUnspecifiedError();
        }
        // Take the Inlet temperature
        oemReadingValue = static_cast<uint16_t>(value);
    }
    else
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Currently Get OEM Reading support only for Inlet Air Temp");
        return ipmi::responseParmOutOfRange();
    }
    return ipmi::responseSuccess(domainId, readingType, oemReadingValue);
}

/** @brief implements the maximum size of
 *  bridgeable messages used between KCS and
 *  IPMB interfacesget security mode command.
 *
 *  @returns IPMI completion code with following data
 *   - KCS Buffer Size (In multiples of four bytes)
 *   - IPMB Buffer Size (In multiples of four bytes)
 **/
ipmi::RspType<uint8_t, uint8_t> ipmiOEMGetBufferSize()
{
    // for now this is hard coded; really this number is dependent on
    // the BMC kcs driver as well as the host kcs driver....
    // we can't know the latter.
    uint8_t kcsMaxBufferSize = 63 / 4;
    uint8_t ipmbMaxBufferSize = 128 / 4;

    return ipmi::responseSuccess(kcsMaxBufferSize, ipmbMaxBufferSize);
}

static void registerOEMFunctions(void)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Registering OEM commands");
    ipmiPrintAndRegister(intel::netFnGeneral,
                         intel::general::cmdGetChassisIdentifier, NULL,
                         ipmiOEMGetChassisIdentifier,
                         PRIVILEGE_USER); // get chassis identifier

    ipmiPrintAndRegister(intel::netFnGeneral, intel::general::cmdSetSystemGUID,
                         NULL, ipmiOEMSetSystemGUID,
                         PRIVILEGE_ADMIN); // set system guid

    // <Disable BMC System Reset Action>
    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdDisableBMCSystemReset, Privilege::Admin,
                    ipmiOEMDisableBMCSystemReset);

    // <Get BMC Reset Disables>
    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetBMCResetDisables, Privilege::Admin,
                    ipmiOEMGetBMCResetDisables);

    ipmiPrintAndRegister(intel::netFnGeneral, intel::general::cmdSetBIOSID,
                         NULL, ipmiOEMSetBIOSID, PRIVILEGE_ADMIN);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetOEMDeviceInfo, Privilege::User,
                    ipmiOEMGetDeviceInfo);

    ipmiPrintAndRegister(intel::netFnGeneral,
                         intel::general::cmdGetAICSlotFRUIDSlotPosRecords, NULL,
                         ipmiOEMGetAICFRU, PRIVILEGE_USER);

    registerHandler(prioOpenBmcBase, intel::netFnGeneral,
                    intel::general::cmdSendEmbeddedFWUpdStatus,
                    Privilege::Operator, ipmiOEMSendEmbeddedFwUpdStatus);

    registerHandler(prioOpenBmcBase, intel::netFnApp, intel::app::cmdSlotIpmb,
                    Privilege::Admin, ipmiOEMSlotIpmb);

    ipmiPrintAndRegister(intel::netFnGeneral,
                         intel::general::cmdSetPowerRestoreDelay, NULL,
                         ipmiOEMSetPowerRestoreDelay, PRIVILEGE_OPERATOR);

    ipmiPrintAndRegister(intel::netFnGeneral,
                         intel::general::cmdGetPowerRestoreDelay, NULL,
                         ipmiOEMGetPowerRestoreDelay, PRIVILEGE_USER);

    registerHandler(prioOpenBmcBase, intel::netFnGeneral,
                    intel::general::cmdSetOEMUser2Activation,
                    Privilege::Callback, ipmiOEMSetUser2Activation);

    registerHandler(prioOpenBmcBase, intel::netFnGeneral,
                    intel::general::cmdSetSpecialUserPassword,
                    Privilege::Callback, ipmiOEMSetSpecialUserPassword);

    // <Get Processor Error Config>
    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetProcessorErrConfig, Privilege::User,
                    ipmiOEMGetProcessorErrConfig);

    // <Set Processor Error Config>
    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdSetProcessorErrConfig, Privilege::Admin,
                    ipmiOEMSetProcessorErrConfig);

    ipmiPrintAndRegister(intel::netFnGeneral,
                         intel::general::cmdSetShutdownPolicy, NULL,
                         ipmiOEMSetShutdownPolicy, PRIVILEGE_ADMIN);

    ipmiPrintAndRegister(intel::netFnGeneral,
                         intel::general::cmdGetShutdownPolicy, NULL,
                         ipmiOEMGetShutdownPolicy, PRIVILEGE_ADMIN);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdSetFanConfig, Privilege::User,
                    ipmiOEMSetFanConfig);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetFanConfig, Privilege::User,
                    ipmiOEMGetFanConfig);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetFanSpeedOffset, Privilege::User,
                    ipmiOEMGetFanSpeedOffset);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdSetFanSpeedOffset, Privilege::User,
                    ipmiOEMSetFanSpeedOffset);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdSetFscParameter, Privilege::User,
                    ipmiOEMSetFscParameter);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetFscParameter, Privilege::User,
                    ipmiOEMGetFscParameter);

    registerHandler(prioOpenBmcBase, intel::netFnGeneral,
                    intel::general::cmdReadBaseBoardProductId, Privilege::Admin,
                    ipmiOEMReadBoardProductId);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetNmiStatus, Privilege::User,
                    ipmiOEMGetNmiSource);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdSetNmiStatus, Privilege::Operator,
                    ipmiOEMSetNmiSource);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetEfiBootOptions, Privilege::User,
                    ipmiOemGetEfiBootOptions);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdSetEfiBootOptions, Privilege::Operator,
                    ipmiOemSetEfiBootOptions);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetSecurityMode, Privilege::User,
                    ipmiGetSecurityMode);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdSetSecurityMode, Privilege::Admin,
                    ipmiSetSecurityMode);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetLEDStatus, Privilege::Admin,
                    ipmiOEMGetLEDStatus);

    ipmiPrintAndRegister(ipmi::intel::netFnPlatform,
                         ipmi::intel::platform::cmdCfgHostSerialPortSpeed, NULL,
                         ipmiOEMCfgHostSerialPortSpeed, PRIVILEGE_ADMIN);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdSetFaultIndication, Privilege::Operator,
                    ipmiOEMSetFaultIndication);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdSetColdRedundancyConfig, Privilege::User,
                    ipmiOEMSetCRConfig);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetColdRedundancyConfig, Privilege::User,
                    ipmiOEMGetCRConfig);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdRestoreConfiguration, Privilege::Admin,
                    ipmiRestoreConfiguration);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdSetDimmOffset, Privilege::Operator,
                    ipmiOEMSetDimmOffset);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetDimmOffset, Privilege::Operator,
                    ipmiOEMGetDimmOffset);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetPSUVersion, Privilege::User,
                    ipmiOEMGetPSUVersion);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdGetBufferSize, Privilege::User,
                    ipmiOEMGetBufferSize);

    registerHandler(prioOemBase, intel::netFnGeneral,
                    intel::general::cmdOEMGetReading, Privilege::User,
                    ipmiOEMGetReading);
}

} // namespace ipmi
