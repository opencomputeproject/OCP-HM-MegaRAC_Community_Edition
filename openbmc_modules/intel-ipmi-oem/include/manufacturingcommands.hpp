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

#include <ipmid/api-types.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/timer.hpp>
#include <variantvisitors.hpp>

#include <vector>

#define FAN_SENSOR_NOT_PRESENT (0 << 0)
#define FAN_SENSOR_PRESENT (1 << 0)
#define FAN_NOT_PRESENT (0 << 1)
#define FAN_PRESENT (1 << 1)

namespace ipmi
{

// TODO: Service names may change. Worth to consider dynamic detection.
static constexpr const char* fanService = "xyz.openbmc_project.FanSensor";
static constexpr const char* buttonService =
    "xyz.openbmc_project.Chassis.Buttons";
static constexpr const char* ledServicePrefix =
    "xyz.openbmc_project.LED.Controller.";

static constexpr const char* ledPathPrefix =
    "/xyz/openbmc_project/led/physical/";
static constexpr const char* fanPwmPath =
    "/xyz/openbmc_project/sensors/fan_pwm/Pwm_";
static constexpr const char* fanTachBasePath =
    "/xyz/openbmc_project/sensors/fan_tach/";

static constexpr const char* fanIntf = "xyz.openbmc_project.Sensor.Value";
static constexpr const char* buttonIntf = "xyz.openbmc_project.Chassis.Buttons";
static constexpr const char* ledIntf = "xyz.openbmc_project.Led.Physical";

static constexpr const char* intrusionService =
    "xyz.openbmc_project.IntrusionSensor";
static constexpr const char* intrusionPath =
    "/xyz/openbmc_project/Intrusion/Chassis_Intrusion";
static constexpr const char* intrusionIntf =
    "xyz.openbmc_project.Chassis.Intrusion";

static constexpr const char* busPropertyIntf =
    "org.freedesktop.DBus.Properties";
static constexpr const char* ledStateStr =
    "xyz.openbmc_project.Led.Physical.Action."; // Comes with postfix Off/On

static constexpr const char* specialModeService =
    "xyz.openbmc_project.SpecialMode";
static constexpr const char* specialModeObjPath =
    "/xyz/openbmc_project/security/special_mode";
static constexpr const char* specialModeIntf =
    "xyz.openbmc_project.Security.SpecialMode";

enum class SpecialMode : uint8_t
{
    none = 0,
    mfg = 1,
#ifdef BMC_VALIDATION_UNSECURE_FEATURE
    valUnsecure = 2
#endif
};

enum class SmActionGet : uint8_t
{
    sample = 0,
    ignore = 1,
    revert = 2
};

enum class SmActionSet : uint8_t
{
    forceDeasserted = 0,
    forceAsserted = 1,
    revert = 2
};

enum class SmSignalGet : uint8_t
{
    smPowerButton = 0,
    smResetButton = 1,
    smSleepButton,
    smNMIButton = 3,
    smChassisIntrusion = 4,
    smPowerGood,
    smPowerRequestGet,
    smSleepRequestGet,
    smFrbTimerHaltGet,
    smForceUpdate,
    smRingIndication,
    smCarrierDetect,
    smIdentifyButton = 0xc,
    smFanPwmGet = 0xd,
    smSignalReserved,
    smFanTachometerGet = 0xf,
    smNcsiDiag = 0x10,
    smGetSignalMax
};

enum class SmSignalSet : uint8_t
{
    smPowerLed = 0,
    smPowerFaultLed,
    smClusterLed,
    smDiskFaultLed,
    smCoolingFaultLed,
    smFanPowerSpeed = 5,
    smPowerRequestSet,
    smSleepRequestSet,
    smAcpiSci,
    smSpeaker,
    smFanPackFaultLed,
    smCpuFailLed,
    smDimmFailLed,
    smIdentifyLed,
    smHddLed,
    smSystemReadyLed,
    smLcdBacklight = 0x10,
    smSetSignalMax
};

/** @enum IntrusionStatus
.*
 *  Intrusion Status
 */
enum class IntrusionStatus : uint8_t
{
    normal = 0,
    hardwareIntrusion,
    tamperingDetected
};

struct SetSmSignalReq
{
    SmSignalSet Signal;
    uint8_t Instance;
    SmActionSet Action;
    uint8_t Value;
};

class LedProperty
{
    SmSignalSet signal;
    std::string name;
    std::string prevState;
    std::string currentState;
    bool isLocked;

  public:
    LedProperty(SmSignalSet signal_, std::string name_) :
        signal(signal_), name(name_), prevState(""), isLocked(false)
    {}

    LedProperty() = delete;

    SmSignalSet getSignal() const
    {
        return signal;
    }

    void setLock(const bool& lock)
    {
        isLocked = lock;
    }
    void setPrevState(const std::string& state)
    {
        prevState = state;
    }
    void setCurrState(const std::string& state)
    {
        currentState = state;
    }
    std::string getCurrState() const
    {
        return currentState;
    }
    bool getLock() const
    {
        return isLocked;
    }
    std::string getPrevState() const
    {
        return prevState;
    }
    std::string getName() const
    {
        return name;
    }
};

/** @class Manufacturing
 *
 *  @brief Implemet commands to support Manufacturing Test Mode.
 */
class Manufacturing
{
    std::string path;
    std::string gpioPaths[(uint8_t)SmSignalGet::smGetSignalMax];
    std::vector<LedProperty> ledPropertyList;
    void initData();

  public:
    Manufacturing();

    ipmi_return_codes detectAccessLvl(ipmi_request_t request,
                                      ipmi_data_len_t req_len);

    LedProperty* findLedProperty(const SmSignalSet& signal)
    {
        auto it = std::find_if(ledPropertyList.begin(), ledPropertyList.end(),
                               [&signal](const LedProperty& led) {
                                   return led.getSignal() == signal;
                               });
        if (it != ledPropertyList.end())
        {
            return &(*it);
        }
        return nullptr;
    }

    std::string getLedPropertyName(const SmSignalSet signal)
    {
        LedProperty* led = findLedProperty(signal);
        if (led != nullptr)
        {
            return led->getName();
        }
        else
        {
            return "";
        }
    }

    int8_t getProperty(const std::string& service, const std::string& path,
                       const std::string& interface,
                       const std::string& propertyName, ipmi::Value* value);
    int8_t setProperty(const std::string& service, const std::string& path,
                       const std::string& interface,
                       const std::string& propertyName, ipmi::Value value);
    int8_t disablePidControlService(const bool disable);

    void revertTimerHandler();

    SpecialMode getMfgMode(void)
    {
        ipmi::Value mode;
        if (getProperty(specialModeService, specialModeObjPath, specialModeIntf,
                        "SpecialMode", &mode) != 0)
        {
            return SpecialMode::none;
        }
        if (std::get<std::string>(mode) ==
            "xyz.openbmc_project.Control.Security.SpecialMode.Modes."
            "Manufacturing")
        {
            return SpecialMode::mfg;
        }
#ifdef BMC_VALIDATION_UNSECURE_FEATURE
        if (std::get<std::string>(mode) ==
            "xyz.openbmc_project.Control.Security.SpecialMode.Modes."
            "ValidationUnsecure")
        {
            return SpecialMode::valUnsecure;
        }
#endif
        return SpecialMode::none;
    }

    bool revertFanPWM = false;
    bool revertLedCallback = false;
    phosphor::Timer revertTimer;
    int mtmTestBeepFd = -1;
};

} // namespace ipmi
