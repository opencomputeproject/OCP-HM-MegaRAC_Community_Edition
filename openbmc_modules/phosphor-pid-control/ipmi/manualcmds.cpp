/**
 * Copyright 2017 Google Inc.
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

#include <ipmid/api.h>

#include <ipmid/iana.hpp>
#include <ipmid/oemopenbmc.hpp>
#include <ipmid/oemrouter.hpp>
#include <map>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <string>
#include <tuple>
#include <variant>

enum ManualSubCmd
{
    GET_CONTROL_STATE = 0,
    SET_CONTROL_STATE = 1,
    GET_FAILSAFE_STATE = 2,
};

struct FanCtrlRequest
{
    uint8_t command;
    uint8_t zone;
} __attribute__((packed));

struct FanCtrlRequestSet
{
    uint8_t command;
    uint8_t zone;
    uint8_t value;
} __attribute__((packed));

static constexpr auto objectPath = "/xyz/openbmc_project/settings/fanctrl/zone";
static constexpr auto busName = "xyz.openbmc_project.State.FanCtrl";
static constexpr auto intf = "xyz.openbmc_project.Control.Mode";
static constexpr auto manualProperty = "Manual";
static constexpr auto failsafeProperty = "FailSafe";
static constexpr auto propertiesintf = "org.freedesktop.DBus.Properties";

using Property = std::string;
using Value = std::variant<bool>;
using PropertyMap = std::map<Property, Value>;

/* The following was copied directly from my manual thread handler. */
static std::string getControlPath(int8_t zone)
{
    return std::string(objectPath) + std::to_string(zone);
}

/*
 * busctl call xyz.openbmc_project.State.FanCtrl \
 *     /xyz/openbmc_project/settings/fanctrl/zone1 \
 *     org.freedesktop.DBus.Properties \
 *     GetAll \
 *     s \
 *     xyz.openbmc_project.Control.Mode
 * a{sv} 2 "Manual" b false "FailSafe" b false
 */

static ipmi_ret_t getFanCtrlProperty(uint8_t zoneId, bool* value,
                                     const std::string& property)
{
    std::string path = getControlPath(zoneId);

    auto propertyReadBus = sdbusplus::bus::new_system();
    auto pimMsg = propertyReadBus.new_method_call(busName, path.c_str(),
                                                  propertiesintf, "GetAll");
    pimMsg.append(intf);

    try
    {
        PropertyMap propMap;

        /* a method could error but the call not error. */
        auto valueResponseMsg = propertyReadBus.call(pimMsg);

        valueResponseMsg.read(propMap);

        *value = std::get<bool>(propMap[property]);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        return IPMI_CC_INVALID;
    }

    return IPMI_CC_OK;
}

static ipmi_ret_t getFailsafeModeState(const uint8_t* reqBuf, uint8_t* replyBuf,
                                       size_t* dataLen)
{
    ipmi_ret_t rc = IPMI_CC_OK;
    bool current;

    if (*dataLen < sizeof(struct FanCtrlRequest))
    {
        return IPMI_CC_INVALID;
    }

    const auto request =
        reinterpret_cast<const struct FanCtrlRequest*>(&reqBuf[0]);

    rc = getFanCtrlProperty(request->zone, &current, failsafeProperty);
    if (rc)
    {
        return rc;
    }

    *replyBuf = (uint8_t)current;
    *dataLen = sizeof(uint8_t);
    return rc;
}

/*
 * <method name="GetAll">
 *   <arg name="interface" direction="in" type="s"/>
 *   <arg name="properties" direction="out" type="a{sv}"/>
 * </method>
 */
static ipmi_ret_t getManualModeState(const uint8_t* reqBuf, uint8_t* replyBuf,
                                     size_t* dataLen)
{
    ipmi_ret_t rc = IPMI_CC_OK;
    bool current;

    if (*dataLen < sizeof(struct FanCtrlRequest))
    {
        return IPMI_CC_INVALID;
    }

    const auto request =
        reinterpret_cast<const struct FanCtrlRequest*>(&reqBuf[0]);

    rc = getFanCtrlProperty(request->zone, &current, manualProperty);
    if (rc)
    {
        return rc;
    }

    *replyBuf = (uint8_t)current;
    *dataLen = sizeof(uint8_t);
    return rc;
}

/*
 * <method name="Set">
 *   <arg name="interface" direction="in" type="s"/>
 *   <arg name="property" direction="in" type="s"/>
 *   <arg name="value" direction="in" type="v"/>
 * </method>
 */
static ipmi_ret_t setManualModeState(const uint8_t* reqBuf, uint8_t* replyBuf,
                                     size_t* dataLen)
{
    ipmi_ret_t rc = IPMI_CC_OK;
    if (*dataLen < sizeof(struct FanCtrlRequestSet))
    {
        return IPMI_CC_INVALID;
    }

    using Value = std::variant<bool>;

    const auto request =
        reinterpret_cast<const struct FanCtrlRequestSet*>(&reqBuf[0]);

    /* 0 is false, 1 is true */
    bool setValue = static_cast<bool>(request->value);
    Value v{setValue};

    auto PropertyWriteBus = sdbusplus::bus::new_system();

    std::string path = getControlPath(request->zone);

    auto pimMsg = PropertyWriteBus.new_method_call(busName, path.c_str(),
                                                   propertiesintf, "Set");
    pimMsg.append(intf);
    pimMsg.append(manualProperty);
    pimMsg.append(v);

    try
    {
        PropertyWriteBus.call_noreply(pimMsg);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        rc = IPMI_CC_INVALID;
    }
    /* TODO(venture): Should sanity check the result. */

    return rc;
}

/* Three command packages: get, set true, set false */
static ipmi_ret_t manualModeControl(ipmi_cmd_t cmd, const uint8_t* reqBuf,
                                    uint8_t* replyCmdBuf, size_t* dataLen)
{
    ipmi_ret_t rc = IPMI_CC_OK;
    // FanCtrlRequest is the smaller of the requests, so it's at a minimum.
    if (*dataLen < sizeof(struct FanCtrlRequest))
    {
        return IPMI_CC_INVALID;
    }

    const auto request =
        reinterpret_cast<const struct FanCtrlRequest*>(&reqBuf[0]);

    switch (request->command)
    {
        case GET_CONTROL_STATE:
            return getManualModeState(reqBuf, replyCmdBuf, dataLen);
        case SET_CONTROL_STATE:
            return setManualModeState(reqBuf, replyCmdBuf, dataLen);
        case GET_FAILSAFE_STATE:
            return getFailsafeModeState(reqBuf, replyCmdBuf, dataLen);
        default:
            rc = IPMI_CC_INVALID;
    }

    return rc;
}

void setupGlobalOemFanControl() __attribute__((constructor));

void setupGlobalOemFanControl()
{
    oem::Router* router = oem::mutableRouter();

    fprintf(stderr,
            "Registering OEM:[%#08X], Cmd:[%#04X] for Manual Zone Control\n",
            oem::obmcOemNumber, oem::Cmd::fanManualCmd);

    router->registerHandler(oem::obmcOemNumber, oem::Cmd::fanManualCmd,
                            manualModeControl);
}
