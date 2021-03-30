#include "config.h"

#include "chassishandler.hpp"

#include <arpa/inet.h>
#include <endian.h>
#include <limits.h>
#include <mapper.h>
#include <netinet/in.h>

#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <ipmid/api.hpp>
#include <ipmid/types.hpp>
#include <ipmid/utils.hpp>
#include <map>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdbusplus/timer.hpp>
#include <settings.hpp>
#include <sstream>
#include <string>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Control/Boot/Mode/server.hpp>
#include <xyz/openbmc_project/Control/Boot/Source/server.hpp>
#include <xyz/openbmc_project/Control/Power/RestorePolicy/server.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>
#include <xyz/openbmc_project/State/PowerOnHours/server.hpp>

// Defines
#define SET_PARM_VERSION 0x01
#define SET_PARM_BOOT_FLAGS_PERMANENT 0x40
#define SET_PARM_BOOT_FLAGS_VALID_ONE_TIME 0x80
#define SET_PARM_BOOT_FLAGS_VALID_PERMANENT 0xC0

std::unique_ptr<phosphor::Timer> identifyTimer
    __attribute__((init_priority(101)));

static ChassisIDState chassisIDState = ChassisIDState::reserved;

constexpr size_t SIZE_MAC = 18;
constexpr size_t SIZE_BOOT_OPTION = (uint8_t)
    BootOptionResponseSize::OPAL_NETWORK_SETTINGS; // Maximum size of the boot
                                                   // option parametrs
constexpr size_t SIZE_PREFIX = 7;
constexpr size_t MAX_PREFIX_VALUE = 32;
constexpr size_t SIZE_COOKIE = 4;
constexpr size_t SIZE_VERSION = 2;
constexpr size_t DEFAULT_IDENTIFY_TIME_OUT = 15;

// PetiBoot-Specific
static constexpr uint8_t net_conf_initial_bytes[] = {0x80, 0x21, 0x70, 0x62,
                                                     0x21, 0x00, 0x01, 0x06};

static constexpr size_t COOKIE_OFFSET = 1;
static constexpr size_t VERSION_OFFSET = 5;
static constexpr size_t ADDR_SIZE_OFFSET = 8;
static constexpr size_t MAC_OFFSET = 9;
static constexpr size_t ADDRTYPE_OFFSET = 16;
static constexpr size_t IPADDR_OFFSET = 17;

static constexpr uint4_t RESERVED = 0;
static constexpr uint8_t CHANNEL_NOT_SUPPORTED = 0;

static constexpr size_t encIdentifyObjectsSize = 1;
static constexpr size_t chassisIdentifyReqLength = 2;
static constexpr size_t identifyIntervalPos = 0;
static constexpr size_t forceIdentifyPos = 1;

void register_netfn_chassis_functions() __attribute__((constructor));

// Host settings in dbus
// Service name should be referenced by connection name got via object mapper
const char* settings_object_name = "/org/openbmc/settings/host0";
const char* settings_intf_name = "org.freedesktop.DBus.Properties";
const char* identify_led_object_name =
    "/xyz/openbmc_project/led/groups/enclosure_identify";

constexpr auto SETTINGS_ROOT = "/";
constexpr auto SETTINGS_MATCH = "host0";

constexpr auto IP_INTERFACE = "xyz.openbmc_project.Network.IP";
constexpr auto MAC_INTERFACE = "xyz.openbmc_project.Network.MACAddress";

static constexpr auto chassisStateRoot = "/xyz/openbmc_project/state";
static constexpr auto chassisPOHStateIntf =
    "xyz.openbmc_project.State.PowerOnHours";
static constexpr auto pOHCounterProperty = "POHCounter";
static constexpr auto match = "chassis0";
const static constexpr char chassisCapIntf[] =
    "xyz.openbmc_project.Control.ChassisCapabilities";
const static constexpr char chassisIntrusionProp[] = "ChassisIntrusionEnabled";
const static constexpr char chassisFrontPanelLockoutProp[] =
    "ChassisFrontPanelLockoutEnabled";
const static constexpr char chassisNMIProp[] = "ChassisNMIEnabled";
const static constexpr char chassisPowerInterlockProp[] =
    "ChassisPowerInterlockEnabled";
const static constexpr char chassisFRUDevAddrProp[] = "FRUDeviceAddress";
const static constexpr char chassisSDRDevAddrProp[] = "SDRDeviceAddress";
const static constexpr char chassisSELDevAddrProp[] = "SELDeviceAddress";
const static constexpr char chassisSMDevAddrProp[] = "SMDeviceAddress";
const static constexpr char chassisBridgeDevAddrProp[] = "BridgeDeviceAddress";
static constexpr uint8_t chassisCapFlagMask = 0x0f;
static constexpr uint8_t chassisCapAddrMask = 0xfe;
static constexpr const char* powerButtonIntf =
    "xyz.openbmc_project.Chassis.Buttons.Power";
static constexpr const char* powerButtonPath =
    "/xyz/openbmc_project/Chassis/Buttons/Power0";
static constexpr const char* resetButtonIntf =
    "xyz.openbmc_project.Chassis.Buttons.Reset";
static constexpr const char* resetButtonPath =
    "/xyz/openbmc_project/Chassis/Buttons/Reset0";

// Phosphor Host State manager
namespace State = sdbusplus::xyz::openbmc_project::State::server;

namespace fs = std::filesystem;

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace sdbusplus::xyz::openbmc_project::Control::Boot::server;

namespace chassis
{
namespace internal
{

constexpr auto bootModeIntf = "xyz.openbmc_project.Control.Boot.Mode";
constexpr auto bootSourceIntf = "xyz.openbmc_project.Control.Boot.Source";
constexpr auto powerRestoreIntf =
    "xyz.openbmc_project.Control.Power.RestorePolicy";
sdbusplus::bus::bus dbus(ipmid_get_sd_bus_connection());

namespace cache
{

std::unique_ptr<settings::Objects> objectsPtr = nullptr;

settings::Objects& getObjects()
{
    if (objectsPtr == nullptr)
    {
        objectsPtr = std::make_unique<settings::Objects>(
            dbus, std::vector<std::string>{bootModeIntf, bootSourceIntf,
                                           powerRestoreIntf});
    }
    return *objectsPtr;
}

} // namespace cache
} // namespace internal
} // namespace chassis

namespace poh
{

constexpr auto minutesPerCount = 60;

} // namespace poh

struct get_sys_boot_options_t
{
    uint8_t parameter;
    uint8_t set;
    uint8_t block;
} __attribute__((packed));

struct get_sys_boot_options_response_t
{
    uint8_t version;
    uint8_t parm;
    uint8_t data[SIZE_BOOT_OPTION];
} __attribute__((packed));

struct set_sys_boot_options_t
{
    uint8_t parameter;
    uint8_t data[SIZE_BOOT_OPTION];
} __attribute__((packed));

int getHostNetworkData(get_sys_boot_options_response_t* respptr)
{
    ipmi::PropertyMap properties;
    int rc = 0;
    uint8_t addrSize = ipmi::network::IPV4_ADDRESS_SIZE_BYTE;

    try
    {
        // TODO There may be cases where an interface is implemented by multiple
        // objects,to handle such cases we are interested on that object
        //  which are on interested busname.
        //  Currenlty mapper doesn't give the readable busname(gives busid)
        //  so we can't match with bus name so giving some object specific info
        //  as SETTINGS_MATCH.
        //  Later SETTINGS_MATCH will be replaced with busname.

        sdbusplus::bus::bus bus(ipmid_get_sd_bus_connection());

        auto ipObjectInfo = ipmi::getDbusObject(bus, IP_INTERFACE,
                                                SETTINGS_ROOT, SETTINGS_MATCH);

        auto macObjectInfo = ipmi::getDbusObject(bus, MAC_INTERFACE,
                                                 SETTINGS_ROOT, SETTINGS_MATCH);

        properties = ipmi::getAllDbusProperties(
            bus, ipObjectInfo.second, ipObjectInfo.first, IP_INTERFACE);
        auto variant = ipmi::getDbusProperty(bus, macObjectInfo.second,
                                             macObjectInfo.first, MAC_INTERFACE,
                                             "MACAddress");

        auto ipAddress = std::get<std::string>(properties["Address"]);

        auto gateway = std::get<std::string>(properties["Gateway"]);

        auto prefix = std::get<uint8_t>(properties["PrefixLength"]);

        uint8_t isStatic =
            (std::get<std::string>(properties["Origin"]) ==
             "xyz.openbmc_project.Network.IP.AddressOrigin.Static")
                ? 1
                : 0;

        auto MACAddress = std::get<std::string>(variant);

        // it is expected here that we should get the valid data
        // but we may also get the default values.
        // Validation of the data is done by settings.
        //
        // if mac address is default mac address then
        // don't send blank override.
        if ((MACAddress == ipmi::network::DEFAULT_MAC_ADDRESS))
        {
            std::memset(respptr->data, 0, SIZE_BOOT_OPTION);
            rc = -1;
            return rc;
        }
        // if addr is static then ipaddress,gateway,prefix
        // should not be default one,don't send blank override.
        if (isStatic)
        {
            if ((ipAddress == ipmi::network::DEFAULT_ADDRESS) ||
                (gateway == ipmi::network::DEFAULT_ADDRESS) || (!prefix))
            {
                std::memset(respptr->data, 0, SIZE_BOOT_OPTION);
                rc = -1;
                return rc;
            }
        }

        sscanf(
            MACAddress.c_str(), ipmi::network::MAC_ADDRESS_FORMAT,
            (respptr->data + MAC_OFFSET), (respptr->data + MAC_OFFSET + 1),
            (respptr->data + MAC_OFFSET + 2), (respptr->data + MAC_OFFSET + 3),
            (respptr->data + MAC_OFFSET + 4), (respptr->data + MAC_OFFSET + 5));

        respptr->data[MAC_OFFSET + 6] = 0x00;

        std::memcpy(respptr->data + ADDRTYPE_OFFSET, &isStatic,
                    sizeof(isStatic));

        uint8_t addressFamily = (std::get<std::string>(properties["Type"]) ==
                                 "xyz.openbmc_project.Network.IP.Protocol.IPv4")
                                    ? AF_INET
                                    : AF_INET6;

        addrSize = (addressFamily == AF_INET)
                       ? ipmi::network::IPV4_ADDRESS_SIZE_BYTE
                       : ipmi::network::IPV6_ADDRESS_SIZE_BYTE;

        // ipaddress and gateway would be in IPv4 format
        inet_pton(addressFamily, ipAddress.c_str(),
                  (respptr->data + IPADDR_OFFSET));

        uint8_t prefixOffset = IPADDR_OFFSET + addrSize;

        std::memcpy(respptr->data + prefixOffset, &prefix, sizeof(prefix));

        uint8_t gatewayOffset = prefixOffset + sizeof(decltype(prefix));

        inet_pton(addressFamily, gateway.c_str(),
                  (respptr->data + gatewayOffset));
    }
    catch (InternalFailure& e)
    {
        commit<InternalFailure>();
        std::memset(respptr->data, 0, SIZE_BOOT_OPTION);
        rc = -1;
        return rc;
    }

    // PetiBoot-Specific
    // If success then copy the first 9 bytes to the data
    std::memcpy(respptr->data, net_conf_initial_bytes,
                sizeof(net_conf_initial_bytes));

    std::memcpy(respptr->data + ADDR_SIZE_OFFSET, &addrSize, sizeof(addrSize));

#ifdef _IPMI_DEBUG_
    std::printf("\n===Printing the IPMI Formatted Data========\n");

    for (uint8_t pos = 0; pos < index; pos++)
    {
        std::printf("%02x ", respptr->data[pos]);
    }
#endif

    return rc;
}

/** @brief convert IPv4 and IPv6 addresses from binary to text form.
 *  @param[in] family - IPv4/Ipv6
 *  @param[in] data - req data pointer.
 *  @param[in] offset - offset in the data.
 *  @param[in] addrSize - size of the data which needs to be read from offset.
 *  @returns address in text form.
 */

std::string getAddrStr(uint8_t family, uint8_t* data, uint8_t offset,
                       uint8_t addrSize)
{
    char ipAddr[INET6_ADDRSTRLEN] = {};

    switch (family)
    {
        case AF_INET:
        {
            struct sockaddr_in addr4
            {
            };
            std::memcpy(&addr4.sin_addr.s_addr, &data[offset], addrSize);

            inet_ntop(AF_INET, &addr4.sin_addr, ipAddr, INET_ADDRSTRLEN);

            break;
        }
        case AF_INET6:
        {
            struct sockaddr_in6 addr6
            {
            };
            std::memcpy(&addr6.sin6_addr.s6_addr, &data[offset], addrSize);

            inet_ntop(AF_INET6, &addr6.sin6_addr, ipAddr, INET6_ADDRSTRLEN);

            break;
        }
        default:
        {
            return {};
        }
    }

    return ipAddr;
}

int setHostNetworkData(set_sys_boot_options_t* reqptr)
{
    using namespace std::string_literals;
    std::string host_network_config;
    char mac[]{"00:00:00:00:00:00"};
    std::string ipAddress, gateway;
    char addrOrigin{0};
    uint8_t addrSize{0};
    std::string addressOrigin =
        "xyz.openbmc_project.Network.IP.AddressOrigin.DHCP";
    std::string addressType = "xyz.openbmc_project.Network.IP.Protocol.IPv4";
    uint8_t prefix{0};
    uint32_t zeroCookie = 0;
    uint8_t family = AF_INET;

    // cookie starts from second byte
    // version starts from sixth byte

    try
    {
        do
        {
            // cookie ==  0x21 0x70 0x62 0x21
            if (memcmp(&(reqptr->data[COOKIE_OFFSET]),
                       (net_conf_initial_bytes + COOKIE_OFFSET),
                       SIZE_COOKIE) != 0)
            {
                // cookie == 0
                if (memcmp(&(reqptr->data[COOKIE_OFFSET]), &zeroCookie,
                           SIZE_COOKIE) == 0)
                {
                    // need to zero out the network settings.
                    break;
                }

                log<level::ERR>("Invalid Cookie");
                elog<InternalFailure>();
            }

            // vesion == 0x00 0x01
            if (memcmp(&(reqptr->data[VERSION_OFFSET]),
                       (net_conf_initial_bytes + VERSION_OFFSET),
                       SIZE_VERSION) != 0)
            {

                log<level::ERR>("Invalid Version");
                elog<InternalFailure>();
            }

            std::snprintf(
                mac, SIZE_MAC, ipmi::network::MAC_ADDRESS_FORMAT,
                reqptr->data[MAC_OFFSET], reqptr->data[MAC_OFFSET + 1],
                reqptr->data[MAC_OFFSET + 2], reqptr->data[MAC_OFFSET + 3],
                reqptr->data[MAC_OFFSET + 4], reqptr->data[MAC_OFFSET + 5]);

            std::memcpy(&addrOrigin, &(reqptr->data[ADDRTYPE_OFFSET]),
                        sizeof(decltype(addrOrigin)));

            if (addrOrigin)
            {
                addressOrigin =
                    "xyz.openbmc_project.Network.IP.AddressOrigin.Static";
            }

            // Get the address size
            std::memcpy(&addrSize, &reqptr->data[ADDR_SIZE_OFFSET],
                        sizeof(addrSize));

            uint8_t prefixOffset = IPADDR_OFFSET + addrSize;

            std::memcpy(&prefix, &(reqptr->data[prefixOffset]),
                        sizeof(decltype(prefix)));

            uint8_t gatewayOffset = prefixOffset + sizeof(decltype(prefix));

            if (addrSize != ipmi::network::IPV4_ADDRESS_SIZE_BYTE)
            {
                addressType = "xyz.openbmc_project.Network.IP.Protocol.IPv6";
                family = AF_INET6;
            }

            ipAddress =
                getAddrStr(family, reqptr->data, IPADDR_OFFSET, addrSize);

            gateway = getAddrStr(family, reqptr->data, gatewayOffset, addrSize);

        } while (0);

        // Cookie == 0 or it is a valid cookie
        host_network_config += "ipaddress="s + ipAddress + ",prefix="s +
                               std::to_string(prefix) + ",gateway="s + gateway +
                               ",mac="s + mac + ",addressOrigin="s +
                               addressOrigin;

        sdbusplus::bus::bus bus(ipmid_get_sd_bus_connection());

        auto ipObjectInfo = ipmi::getDbusObject(bus, IP_INTERFACE,
                                                SETTINGS_ROOT, SETTINGS_MATCH);
        auto macObjectInfo = ipmi::getDbusObject(bus, MAC_INTERFACE,
                                                 SETTINGS_ROOT, SETTINGS_MATCH);
        // set the dbus property
        ipmi::setDbusProperty(bus, ipObjectInfo.second, ipObjectInfo.first,
                              IP_INTERFACE, "Address", std::string(ipAddress));
        ipmi::setDbusProperty(bus, ipObjectInfo.second, ipObjectInfo.first,
                              IP_INTERFACE, "PrefixLength", prefix);
        ipmi::setDbusProperty(bus, ipObjectInfo.second, ipObjectInfo.first,
                              IP_INTERFACE, "Origin", addressOrigin);
        ipmi::setDbusProperty(bus, ipObjectInfo.second, ipObjectInfo.first,
                              IP_INTERFACE, "Gateway", std::string(gateway));
        ipmi::setDbusProperty(
            bus, ipObjectInfo.second, ipObjectInfo.first, IP_INTERFACE, "Type",
            std::string("xyz.openbmc_project.Network.IP.Protocol.IPv4"));
        ipmi::setDbusProperty(bus, macObjectInfo.second, macObjectInfo.first,
                              MAC_INTERFACE, "MACAddress", std::string(mac));

        log<level::DEBUG>(
            "Network configuration changed",
            entry("NETWORKCONFIG=%s", host_network_config.c_str()));
    }
    catch (InternalFailure& e)
    {
        commit<InternalFailure>();
        return -1;
    }

    return 0;
}

uint32_t getPOHCounter()
{
    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};

    auto chassisStateObj =
        ipmi::getDbusObject(bus, chassisPOHStateIntf, chassisStateRoot, match);

    auto service =
        ipmi::getService(bus, chassisPOHStateIntf, chassisStateObj.first);

    auto propValue =
        ipmi::getDbusProperty(bus, service, chassisStateObj.first,
                              chassisPOHStateIntf, pOHCounterProperty);

    return std::get<uint32_t>(propValue);
}

/** @brief Implements the get chassis capabilities command
 *
 *  @returns IPMI completion code plus response data
 *  chassisCapFlags        - chassis capability flag
 *  chassisFRUInfoDevAddr  - chassis FRU info Device Address
 *  chassisSDRDevAddr      - chassis SDR device address
 *  chassisSELDevAddr      - chassis SEL device address
 *  chassisSMDevAddr       - chassis system management device address
 *  chassisBridgeDevAddr   - chassis bridge device address
 */
ipmi::RspType<bool,    // chassis intrusion sensor
              bool,    // chassis Front panel lockout
              bool,    // chassis NMI
              bool,    // chassis power interlock
              uint4_t, // reserved
              uint8_t, // chassis FRU info Device Address
              uint8_t, // chassis SDR device address
              uint8_t, // chassis SEL device address
              uint8_t, // chassis system management device address
              uint8_t  // chassis bridge device address
              >
    ipmiGetChassisCap()
{
    ipmi::PropertyMap properties;
    try
    {
        sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};

        ipmi::DbusObjectInfo chassisCapObject =
            ipmi::getDbusObject(bus, chassisCapIntf);

        // capabilities flags
        // [7..4] - reserved
        // [3] – 1b = provides power interlock  (IPM 1.5)
        // [2] – 1b = provides Diagnostic Interrupt (FP NMI)
        // [1] – 1b = provides “Front Panel Lockout” (indicates that the chassis
        // has capabilities
        //            to lock out external power control and reset button or
        //            front panel interfaces and/or detect tampering with those
        //            interfaces).
        // [0] -1b = Chassis provides intrusion (physical security) sensor.
        // set to default value 0x0.

        properties =
            ipmi::getAllDbusProperties(bus, chassisCapObject.second,
                                       chassisCapObject.first, chassisCapIntf);
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Failed to fetch Chassis Capability properties",
                        entry("ERROR=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }

    bool* chassisIntrusionFlag =
        std::get_if<bool>(&properties[chassisIntrusionProp]);
    if (chassisIntrusionFlag == nullptr)
    {
        log<level::ERR>("Error to get chassis Intrusion flags");
        return ipmi::responseUnspecifiedError();
    }
    bool* chassisFrontPanelFlag =
        std::get_if<bool>(&properties[chassisFrontPanelLockoutProp]);
    if (chassisFrontPanelFlag == nullptr)
    {
        log<level::ERR>("Error to get chassis intrusion flags");
        return ipmi::responseUnspecifiedError();
    }
    bool* chassisNMIFlag = std::get_if<bool>(&properties[chassisNMIProp]);
    if (chassisNMIFlag == nullptr)
    {
        log<level::ERR>("Error to get chassis NMI flags");
        return ipmi::responseUnspecifiedError();
    }
    bool* chassisPowerInterlockFlag =
        std::get_if<bool>(&properties[chassisPowerInterlockProp]);
    if (chassisPowerInterlockFlag == nullptr)
    {
        log<level::ERR>("Error to get chassis power interlock flags");
        return ipmi::responseUnspecifiedError();
    }
    uint8_t* chassisFRUInfoDevAddr =
        std::get_if<uint8_t>(&properties[chassisFRUDevAddrProp]);
    if (chassisFRUInfoDevAddr == nullptr)
    {
        log<level::ERR>("Error to get chassis FRU info device address");
        return ipmi::responseUnspecifiedError();
    }
    uint8_t* chassisSDRDevAddr =
        std::get_if<uint8_t>(&properties[chassisSDRDevAddrProp]);
    if (chassisSDRDevAddr == nullptr)
    {
        log<level::ERR>("Error to get chassis SDR device address");
        return ipmi::responseUnspecifiedError();
    }
    uint8_t* chassisSELDevAddr =
        std::get_if<uint8_t>(&properties[chassisSELDevAddrProp]);
    if (chassisSELDevAddr == nullptr)
    {
        log<level::ERR>("Error to get chassis SEL device address");
        return ipmi::responseUnspecifiedError();
    }
    uint8_t* chassisSMDevAddr =
        std::get_if<uint8_t>(&properties[chassisSMDevAddrProp]);
    if (chassisSMDevAddr == nullptr)
    {
        log<level::ERR>("Error to get chassis SM device address");
        return ipmi::responseUnspecifiedError();
    }
    uint8_t* chassisBridgeDevAddr =
        std::get_if<uint8_t>(&properties[chassisBridgeDevAddrProp]);
    if (chassisBridgeDevAddr == nullptr)
    {
        log<level::ERR>("Error to get chassis bridge device address");
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(*chassisIntrusionFlag, *chassisFrontPanelFlag,
                                 *chassisNMIFlag, *chassisPowerInterlockFlag, 0,
                                 *chassisFRUInfoDevAddr, *chassisSDRDevAddr,
                                 *chassisSELDevAddr, *chassisSMDevAddr,
                                 *chassisBridgeDevAddr);
}

/** @brief implements set chassis capalibities command
 *  @param intrusion        - chassis intrusion
 *  @param fpLockout        - frontpannel lockout
 *  @param reserved1        - skip one bit
 *  @param fruDeviceAddr    - chassis FRU info Device Address
 *  @param sdrDeviceAddr    - chassis SDR device address
 *  @param selDeviceAddr    - chassis SEL device address
 *  @param smDeviceAddr     - chassis system management device address
 *  @param bridgeDeviceAddr - chassis bridge device address
 *
 *  @returns IPMI completion code
 */
ipmi::RspType<> ipmiSetChassisCap(bool intrusion, bool fpLockout,
                                  uint6_t reserved1,

                                  uint8_t fruDeviceAddr,

                                  uint8_t sdrDeviceAddr,

                                  uint8_t selDeviceAddr,

                                  uint8_t smDeviceAddr,

                                  uint8_t bridgeDeviceAddr)
{

    // check input data
    if (reserved1 != 0)
    {
        log<level::ERR>("Unsupported request parameter");
        return ipmi::responseInvalidFieldRequest();
    }

    if ((fruDeviceAddr & ~chassisCapAddrMask) != 0)
    {
        log<level::ERR>("Unsupported request parameter(FRU Addr)",
                        entry("REQ=0x%x", fruDeviceAddr));
        return ipmi::responseInvalidFieldRequest();
    }
    if ((sdrDeviceAddr & ~chassisCapAddrMask) != 0)
    {
        log<level::ERR>("Unsupported request parameter(SDR Addr)",
                        entry("REQ=0x%x", sdrDeviceAddr));
        return ipmi::responseInvalidFieldRequest();
    }

    if ((selDeviceAddr & ~chassisCapAddrMask) != 0)
    {
        log<level::ERR>("Unsupported request parameter(SEL Addr)",
                        entry("REQ=0x%x", selDeviceAddr));
        return ipmi::responseInvalidFieldRequest();
    }

    if ((smDeviceAddr & ~chassisCapAddrMask) != 0)
    {
        log<level::ERR>("Unsupported request parameter(SM Addr)",
                        entry("REQ=0x%x", smDeviceAddr));
        return ipmi::responseInvalidFieldRequest();
    }

    if ((bridgeDeviceAddr & ~chassisCapAddrMask) != 0)
    {
        log<level::ERR>("Unsupported request parameter(Bridge Addr)",
                        entry("REQ=0x%x", bridgeDeviceAddr));
        return ipmi::responseInvalidFieldRequest();
    }

    try
    {
        sdbusplus::bus::bus bus(ipmid_get_sd_bus_connection());
        ipmi::DbusObjectInfo chassisCapObject =
            ipmi::getDbusObject(bus, chassisCapIntf);

        ipmi::setDbusProperty(bus, chassisCapObject.second,
                              chassisCapObject.first, chassisCapIntf,
                              chassisIntrusionProp, intrusion);

        ipmi::setDbusProperty(bus, chassisCapObject.second,
                              chassisCapObject.first, chassisCapIntf,
                              chassisFrontPanelLockoutProp, fpLockout);

        ipmi::setDbusProperty(bus, chassisCapObject.second,
                              chassisCapObject.first, chassisCapIntf,
                              chassisFRUDevAddrProp, fruDeviceAddr);

        ipmi::setDbusProperty(bus, chassisCapObject.second,
                              chassisCapObject.first, chassisCapIntf,
                              chassisSDRDevAddrProp, sdrDeviceAddr);

        ipmi::setDbusProperty(bus, chassisCapObject.second,
                              chassisCapObject.first, chassisCapIntf,
                              chassisSELDevAddrProp, selDeviceAddr);

        ipmi::setDbusProperty(bus, chassisCapObject.second,
                              chassisCapObject.first, chassisCapIntf,
                              chassisSMDevAddrProp, smDeviceAddr);

        ipmi::setDbusProperty(bus, chassisCapObject.second,
                              chassisCapObject.first, chassisCapIntf,
                              chassisBridgeDevAddrProp, bridgeDeviceAddr);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }
    return ipmi::responseSuccess();
}

//------------------------------------------
// Calls into Host State Manager Dbus object
//------------------------------------------
int initiate_state_transition(State::Host::Transition transition)
{
    // OpenBMC Host State Manager dbus framework
    constexpr auto HOST_STATE_MANAGER_ROOT = "/xyz/openbmc_project/state/host0";
    constexpr auto HOST_STATE_MANAGER_IFACE = "xyz.openbmc_project.State.Host";
    constexpr auto DBUS_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";
    constexpr auto PROPERTY = "RequestedHostTransition";

    // sd_bus error
    int rc = 0;
    char* busname = NULL;

    // SD Bus error report mechanism.
    sd_bus_error bus_error = SD_BUS_ERROR_NULL;

    // Gets a hook onto either a SYSTEM or SESSION bus
    sd_bus* bus_type = ipmid_get_sd_bus_connection();
    rc = mapper_get_service(bus_type, HOST_STATE_MANAGER_ROOT, &busname);
    if (rc < 0)
    {
        log<level::ERR>(
            "Failed to get bus name",
            entry("ERRNO=0x%X, OBJPATH=%s", -rc, HOST_STATE_MANAGER_ROOT));
        return rc;
    }

    // Convert to string equivalent of the passed in transition enum.
    auto request = State::convertForMessage(transition);

    rc = sd_bus_call_method(bus_type,                // On the system bus
                            busname,                 // Service to contact
                            HOST_STATE_MANAGER_ROOT, // Object path
                            DBUS_PROPERTY_IFACE,     // Interface name
                            "Set",                   // Method to be called
                            &bus_error,              // object to return error
                            nullptr,                 // Response buffer if any
                            "ssv",                   // Takes 3 arguments
                            HOST_STATE_MANAGER_IFACE, PROPERTY, "s",
                            request.c_str());
    if (rc < 0)
    {
        log<level::ERR>("Failed to initiate transition",
                        entry("ERRNO=0x%X, REQUEST=%s", -rc, request.c_str()));
    }
    else
    {
        log<level::INFO>("Transition request initiated successfully");
    }

    sd_bus_error_free(&bus_error);
    free(busname);

    return rc;
}

//------------------------------------------
// Set Enabled property to inform NMI source
// handling to trigger a NMI_OUT BSOD.
//------------------------------------------
int setNmiProperty(const bool value)
{
    constexpr const char* nmiSourceObjPath =
        "/xyz/openbmc_project/Chassis/Control/NMISource";
    constexpr const char* nmiSourceIntf =
        "xyz.openbmc_project.Chassis.Control.NMISource";
    std::string bmcSourceSignal = "xyz.openbmc_project.Chassis.Control."
                                  "NMISource.BMCSourceSignal.ChassisCmd";
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();

    try
    {
        auto service = ipmi::getService(*busp, nmiSourceIntf, nmiSourceObjPath);
        ipmi::setDbusProperty(*busp, service, nmiSourceObjPath, nmiSourceIntf,
                              "BMCSource", bmcSourceSignal);
        ipmi::setDbusProperty(*busp, service, nmiSourceObjPath, nmiSourceIntf,
                              "Enabled", value);
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Failed to trigger NMI_OUT",
                        entry("EXCEPTION=%s", e.what()));
        return -1;
    }

    return 0;
}

namespace power_policy
{

using namespace sdbusplus::xyz::openbmc_project::Control::Power::server;
using IpmiValue = uint8_t;
using DbusValue = RestorePolicy::Policy;

const std::map<DbusValue, IpmiValue> dbusToIpmi = {
    {RestorePolicy::Policy::AlwaysOff, 0x00},
    {RestorePolicy::Policy::Restore, 0x01},
    {RestorePolicy::Policy::AlwaysOn, 0x02}};

static constexpr uint8_t noChange = 0x03;
static constexpr uint8_t allSupport = 0x01 | 0x02 | 0x04;

/* helper function for Get Chassis Status Command
 */
std::optional<uint2_t> getPowerRestorePolicy()
{
    uint2_t restorePolicy = 0;
    using namespace chassis::internal;

    settings::Objects& objects = cache::getObjects();

    try
    {
        const auto& powerRestoreSetting =
            objects.map.at(powerRestoreIntf).front();
        ipmi::Value result = ipmi::getDbusProperty(
            *getSdBus(),
            objects.service(powerRestoreSetting, powerRestoreIntf).c_str(),
            powerRestoreSetting.c_str(), powerRestoreIntf,
            "PowerRestorePolicy");
        auto powerRestore = RestorePolicy::convertPolicyFromString(
            std::get<std::string>(result));
        restorePolicy = dbusToIpmi.at(powerRestore);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            "Failed to fetch pgood property", entry("ERROR=%s", e.what()),
            entry("PATH=%s", objects.map.at(powerRestoreIntf).front().c_str()),
            entry("INTERFACE=%s", powerRestoreIntf));
        cache::objectsPtr.reset();
        return std::nullopt;
    }
    return std::make_optional(restorePolicy);
}

/*
 * getPowerStatus
 * helper function for Get Chassis Status Command
 * return - optional value for pgood (no value on error)
 */
std::optional<bool> getPowerStatus()
{
    bool powerGood = false;
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    try
    {
        constexpr const char* chassisStatePath =
            "/xyz/openbmc_project/state/chassis0";
        constexpr const char* chassisStateIntf =
            "xyz.openbmc_project.State.Chassis";
        auto service =
            ipmi::getService(*busp, chassisStateIntf, chassisStatePath);

        ipmi::Value powerState =
            ipmi::getDbusProperty(*busp, service, chassisStatePath,
                                  chassisStateIntf, "CurrentPowerState");
        powerGood = std::get<std::string>(powerState) ==
                    "xyz.openbmc_project.State.Chassis.PowerState.On";
    }
    catch (const std::exception& e)
    {
        try
        {
            // FIXME: some legacy modules use the older path; try that next
            constexpr const char* legacyPwrCtrlObj =
                "/org/openbmc/control/power0";
            constexpr const char* legacyPwrCtrlIntf =
                "org.openbmc.control.Power";
            auto service =
                ipmi::getService(*busp, legacyPwrCtrlIntf, legacyPwrCtrlObj);

            ipmi::Value variant = ipmi::getDbusProperty(
                *busp, service, legacyPwrCtrlObj, legacyPwrCtrlIntf, "pgood");
            powerGood = static_cast<bool>(std::get<int>(variant));
        }
        catch (const std::exception& e)
        {
            log<level::ERR>("Failed to fetch pgood property",
                            entry("ERROR=%s", e.what()));
            return std::nullopt;
        }
    }
    return std::make_optional(powerGood);
}

/*
 * getACFailStatus
 * helper function for Get Chassis Status Command
 * return - bool value for ACFail (false on error)
 */
bool getACFailStatus()
{
    constexpr const char* powerControlObj =
        "/xyz/openbmc_project/Chassis/Control/Power0";
    constexpr const char* powerControlIntf =
        "xyz.openbmc_project.Chassis.Control.Power";
    bool acFail = false;
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    try
    {
        auto service =
            ipmi::getService(*bus, powerControlIntf, powerControlObj);

        ipmi::Value variant = ipmi::getDbusProperty(
            *bus, service, powerControlObj, powerControlIntf, "PFail");
        acFail = std::get<bool>(variant);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed to fetch PFail property",
                        entry("ERROR=%s", e.what()),
                        entry("PATH=%s", powerControlObj),
                        entry("INTERFACE=%s", powerControlIntf));
    }
    return acFail;
}
} // namespace power_policy

static std::optional<bool> getButtonEnabled(const std::string& buttonPath,
                                            const std::string& buttonIntf)
{
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    bool buttonDisabled = false;
    try
    {
        auto service = ipmi::getService(*busp, buttonIntf, buttonPath);
        ipmi::Value enabled = ipmi::getDbusProperty(*busp, service, buttonPath,
                                                    buttonIntf, "Enabled");
        buttonDisabled = !std::get<bool>(enabled);
    }
    catch (sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Fail to get button Enabled property",
                        entry("PATH=%s", buttonPath.c_str()),
                        entry("ERROR=%s", e.what()));
        return std::nullopt;
    }
    return std::make_optional(buttonDisabled);
}

static bool setButtonEnabled(ipmi::Context::ptr& ctx,
                             const std::string& buttonPath,
                             const std::string& buttonIntf, bool enable)
{
    std::string service;
    boost::system::error_code ec;
    ec = ipmi::getService(ctx, buttonIntf, buttonPath, service);
    if (!ec)
    {
        ec = ipmi::setDbusProperty(ctx, service, buttonPath, buttonIntf,
                                   "Enabled", enable);
    }
    if (ec)
    {
        log<level::ERR>("Fail to set button Enabled property",
                        entry("SERVICE=%s", service.c_str()),
                        entry("PATH=%s", buttonPath.c_str()),
                        entry("ERROR=%s", ec.message().c_str()));
        return false;
    }
    return true;
}

//----------------------------------------------------------------------
// Get Chassis Status commands
//----------------------------------------------------------------------
ipmi::RspType<bool,    // Power is on
              bool,    // Power overload
              bool,    // Interlock
              bool,    // power fault
              bool,    // power control fault
              uint2_t, // power restore policy
              bool,    // reserved

              bool, // AC failed
              bool, // last power down caused by a Power overload
              bool, // last power down caused by a power interlock
              bool, // last power down caused by power fault
              bool, // last ‘Power is on’ state was entered via IPMI command
              uint3_t, // reserved

              bool,    // Chassis intrusion active
              bool,    // Front Panel Lockout active
              bool,    // Drive Fault
              bool,    // Cooling/fan fault detected
              uint2_t, // Chassis Identify State
              bool,    // Chassis Identify command and state info supported
              bool,    // reserved

              bool, // Power off button disabled
              bool, // Reset button disabled
              bool, // Diagnostic Interrupt button disabled
              bool, // Standby (sleep) button disabled
              bool, // Power off button disable allowed
              bool, // Reset button disable allowed
              bool, // Diagnostic Interrupt button disable allowed
              bool  // Standby (sleep) button disable allowed
              >
    ipmiGetChassisStatus()
{
    using namespace chassis::internal;
    std::optional<uint2_t> restorePolicy =
        power_policy::getPowerRestorePolicy();
    std::optional<bool> powerGood = power_policy::getPowerStatus();
    if (!restorePolicy || !powerGood)
    {
        return ipmi::responseUnspecifiedError();
    }

    //  Front Panel Button Capabilities and disable/enable status(Optional)
    std::optional<bool> powerButtonReading =
        getButtonEnabled(powerButtonPath, powerButtonIntf);
    // allow disable if the interface is present
    bool powerButtonDisableAllow = static_cast<bool>(powerButtonReading);
    // default return the button is enabled (not disabled)
    bool powerButtonDisabled = false;
    if (powerButtonDisableAllow)
    {
        // return the real value of the button status, if present
        powerButtonDisabled = *powerButtonReading;
    }

    std::optional<bool> resetButtonReading =
        getButtonEnabled(resetButtonPath, resetButtonIntf);
    // allow disable if the interface is present
    bool resetButtonDisableAllow = static_cast<bool>(resetButtonReading);
    // default return the button is enabled (not disabled)
    bool resetButtonDisabled = false;
    if (resetButtonDisableAllow)
    {
        // return the real value of the button status, if present
        resetButtonDisabled = *resetButtonReading;
    }

    bool powerDownAcFailed = power_policy::getACFailStatus();

    // This response has a lot of hard-coded, unsupported fields
    // They are set to false or 0
    constexpr bool powerOverload = false;
    constexpr bool chassisInterlock = false;
    constexpr bool powerFault = false;
    constexpr bool powerControlFault = false;
    constexpr bool powerDownOverload = false;
    constexpr bool powerDownInterlock = false;
    constexpr bool powerDownPowerFault = false;
    constexpr bool powerStatusIPMI = false;
    constexpr bool chassisIntrusionActive = false;
    constexpr bool frontPanelLockoutActive = false;
    constexpr bool driveFault = false;
    constexpr bool coolingFanFault = false;
    // chassisIdentifySupport set because this command is implemented
    constexpr bool chassisIdentifySupport = true;
    uint2_t chassisIdentifyState = static_cast<uint2_t>(chassisIDState);
    constexpr bool diagButtonDisabled = false;
    constexpr bool sleepButtonDisabled = false;
    constexpr bool diagButtonDisableAllow = false;
    constexpr bool sleepButtonDisableAllow = false;

    return ipmi::responseSuccess(
        *powerGood, powerOverload, chassisInterlock, powerFault,
        powerControlFault, *restorePolicy,
        false, // reserved

        powerDownAcFailed, powerDownOverload, powerDownInterlock,
        powerDownPowerFault, powerStatusIPMI,
        uint3_t(0), // reserved

        chassisIntrusionActive, frontPanelLockoutActive, driveFault,
        coolingFanFault, chassisIdentifyState, chassisIdentifySupport,
        false, // reserved

        powerButtonDisabled, resetButtonDisabled, diagButtonDisabled,
        sleepButtonDisabled, powerButtonDisableAllow, resetButtonDisableAllow,
        diagButtonDisableAllow, sleepButtonDisableAllow);
}

enum class IpmiRestartCause
{
    Unknown = 0x0,
    RemoteCommand = 0x1,
    ResetButton = 0x2,
    PowerButton = 0x3,
    WatchdogTimer = 0x4,
    PowerPolicyAlwaysOn = 0x6,
    PowerPolicyPreviousState = 0x7,
    SoftReset = 0xa,
};

static IpmiRestartCause
    restartCauseToIpmiRestartCause(State::Host::RestartCause cause)
{
    switch (cause)
    {
        case State::Host::RestartCause::Unknown:
        {
            return IpmiRestartCause::Unknown;
        }
        case State::Host::RestartCause::RemoteCommand:
        {
            return IpmiRestartCause::RemoteCommand;
        }
        case State::Host::RestartCause::ResetButton:
        {
            return IpmiRestartCause::ResetButton;
        }
        case State::Host::RestartCause::PowerButton:
        {
            return IpmiRestartCause::PowerButton;
        }
        case State::Host::RestartCause::WatchdogTimer:
        {
            return IpmiRestartCause::WatchdogTimer;
        }
        case State::Host::RestartCause::PowerPolicyAlwaysOn:
        {
            return IpmiRestartCause::PowerPolicyAlwaysOn;
        }
        case State::Host::RestartCause::PowerPolicyPreviousState:
        {
            return IpmiRestartCause::PowerPolicyPreviousState;
        }
        case State::Host::RestartCause::SoftReset:
        {
            return IpmiRestartCause::SoftReset;
        }
        default:
        {
            return IpmiRestartCause::Unknown;
        }
    }
}

/*
 * getRestartCause
 * helper function for Get Host restart cause Command
 * return - optional value for RestartCause (no value on error)
 */
static std::optional<uint4_t> getRestartCause(ipmi::Context::ptr ctx)
{
    constexpr const char* restartCausePath =
        "/xyz/openbmc_project/control/host0/restart_cause";
    constexpr const char* restartCauseIntf =
        "xyz.openbmc_project.Control.Host.RestartCause";

    std::string service;
    boost::system::error_code ec =
        ipmi::getService(ctx, restartCauseIntf, restartCausePath, service);
    if (!ec)
    {
        std::string restartCauseStr;
        ec = ipmi::getDbusProperty<std::string>(
            ctx, service, restartCausePath, restartCauseIntf, "RestartCause",
            restartCauseStr);
        if (!ec)
        {
            auto cause =
                State::Host::convertRestartCauseFromString(restartCauseStr);
            return restartCauseToIpmiRestartCause(cause);
        }
    }

    log<level::ERR>("Failed to fetch RestartCause property",
                    entry("ERROR=%s", ec.message().c_str()),
                    entry("PATH=%s", restartCausePath),
                    entry("INTERFACE=%s", restartCauseIntf));
    return std::nullopt;
}

ipmi::RspType<uint4_t, // Restart Cause
              uint4_t, // reserved
              uint8_t  // channel number (not supported)
              >
    ipmiGetSystemRestartCause(ipmi::Context::ptr ctx)
{
    std::optional<uint4_t> cause = getRestartCause(ctx);
    if (!cause)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(cause.value(), RESERVED,
                                 CHANNEL_NOT_SUPPORTED);
}

//-------------------------------------------------------------
// Send a command to SoftPowerOff application to stop any timer
//-------------------------------------------------------------
int stop_soft_off_timer()
{
    constexpr auto iface = "org.freedesktop.DBus.Properties";
    constexpr auto soft_off_iface = "xyz.openbmc_project.Ipmi.Internal."
                                    "SoftPowerOff";

    constexpr auto property = "ResponseReceived";
    constexpr auto value = "xyz.openbmc_project.Ipmi.Internal."
                           "SoftPowerOff.HostResponse.HostShutdown";

    // Get the system bus where most system services are provided.
    auto bus = ipmid_get_sd_bus_connection();

    // Get the service name
    // TODO openbmc/openbmc#1661 - Mapper refactor
    //
    // See openbmc/openbmc#1743 for some details but high level summary is that
    // for now the code will directly call the soft off interface due to a
    // race condition with mapper usage
    //
    // char *busname = nullptr;
    // auto r = mapper_get_service(bus, SOFTOFF_OBJPATH, &busname);
    // if (r < 0)
    //{
    //    fprintf(stderr, "Failed to get %s bus name: %s\n",
    //            SOFTOFF_OBJPATH, -r);
    //    return r;
    //}

    // No error object or reply expected.
    int rc = sd_bus_call_method(bus, SOFTOFF_BUSNAME, SOFTOFF_OBJPATH, iface,
                                "Set", nullptr, nullptr, "ssv", soft_off_iface,
                                property, "s", value);
    if (rc < 0)
    {
        log<level::ERR>("Failed to set property in SoftPowerOff object",
                        entry("ERRNO=0x%X", -rc));
    }

    // TODO openbmc/openbmc#1661 - Mapper refactor
    // free(busname);
    return rc;
}

//----------------------------------------------------------------------
// Create file to indicate there is no need for softoff notification to host
//----------------------------------------------------------------------
void indicate_no_softoff_needed()
{
    fs::path path{HOST_INBAND_REQUEST_DIR};
    if (!fs::is_directory(path))
    {
        fs::create_directory(path);
    }

    // Add the host instance (default 0 for now) to the file name
    std::string file{HOST_INBAND_REQUEST_FILE};
    auto size = std::snprintf(nullptr, 0, file.c_str(), 0);
    size++; // null
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, file.c_str(), 0);

    // Append file name to directory and create it
    path /= buf.get();
    std::ofstream(path.c_str());
}

/** @brief Implementation of chassis control command
 *
 *  @param - chassisControl command byte
 *
 *  @return  Success or InvalidFieldRequest.
 */
ipmi::RspType<> ipmiChassisControl(uint8_t chassisControl)
{
    int rc = 0;
    switch (chassisControl)
    {
        case CMD_POWER_ON:
            rc = initiate_state_transition(State::Host::Transition::On);
            break;
        case CMD_POWER_OFF:
            // This path would be hit in 2 conditions.
            // 1: When user asks for power off using ipmi chassis command 0x04
            // 2: Host asking for power off post shutting down.

            // If it's a host requested power off, then need to nudge Softoff
            // application that it needs to stop the watchdog timer if running.
            // If it is a user requested power off, then this is not really
            // needed. But then we need to differentiate between user and host
            // calling this same command

            // For now, we are going ahead with trying to nudge the soft off and
            // interpret the failure to do so as a non softoff case
            rc = stop_soft_off_timer();

            // Only request the Off transition if the soft power off
            // application is not running
            if (rc < 0)
            {
                // First create a file to indicate to the soft off application
                // that it should not run. Not doing this will result in State
                // manager doing a default soft power off when asked for power
                // off.
                indicate_no_softoff_needed();

                // Now request the shutdown
                rc = initiate_state_transition(State::Host::Transition::Off);
            }
            else
            {
                log<level::INFO>("Soft off is running, so let shutdown target "
                                 "stop the host");
            }
            break;

        case CMD_HARD_RESET:
        case CMD_POWER_CYCLE:
            // SPEC has a section that says certain implementations can trigger
            // PowerOn if power is Off when a command to power cycle is
            // requested

            // First create a file to indicate to the soft off application
            // that it should not run since this is a direct user initiated
            // power reboot request (i.e. a reboot request that is not
            // originating via a soft power off SMS request)
            indicate_no_softoff_needed();

            rc = initiate_state_transition(State::Host::Transition::Reboot);
            break;

        case CMD_SOFT_OFF_VIA_OVER_TEMP:
            // Request Host State Manager to do a soft power off
            rc = initiate_state_transition(State::Host::Transition::Off);
            break;

        case CMD_PULSE_DIAGNOSTIC_INTR:
            rc = setNmiProperty(true);
            break;

        default:
        {
            log<level::ERR>("Invalid Chassis Control command",
                            entry("CMD=0x%X", chassisControl));
            return ipmi::responseInvalidFieldRequest();
        }
    }

    return ((rc < 0) ? ipmi::responseUnspecifiedError()
                     : ipmi::responseSuccess());
}

/** @brief Return D-Bus connection string to enclosure identify LED object
 *
 *  @param[in, out] connection - connection to D-Bus object
 *  @return a IPMI return code
 */
std::string getEnclosureIdentifyConnection()
{
    // lookup enclosure_identify group owner(s) in mapper
    auto mapperCall = chassis::internal::dbus.new_method_call(
        ipmi::MAPPER_BUS_NAME, ipmi::MAPPER_OBJ, ipmi::MAPPER_INTF,
        "GetObject");

    mapperCall.append(identify_led_object_name);
    static const std::vector<std::string> interfaces = {
        "xyz.openbmc_project.Led.Group"};
    mapperCall.append(interfaces);
    auto mapperReply = chassis::internal::dbus.call(mapperCall);
    if (mapperReply.is_method_error())
    {
        log<level::ERR>("Chassis Identify: Error communicating to mapper.");
        elog<InternalFailure>();
    }
    std::vector<std::pair<std::string, std::vector<std::string>>> mapperResp;
    mapperReply.read(mapperResp);

    if (mapperResp.size() != encIdentifyObjectsSize)
    {
        log<level::ERR>(
            "Invalid number of enclosure identify objects.",
            entry("ENC_IDENTITY_OBJECTS_SIZE=%d", mapperResp.size()));
        elog<InternalFailure>();
    }
    auto pair = mapperResp[encIdentifyObjectsSize - 1];
    return pair.first;
}

/** @brief Turn On/Off enclosure identify LED
 *
 *  @param[in] flag - true to turn on LED, false to turn off
 *  @return a IPMI return code
 */
void enclosureIdentifyLed(bool flag)
{
    using namespace chassis::internal;
    std::string connection = std::move(getEnclosureIdentifyConnection());
    auto msg = std::string("enclosureIdentifyLed(") +
               boost::lexical_cast<std::string>(flag) + ")";
    log<level::DEBUG>(msg.c_str());
    auto led =
        dbus.new_method_call(connection.c_str(), identify_led_object_name,
                             "org.freedesktop.DBus.Properties", "Set");
    led.append("xyz.openbmc_project.Led.Group", "Asserted",
               std::variant<bool>(flag));
    auto ledReply = dbus.call(led);
    if (ledReply.is_method_error())
    {
        log<level::ERR>("Chassis Identify: Error Setting State On/Off\n",
                        entry("LED_STATE=%d", flag));
        elog<InternalFailure>();
    }
}

/** @brief Callback method to turn off LED
 */
void enclosureIdentifyLedOff()
{
    try
    {
        chassisIDState = ChassisIDState::off;
        enclosureIdentifyLed(false);
    }
    catch (const InternalFailure& e)
    {
        report<InternalFailure>();
    }
}

/** @brief Create timer to turn on and off the enclosure LED
 */
void createIdentifyTimer()
{
    if (!identifyTimer)
    {
        identifyTimer =
            std::make_unique<phosphor::Timer>(enclosureIdentifyLedOff);
    }
}

ipmi::RspType<> ipmiChassisIdentify(std::optional<uint8_t> interval,
                                    std::optional<uint8_t> force)
{
    uint8_t identifyInterval = interval.value_or(DEFAULT_IDENTIFY_TIME_OUT);
    bool forceIdentify = force.value_or(0) & 0x01;

    if (identifyInterval || forceIdentify)
    {
        // stop the timer if already started;
        // for force identify we should not turn off LED
        identifyTimer->stop();
        try
        {
            chassisIDState = ChassisIDState::temporaryOn;
            enclosureIdentifyLed(true);
        }
        catch (const InternalFailure& e)
        {
            report<InternalFailure>();
            return ipmi::responseResponseError();
        }

        if (forceIdentify)
        {
            chassisIDState = ChassisIDState::indefiniteOn;
            return ipmi::responseSuccess();
        }
        // start the timer
        auto time = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::seconds(identifyInterval));
        identifyTimer->start(time);
    }
    else if (!identifyInterval)
    {
        identifyTimer->stop();
        enclosureIdentifyLedOff();
    }
    return ipmi::responseSuccess();
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
#ifdef ENABLE_BOOT_FLAG_SAFE_MODE_SUPPORT
    {0x03, Mode::Modes::Safe},
#endif // ENABLE_BOOT_SAFE_MODE_SUPPORT
    {0x06, Mode::Modes::Setup},
    {ipmiDefault, Mode::Modes::Regular}};

std::map<Source::Sources, IpmiValue> sourceDbusToIpmi = {
    {Source::Sources::Network, 0x01},
    {Source::Sources::Disk, 0x02},
    {Source::Sources::ExternalMedia, 0x05},
    {Source::Sources::RemovableMedia, 0x0f},
    {Source::Sources::Default, ipmiDefault}};

std::map<Mode::Modes, IpmiValue> modeDbusToIpmi = {
#ifdef ENABLE_BOOT_FLAG_SAFE_MODE_SUPPORT
    {Mode::Modes::Safe, 0x03},
#endif // ENABLE_BOOT_SAFE_MODE_SUPPORT
    {Mode::Modes::Setup, 0x06},
    {Mode::Modes::Regular, ipmiDefault}};

} // namespace boot_options

/** @brief Set the property value for boot source
 *  @param[in] source - boot source value
 *  @return On failure return IPMI error.
 */
static ipmi_ret_t setBootSource(const Source::Sources& source)
{
    using namespace chassis::internal;
    using namespace chassis::internal::cache;
    std::variant<std::string> property = convertForMessage(source);
    settings::Objects& objects = getObjects();
    auto bootSetting = settings::boot::setting(objects, bootSourceIntf);
    const auto& bootSourceSetting = std::get<settings::Path>(bootSetting);
    auto method = dbus.new_method_call(
        objects.service(bootSourceSetting, bootSourceIntf).c_str(),
        bootSourceSetting.c_str(), ipmi::PROP_INTF, "Set");
    method.append(bootSourceIntf, "BootSource", property);
    auto reply = dbus.call(method);
    if (reply.is_method_error())
    {
        log<level::ERR>("Error in BootSource Set");
        report<InternalFailure>();
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    return IPMI_CC_OK;
}

/** @brief Set the property value for boot mode
 *  @param[in] mode - boot mode value
 *  @return On failure return IPMI error.
 */
static ipmi_ret_t setBootMode(const Mode::Modes& mode)
{
    using namespace chassis::internal;
    using namespace chassis::internal::cache;
    std::variant<std::string> property = convertForMessage(mode);
    settings::Objects& objects = getObjects();
    auto bootSetting = settings::boot::setting(objects, bootModeIntf);
    const auto& bootModeSetting = std::get<settings::Path>(bootSetting);
    auto method = dbus.new_method_call(
        objects.service(bootModeSetting, bootModeIntf).c_str(),
        bootModeSetting.c_str(), ipmi::PROP_INTF, "Set");
    method.append(bootModeIntf, "BootMode", property);
    auto reply = dbus.call(method);
    if (reply.is_method_error())
    {
        log<level::ERR>("Error in BootMode Set");
        report<InternalFailure>();
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    return IPMI_CC_OK;
}

ipmi_ret_t ipmi_chassis_get_sys_boot_options(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                             ipmi_request_t request,
                                             ipmi_response_t response,
                                             ipmi_data_len_t data_len,
                                             ipmi_context_t context)
{
    using namespace boot_options;
    ipmi_ret_t rc = IPMI_CC_PARM_NOT_SUPPORTED;
    char* p = NULL;
    get_sys_boot_options_response_t* resp =
        (get_sys_boot_options_response_t*)response;
    get_sys_boot_options_t* reqptr = (get_sys_boot_options_t*)request;
    IpmiValue bootOption = ipmiDefault;

    std::memset(resp, 0, sizeof(*resp));
    resp->version = SET_PARM_VERSION;
    resp->parm = 5;
    resp->data[0] = SET_PARM_BOOT_FLAGS_VALID_ONE_TIME;

    /*
     * Parameter #5 means boot flags. Please refer to 28.13 of ipmi doc.
     * This is the only parameter used by petitboot.
     */
    if (reqptr->parameter ==
        static_cast<uint8_t>(BootOptionParameter::BOOT_FLAGS))
    {

        *data_len = static_cast<uint8_t>(BootOptionResponseSize::BOOT_FLAGS);
        using namespace chassis::internal;
        using namespace chassis::internal::cache;

        try
        {
            settings::Objects& objects = getObjects();
            auto bootSetting = settings::boot::setting(objects, bootSourceIntf);
            const auto& bootSourceSetting =
                std::get<settings::Path>(bootSetting);
            auto oneTimeEnabled =
                std::get<settings::boot::OneTimeEnabled>(bootSetting);
            auto method = dbus.new_method_call(
                objects.service(bootSourceSetting, bootSourceIntf).c_str(),
                bootSourceSetting.c_str(), ipmi::PROP_INTF, "Get");
            method.append(bootSourceIntf, "BootSource");
            auto reply = dbus.call(method);
            if (reply.is_method_error())
            {
                log<level::ERR>("Error in BootSource Get");
                report<InternalFailure>();
                *data_len = 0;
                return IPMI_CC_UNSPECIFIED_ERROR;
            }
            std::variant<std::string> result;
            reply.read(result);
            auto bootSource =
                Source::convertSourcesFromString(std::get<std::string>(result));

            bootSetting = settings::boot::setting(objects, bootModeIntf);
            const auto& bootModeSetting = std::get<settings::Path>(bootSetting);
            method = dbus.new_method_call(
                objects.service(bootModeSetting, bootModeIntf).c_str(),
                bootModeSetting.c_str(), ipmi::PROP_INTF, "Get");
            method.append(bootModeIntf, "BootMode");
            reply = dbus.call(method);
            if (reply.is_method_error())
            {
                log<level::ERR>("Error in BootMode Get");
                report<InternalFailure>();
                *data_len = 0;
                return IPMI_CC_UNSPECIFIED_ERROR;
            }
            reply.read(result);
            auto bootMode =
                Mode::convertModesFromString(std::get<std::string>(result));

            bootOption = sourceDbusToIpmi.at(bootSource);
            if ((Mode::Modes::Regular == bootMode) &&
                (Source::Sources::Default == bootSource))
            {
                bootOption = ipmiDefault;
            }
            else if (Source::Sources::Default == bootSource)
            {
                bootOption = modeDbusToIpmi.at(bootMode);
            }
            resp->data[1] = (bootOption << 2);

            resp->data[0] = oneTimeEnabled
                                ? SET_PARM_BOOT_FLAGS_VALID_ONE_TIME
                                : SET_PARM_BOOT_FLAGS_VALID_PERMANENT;

            rc = IPMI_CC_OK;
        }
        catch (InternalFailure& e)
        {
            cache::objectsPtr.reset();
            report<InternalFailure>();
            *data_len = 0;
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
    }
    else if (reqptr->parameter ==
             static_cast<uint8_t>(BootOptionParameter::OPAL_NETWORK_SETTINGS))
    {

        *data_len =
            static_cast<uint8_t>(BootOptionResponseSize::OPAL_NETWORK_SETTINGS);

        resp->parm =
            static_cast<uint8_t>(BootOptionParameter::OPAL_NETWORK_SETTINGS);

        int ret = getHostNetworkData(resp);

        if (ret < 0)
        {

            log<level::ERR>(
                "getHostNetworkData failed for get_sys_boot_options.");
            rc = IPMI_CC_UNSPECIFIED_ERROR;
        }
        else
            rc = IPMI_CC_OK;
    }

    else
    {
        log<level::ERR>("Unsupported parameter",
                        entry("PARAM=0x%x", reqptr->parameter));
    }

    if (p)
        free(p);

    if (rc == IPMI_CC_OK)
    {
        *data_len += 2;
    }

    return rc;
}

ipmi_ret_t ipmi_chassis_set_sys_boot_options(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                             ipmi_request_t request,
                                             ipmi_response_t response,
                                             ipmi_data_len_t data_len,
                                             ipmi_context_t context)
{
    using namespace boot_options;
    ipmi_ret_t rc = IPMI_CC_OK;
    set_sys_boot_options_t* reqptr = (set_sys_boot_options_t*)request;

    std::printf("IPMI SET_SYS_BOOT_OPTIONS reqptr->parameter =[%d]\n",
                reqptr->parameter);

    // This IPMI command does not have any resposne data
    *data_len = 0;

    /*  000101
     * Parameter #5 means boot flags. Please refer to 28.13 of ipmi doc.
     * This is the only parameter used by petitboot.
     */

    if (reqptr->parameter == (uint8_t)BootOptionParameter::BOOT_FLAGS)
    {
        IpmiValue bootOption = ((reqptr->data[1] & 0x3C) >> 2);
        using namespace chassis::internal;
        using namespace chassis::internal::cache;
        auto oneTimeEnabled = false;
        constexpr auto enabledIntf = "xyz.openbmc_project.Object.Enable";
        constexpr auto oneTimePath =
            "/xyz/openbmc_project/control/host0/boot/one_time";

        try
        {
            bool permanent =
                (reqptr->data[0] & SET_PARM_BOOT_FLAGS_PERMANENT) ==
                SET_PARM_BOOT_FLAGS_PERMANENT;

            settings::Objects& objects = getObjects();

            auto bootSetting = settings::boot::setting(objects, bootSourceIntf);

            oneTimeEnabled =
                std::get<settings::boot::OneTimeEnabled>(bootSetting);

            /*
             * Check if the current boot setting is onetime or permanent, if the
             * request in the command is otherwise, then set the "Enabled"
             * property in one_time object path to 'True' to indicate onetime
             * and 'False' to indicate permanent.
             *
             * Once the onetime/permanent setting is applied, then the bootMode
             * and bootSource is updated for the corresponding object.
             */
            if ((permanent && oneTimeEnabled) ||
                (!permanent && !oneTimeEnabled))
            {
                auto service = ipmi::getService(dbus, enabledIntf, oneTimePath);

                ipmi::setDbusProperty(dbus, service, oneTimePath, enabledIntf,
                                      "Enabled", !permanent);
            }

            auto modeItr = modeIpmiToDbus.find(bootOption);
            auto sourceItr = sourceIpmiToDbus.find(bootOption);
            if (sourceIpmiToDbus.end() != sourceItr)
            {
                rc = setBootSource(sourceItr->second);
                if (rc != IPMI_CC_OK)
                {
                    *data_len = 0;
                    return rc;
                }
                // If a set boot device is mapping to a boot source, then reset
                // the boot mode D-Bus property to default.
                // This way the ipmid code can determine which property is not
                // at the default value
                if (sourceItr->second != Source::Sources::Default)
                {
                    setBootMode(Mode::Modes::Regular);
                }
            }
            if (modeIpmiToDbus.end() != modeItr)
            {
                rc = setBootMode(modeItr->second);
                if (rc != IPMI_CC_OK)
                {
                    *data_len = 0;
                    return rc;
                }
                // If a set boot device is mapping to a boot mode, then reset
                // the boot source D-Bus property to default.
                // This way the ipmid code can determine which property is not
                // at the default value
                if (modeItr->second != Mode::Modes::Regular)
                {
                    setBootSource(Source::Sources::Default);
                }
            }
            if ((modeIpmiToDbus.end() == modeItr) &&
                (sourceIpmiToDbus.end() == sourceItr))
            {
                // return error if boot option is not supported
                *data_len = 0;
                return IPMI_CC_INVALID_FIELD_REQUEST;
            }
        }
        catch (InternalFailure& e)
        {
            objectsPtr.reset();
            report<InternalFailure>();
            *data_len = 0;
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
    }
    else if (reqptr->parameter ==
             (uint8_t)BootOptionParameter::OPAL_NETWORK_SETTINGS)
    {

        int ret = setHostNetworkData(reqptr);
        if (ret < 0)
        {
            log<level::ERR>(
                "setHostNetworkData failed for set_sys_boot_options");
            rc = IPMI_CC_UNSPECIFIED_ERROR;
        }
    }
    else if (reqptr->parameter ==
             static_cast<uint8_t>(BootOptionParameter::BOOT_INFO))
    {
        // Handle parameter #4 and return command completed normally
        // (IPMI_CC_OK). There is no implementation in OpenBMC for this
        // parameter. This is added to support the ipmitool command `chassis
        // bootdev` which sends set on parameter #4, before setting the boot
        // flags.
        rc = IPMI_CC_OK;
    }
    else
    {
        log<level::ERR>("Unsupported parameter",
                        entry("PARAM=0x%x", reqptr->parameter));
        rc = IPMI_CC_PARM_NOT_SUPPORTED;
    }

    return rc;
}

/** @brief implements Get POH counter command
 *  @parameter
 *   -  none
 *  @returns IPMI completion code plus response data
 *   - minPerCount - Minutes per count
 *   - counterReading - counter reading
 */
ipmi::RspType<uint8_t, // Minutes per count
              uint32_t // Counter reading
              >
    ipmiGetPOHCounter()
{
    // sd_bus error
    try
    {
        return ipmi::responseSuccess(static_cast<uint8_t>(poh::minutesPerCount),
                                     getPOHCounter());
    }
    catch (std::exception& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }
}

ipmi::RspType<uint3_t, // policy support
              uint5_t  // reserved
              >
    ipmiChassisSetPowerRestorePolicy(boost::asio::yield_context yield,
                                     uint3_t policy, uint5_t reserved)
{
    power_policy::DbusValue value =
        power_policy::RestorePolicy::Policy::AlwaysOff;

    if (reserved || (policy > power_policy::noChange))
    {
        phosphor::logging::log<level::ERR>(
            "Reserved request parameter",
            entry("REQ=0x%x", static_cast<int>(policy)));
        return ipmi::responseInvalidFieldRequest();
    }

    if (policy == power_policy::noChange)
    {
        // just return the supported policy
        return ipmi::responseSuccess(power_policy::allSupport, reserved);
    }

    for (auto const& it : power_policy::dbusToIpmi)
    {
        if (it.second == policy)
        {
            value = it.first;
            break;
        }
    }

    try
    {
        settings::Objects& objects = chassis::internal::cache::getObjects();
        const settings::Path& powerRestoreSetting =
            objects.map.at(chassis::internal::powerRestoreIntf).front();
        std::variant<std::string> property = convertForMessage(value);

        auto sdbusp = getSdBus();
        boost::system::error_code ec;
        sdbusp->yield_method_call<void>(
            yield, ec,
            objects
                .service(powerRestoreSetting,
                         chassis::internal::powerRestoreIntf)
                .c_str(),
            powerRestoreSetting, ipmi::PROP_INTF, "Set",
            chassis::internal::powerRestoreIntf, "PowerRestorePolicy",
            property);
        if (ec)
        {
            phosphor::logging::log<level::ERR>("Unspecified Error");
            return ipmi::responseUnspecifiedError();
        }
    }
    catch (InternalFailure& e)
    {
        chassis::internal::cache::objectsPtr.reset();
        report<InternalFailure>();
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(power_policy::allSupport, reserved);
}

ipmi::RspType<> ipmiSetFrontPanelButtonEnables(
    ipmi::Context::ptr ctx, bool disablePowerButton, bool disableResetButton,
    bool disableDiagButton, bool disableSleepButton, uint4_t reserved)
{
    using namespace chassis::internal;

    // set power button Enabled property
    bool success = setButtonEnabled(ctx, powerButtonPath, powerButtonIntf,
                                    !disablePowerButton);

    // set reset button Enabled property
    success &= setButtonEnabled(ctx, resetButtonPath, resetButtonIntf,
                                !disableResetButton);

    if (!success)
    {
        // not all buttons were successfully set
        return ipmi::responseUnspecifiedError();
    }
    return ipmi::responseSuccess();
}

void register_netfn_chassis_functions()
{
    createIdentifyTimer();

    // Get Chassis Capabilities
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnChassis,
                          ipmi::chassis::cmdGetChassisCapabilities,
                          ipmi::Privilege::User, ipmiGetChassisCap);

    // Set Front Panel Button Enables
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnChassis,
                          ipmi::chassis::cmdSetFrontPanelButtonEnables,
                          ipmi::Privilege::Admin,
                          ipmiSetFrontPanelButtonEnables);

    // Set Chassis Capabilities
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnChassis,
                          ipmi::chassis::cmdSetChassisCapabilities,
                          ipmi::Privilege::User, ipmiSetChassisCap);

    // <Get System Boot Options>
    ipmi_register_callback(NETFUN_CHASSIS, IPMI_CMD_GET_SYS_BOOT_OPTIONS, NULL,
                           ipmi_chassis_get_sys_boot_options,
                           PRIVILEGE_OPERATOR);

    // <Get Chassis Status>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnChassis,
                          ipmi::chassis::cmdGetChassisStatus,
                          ipmi::Privilege::User, ipmiGetChassisStatus);

    // <Chassis Get System Restart Cause>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnChassis,
                          ipmi::chassis::cmdGetSystemRestartCause,
                          ipmi::Privilege::User, ipmiGetSystemRestartCause);

    // <Chassis Control>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnChassis,
                          ipmi::chassis::cmdChassisControl,
                          ipmi::Privilege::Operator, ipmiChassisControl);

    // <Chassis Identify>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnChassis,
                          ipmi::chassis::cmdChassisIdentify,
                          ipmi::Privilege::Operator, ipmiChassisIdentify);

    // <Set System Boot Options>
    ipmi_register_callback(NETFUN_CHASSIS, IPMI_CMD_SET_SYS_BOOT_OPTIONS, NULL,
                           ipmi_chassis_set_sys_boot_options,
                           PRIVILEGE_OPERATOR);
    // <Get POH Counter>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnChassis,
                          ipmi::chassis::cmdGetPohCounter,
                          ipmi::Privilege::User, ipmiGetPOHCounter);

    // <Set Power Restore Policy>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnChassis,
                          ipmi::chassis::cmdSetPowerRestorePolicy,
                          ipmi::Privilege::Operator,
                          ipmiChassisSetPowerRestorePolicy);
}
