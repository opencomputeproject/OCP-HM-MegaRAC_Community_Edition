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

#include <linux/input.h>

#include <boost/container/flat_map.hpp>
#include <ipmid/api.hpp>
#include <manufacturingcommands.hpp>
#include <oemcommands.hpp>

#include <filesystem>
#include <fstream>

namespace ipmi
{

Manufacturing mtm;

static auto revertTimeOut =
    std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::seconds(60)); // 1 minute timeout

static constexpr uint8_t slotAddressTypeBus = 0;
static constexpr uint8_t slotAddressTypeUniqueid = 1;
static constexpr uint8_t slotI2CMaxReadSize = 35;

static constexpr const char* callbackMgrService =
    "xyz.openbmc_project.CallbackManager";
static constexpr const char* callbackMgrIntf =
    "xyz.openbmc_project.CallbackManager";
static constexpr const char* callbackMgrObjPath =
    "/xyz/openbmc_project/CallbackManager";
static constexpr const char* retriggerLedUpdate = "RetriggerLEDUpdate";

const static constexpr char* systemDService = "org.freedesktop.systemd1";
const static constexpr char* systemDObjPath = "/org/freedesktop/systemd1";
const static constexpr char* systemDMgrIntf =
    "org.freedesktop.systemd1.Manager";
const static constexpr char* pidControlService = "phosphor-pid-control.service";

static inline Cc resetMtmTimer(ipmi::Context::ptr ctx)
{
    boost::system::error_code ec;
    ctx->bus->yield_method_call<>(ctx->yield, ec, specialModeService,
                                  specialModeObjPath, specialModeIntf,
                                  "ResetTimer");
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to reset the manufacturing mode timer");
        return ccUnspecifiedError;
    }
    return ccSuccess;
}

int getGpioPathForSmSignal(const SmSignalGet signal, std::string& path)
{
    switch (signal)
    {
        case SmSignalGet::smPowerButton:
            path = "/xyz/openbmc_project/chassis/buttons/power";
            break;
        case SmSignalGet::smResetButton:
            path = "/xyz/openbmc_project/chassis/buttons/reset";
            break;
        case SmSignalGet::smNMIButton:
            path = "/xyz/openbmc_project/chassis/buttons/nmi";
            break;
        case SmSignalGet::smIdentifyButton:
            path = "/xyz/openbmc_project/chassis/buttons/id";
            break;
        default:
            return -1;
            break;
    }
    return 0;
}

ipmi_ret_t ledStoreAndSet(SmSignalSet signal, const std::string& setState)
{
    LedProperty* ledProp = mtm.findLedProperty(signal);
    if (ledProp == nullptr)
    {
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    std::string ledName = ledProp->getName();
    std::string ledService = ledServicePrefix + ledName;
    std::string ledPath = ledPathPrefix + ledName;
    ipmi::Value presentState;

    if (false == ledProp->getLock())
    {
        if (mtm.getProperty(ledService.c_str(), ledPath.c_str(), ledIntf,
                            "State", &presentState) != 0)
        {
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
        ledProp->setPrevState(std::get<std::string>(presentState));
        ledProp->setLock(true);
        if (signal == SmSignalSet::smPowerFaultLed ||
            signal == SmSignalSet::smSystemReadyLed)
        {
            mtm.revertLedCallback = true;
        }
    }
    if (mtm.setProperty(ledService, ledPath, ledIntf, "State",
                        ledStateStr + setState) != 0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    return IPMI_CC_OK;
}

ipmi_ret_t ledRevert(SmSignalSet signal)
{
    LedProperty* ledProp = mtm.findLedProperty(signal);
    if (ledProp == nullptr)
    {
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }
    if (true == ledProp->getLock())
    {
        ledProp->setLock(false);
        if (signal == SmSignalSet::smPowerFaultLed ||
            signal == SmSignalSet::smSystemReadyLed)
        {
            try
            {
                ipmi::method_no_args::callDbusMethod(
                    *getSdBus(), callbackMgrService, callbackMgrObjPath,
                    callbackMgrIntf, retriggerLedUpdate);
            }
            catch (sdbusplus::exception_t& e)
            {
                return IPMI_CC_UNSPECIFIED_ERROR;
            }
            mtm.revertLedCallback = false;
        }
        else
        {
            std::string ledName = ledProp->getName();
            std::string ledService = ledServicePrefix + ledName;
            std::string ledPath = ledPathPrefix + ledName;
            if (mtm.setProperty(ledService, ledPath, ledIntf, "State",
                                ledProp->getPrevState()) != 0)
            {
                return IPMI_CC_UNSPECIFIED_ERROR;
            }
        }
    }
    return IPMI_CC_OK;
}

void Manufacturing::initData()
{
    ledPropertyList.push_back(
        LedProperty(SmSignalSet::smPowerFaultLed, "status_amber"));
    ledPropertyList.push_back(
        LedProperty(SmSignalSet::smSystemReadyLed, "status_green"));
    ledPropertyList.push_back(
        LedProperty(SmSignalSet::smIdentifyLed, "identify"));
}

void Manufacturing::revertTimerHandler()
{

#ifdef BMC_VALIDATION_UNSECURE_FEATURE
    if (mtm.getMfgMode() == SpecialMode::valUnsecure)
    {
        // Don't revert the behaviour for validation unsecure mode.
        return;
    }
#endif
    if (revertFanPWM)
    {
        revertFanPWM = false;
        disablePidControlService(false);
    }

    if (mtmTestBeepFd != -1)
    {
        ::close(mtmTestBeepFd);
        mtmTestBeepFd = -1;
    }

    for (const auto& ledProperty : ledPropertyList)
    {
        const std::string& ledName = ledProperty.getName();
        ledRevert(ledProperty.getSignal());
    }
}

Manufacturing::Manufacturing() :
    revertTimer([&](void) { revertTimerHandler(); })
{
    initData();
}

int8_t Manufacturing::getProperty(const std::string& service,
                                  const std::string& path,
                                  const std::string& interface,
                                  const std::string& propertyName,
                                  ipmi::Value* reply)
{
    try
    {
        *reply = ipmi::getDbusProperty(*getSdBus(), service, path, interface,
                                       propertyName);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "ERROR: getProperty");
        return -1;
    }

    return 0;
}

int8_t Manufacturing::setProperty(const std::string& service,
                                  const std::string& path,
                                  const std::string& interface,
                                  const std::string& propertyName,
                                  ipmi::Value value)
{
    try
    {
        ipmi::setDbusProperty(*getSdBus(), service, path, interface,
                              propertyName, value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "ERROR: setProperty");
        return -1;
    }

    return 0;
}

int8_t Manufacturing::disablePidControlService(const bool disable)
{
    try
    {
        auto dbus = getSdBus();
        auto method = dbus->new_method_call(systemDService, systemDObjPath,
                                            systemDMgrIntf,
                                            disable ? "StopUnit" : "StartUnit");
        method.append(pidControlService, "replace");
        auto reply = dbus->call(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "ERROR: phosphor-pid-control service start or stop failed");
        return -1;
    }
    return 0;
}

ipmi::RspType<uint8_t,                // Signal value
              std::optional<uint16_t> // Fan tach value
              >
    appMTMGetSignal(ipmi::Context::ptr ctx, uint8_t signalTypeByte,
                    uint8_t instance, uint8_t actionByte)
{
    // mfg filter logic is used to allow MTM get signal command only in
    // manfacturing mode.

    SmSignalGet signalType = static_cast<SmSignalGet>(signalTypeByte);
    SmActionGet action = static_cast<SmActionGet>(actionByte);

    switch (signalType)
    {
        case SmSignalGet::smChassisIntrusion:
        {
            ipmi::Value reply;
            if (mtm.getProperty(intrusionService, intrusionPath, intrusionIntf,
                                "Status", &reply) < 0)
            {
                return ipmi::responseInvalidFieldRequest();
            }
            std::string* intrusionStatus = std::get_if<std::string>(&reply);
            if (!intrusionStatus)
            {
                return ipmi::responseUnspecifiedError();
            }

            uint8_t status = 0;
            if (!intrusionStatus->compare("Normal"))
            {
                status = static_cast<uint8_t>(IntrusionStatus::normal);
            }
            else if (!intrusionStatus->compare("HardwareIntrusion"))
            {
                status =
                    static_cast<uint8_t>(IntrusionStatus::hardwareIntrusion);
            }
            else if (!intrusionStatus->compare("TamperingDetected"))
            {
                status =
                    static_cast<uint8_t>(IntrusionStatus::tamperingDetected);
            }
            else
            {
                return ipmi::responseUnspecifiedError();
            }
            return ipmi::responseSuccess(status, std::nullopt);
        }
        case SmSignalGet::smFanPwmGet:
        {
            ipmi::Value reply;
            std::string fullPath = fanPwmPath + std::to_string(instance + 1);
            if (mtm.getProperty(fanService, fullPath, fanIntf, "Value",
                                &reply) < 0)
            {
                return ipmi::responseInvalidFieldRequest();
            }
            double* doubleVal = std::get_if<double>(&reply);
            if (doubleVal == nullptr)
            {
                return ipmi::responseUnspecifiedError();
            }
            uint8_t sensorVal = std::round(*doubleVal);
            resetMtmTimer(ctx);
            return ipmi::responseSuccess(sensorVal, std::nullopt);
        }
        break;
        case SmSignalGet::smFanTachometerGet:
        {
            boost::system::error_code ec;
            using objFlatMap = boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>;

            auto flatMap = ctx->bus->yield_method_call<objFlatMap>(
                ctx->yield, ec, "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                fanTachBasePath, 0, std::array<const char*, 1>{fanIntf});
            if (ec)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Failed to query fan tach sub tree objects");
                return ipmi::responseUnspecifiedError();
            }
            if (instance >= flatMap.size())
            {
                return ipmi::responseInvalidFieldRequest();
            }
            auto itr = flatMap.nth(instance);
            ipmi::Value reply;
            if (mtm.getProperty(fanService, itr->first, fanIntf, "Value",
                                &reply) < 0)
            {
                return ipmi::responseInvalidFieldRequest();
            }

            double* doubleVal = std::get_if<double>(&reply);
            if (doubleVal == nullptr)
            {
                return ipmi::responseUnspecifiedError();
            }
            uint8_t sensorVal = FAN_PRESENT | FAN_SENSOR_PRESENT;
            std::optional<uint16_t> fanTach = std::round(*doubleVal);

            resetMtmTimer(ctx);
            return ipmi::responseSuccess(sensorVal, fanTach);
        }
        break;
        case SmSignalGet::smIdentifyButton:
        {
            if (action == SmActionGet::revert || action == SmActionGet::ignore)
            {
                // ButtonMasked property is not supported for ID button as it is
                // unnecessary. Hence if requested for revert / ignore, override
                // it to sample action to make tools happy.
                action = SmActionGet::sample;
            }
            // fall-through
        }
        case SmSignalGet::smResetButton:
        case SmSignalGet::smPowerButton:
        case SmSignalGet::smNMIButton:
        {
            std::string path;
            if (getGpioPathForSmSignal(signalType, path) < 0)
            {
                return ipmi::responseInvalidFieldRequest();
            }

            switch (action)
            {
                case SmActionGet::sample:
                    phosphor::logging::log<phosphor::logging::level::INFO>(
                        "case SmActionGet::sample");
                    break;
                case SmActionGet::ignore:
                {
                    phosphor::logging::log<phosphor::logging::level::INFO>(
                        "case SmActionGet::ignore");
                    if (mtm.setProperty(buttonService, path, buttonIntf,
                                        "ButtonMasked", true) < 0)
                    {
                        return ipmi::responseUnspecifiedError();
                    }
                }
                break;
                case SmActionGet::revert:
                {
                    phosphor::logging::log<phosphor::logging::level::INFO>(
                        "case SmActionGet::revert");
                    if (mtm.setProperty(buttonService, path, buttonIntf,
                                        "ButtonMasked", false) < 0)
                    {
                        return ipmi::responseUnspecifiedError();
                    }
                }
                break;

                default:
                    return ipmi::responseInvalidFieldRequest();
                    break;
            }

            ipmi::Value reply;
            if (mtm.getProperty(buttonService, path, buttonIntf,
                                "ButtonPressed", &reply) < 0)
            {
                return ipmi::responseUnspecifiedError();
            }
            bool* valPtr = std::get_if<bool>(&reply);
            if (valPtr == nullptr)
            {
                return ipmi::responseUnspecifiedError();
            }
            resetMtmTimer(ctx);
            uint8_t sensorVal = *valPtr;
            return ipmi::responseSuccess(sensorVal, std::nullopt);
        }
        break;
        case SmSignalGet::smNcsiDiag:
        {
            constexpr const char* netBasePath = "/sys/class/net/eth";
            constexpr const char* carrierSuffix = "/carrier";
            std::ifstream netIfs(netBasePath + std::to_string(instance) +
                                 carrierSuffix);
            if (!netIfs.good())
            {
                return ipmi::responseInvalidFieldRequest();
            }
            std::string carrier;
            netIfs >> carrier;
            resetMtmTimer(ctx);
            return ipmi::responseSuccess(
                static_cast<uint8_t>(std::stoi(carrier)), std::nullopt);
        }
        break;
        default:
            return ipmi::responseInvalidFieldRequest();
            break;
    }
}

ipmi::RspType<> appMTMSetSignal(ipmi::Context::ptr ctx, uint8_t signalTypeByte,
                                uint8_t instance, uint8_t actionByte,
                                std::optional<uint8_t> pwmSpeed)
{
    // mfg filter logic is used to allow MTM set signal command only in
    // manfacturing mode.

    SmSignalSet signalType = static_cast<SmSignalSet>(signalTypeByte);
    SmActionSet action = static_cast<SmActionSet>(actionByte);
    Cc retCode = ccSuccess;
    int8_t ret = 0;

    switch (signalType)
    {
        case SmSignalSet::smPowerFaultLed:
        case SmSignalSet::smSystemReadyLed:
        case SmSignalSet::smIdentifyLed:
            switch (action)
            {
                case SmActionSet::forceDeasserted:
                {
                    phosphor::logging::log<phosphor::logging::level::INFO>(
                        "case SmActionSet::forceDeasserted");

                    retCode = ledStoreAndSet(signalType, std::string("Off"));
                    if (retCode != ccSuccess)
                    {
                        return ipmi::response(retCode);
                    }
                    mtm.revertTimer.start(revertTimeOut);
                }
                break;
                case SmActionSet::forceAsserted:
                {
                    phosphor::logging::log<phosphor::logging::level::INFO>(
                        "case SmActionSet::forceAsserted");

                    retCode = ledStoreAndSet(signalType, std::string("On"));
                    if (retCode != ccSuccess)
                    {
                        return ipmi::response(retCode);
                    }
                    mtm.revertTimer.start(revertTimeOut);
                    if (SmSignalSet::smPowerFaultLed == signalType)
                    {
                        // Deassert "system ready"
                        retCode = ledStoreAndSet(SmSignalSet::smSystemReadyLed,
                                                 std::string("Off"));
                    }
                    else if (SmSignalSet::smSystemReadyLed == signalType)
                    {
                        // Deassert "fault led"
                        retCode = ledStoreAndSet(SmSignalSet::smPowerFaultLed,
                                                 std::string("Off"));
                    }
                }
                break;
                case SmActionSet::revert:
                {
                    phosphor::logging::log<phosphor::logging::level::INFO>(
                        "case SmActionSet::revert");
                    retCode = ledRevert(signalType);
                }
                break;
                default:
                {
                    return ipmi::responseInvalidFieldRequest();
                }
            }
            break;
        case SmSignalSet::smFanPowerSpeed:
        {
            if ((action == SmActionSet::forceAsserted) && (!pwmSpeed))
            {
                return ipmi::responseReqDataLenInvalid();
            }

            if ((action == SmActionSet::forceAsserted) && (*pwmSpeed > 100))
            {
                return ipmi::responseInvalidFieldRequest();
            }

            uint8_t pwmValue = 0;
            switch (action)
            {
                case SmActionSet::revert:
                {
                    if (mtm.revertFanPWM)
                    {
                        ret = mtm.disablePidControlService(false);
                        if (ret < 0)
                        {
                            return ipmi::responseUnspecifiedError();
                        }
                        mtm.revertFanPWM = false;
                    }
                }
                break;
                case SmActionSet::forceAsserted:
                {
                    pwmValue = *pwmSpeed;
                } // fall-through
                case SmActionSet::forceDeasserted:
                {
                    if (!mtm.revertFanPWM)
                    {
                        ret = mtm.disablePidControlService(true);
                        if (ret < 0)
                        {
                            return ipmi::responseUnspecifiedError();
                        }
                        mtm.revertFanPWM = true;
                    }
                    mtm.revertTimer.start(revertTimeOut);
                    std::string fanPwmInstancePath =
                        fanPwmPath + std::to_string(instance + 1);

                    ret =
                        mtm.setProperty(fanService, fanPwmInstancePath, fanIntf,
                                        "Value", static_cast<double>(pwmValue));
                    if (ret < 0)
                    {
                        return ipmi::responseUnspecifiedError();
                    }
                }
                break;
                default:
                {
                    return ipmi::responseInvalidFieldRequest();
                }
            }
        }
        break;
        case SmSignalSet::smSpeaker:
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "Performing Speaker SmActionSet",
                phosphor::logging::entry("ACTION=%d",
                                         static_cast<uint8_t>(action)));
            switch (action)
            {
                case SmActionSet::forceAsserted:
                {
                    char beepDevName[] = "/dev/input/event0";
                    if (mtm.mtmTestBeepFd != -1)
                    {
                        phosphor::logging::log<phosphor::logging::level::INFO>(
                            "mtm beep device is opened already!");
                        // returning success as already beep is in progress
                        return ipmi::response(retCode);
                    }

                    if ((mtm.mtmTestBeepFd =
                             ::open(beepDevName, O_RDWR | O_CLOEXEC)) < 0)
                    {
                        phosphor::logging::log<phosphor::logging::level::ERR>(
                            "Failed to open input device");
                        return ipmi::responseUnspecifiedError();
                    }

                    struct input_event event;
                    event.type = EV_SND;
                    event.code = SND_TONE;
                    event.value = 2000;

                    if (::write(mtm.mtmTestBeepFd, &event,
                                sizeof(struct input_event)) !=
                        sizeof(struct input_event))
                    {
                        phosphor::logging::log<phosphor::logging::level::ERR>(
                            "Failed to write a tone sound event");
                        ::close(mtm.mtmTestBeepFd);
                        mtm.mtmTestBeepFd = -1;
                        return ipmi::responseUnspecifiedError();
                    }
                    mtm.revertTimer.start(revertTimeOut);
                }
                break;
                case SmActionSet::revert:
                case SmActionSet::forceDeasserted:
                {
                    if (mtm.mtmTestBeepFd != -1)
                    {
                        ::close(mtm.mtmTestBeepFd);
                        mtm.mtmTestBeepFd = -1;
                    }
                }
                break;
                default:
                {
                    return ipmi::responseInvalidFieldRequest();
                }
            }
        }
        break;
        case SmSignalSet::smDiskFaultLed:
        {
            boost::system::error_code ec;
            using objPaths = std::vector<std::string>;
            std::string driveBasePath =
                "/xyz/openbmc_project/inventory/item/drive/";
            static constexpr const char* driveLedIntf =
                "xyz.openbmc_project.Led.Group";
            static constexpr const char* hsbpService =
                "xyz.openbmc_project.HsbpManager";

            auto driveList = ctx->bus->yield_method_call<objPaths>(
                ctx->yield, ec, "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                driveBasePath, 0, std::array<const char*, 1>{driveLedIntf});
            if (ec)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Failed to query HSBP drive sub tree objects");
                return ipmi::responseUnspecifiedError();
            }
            std::string driveObjPath =
                driveBasePath + "Drive_" + std::to_string(instance + 1);
            if (std::find(driveList.begin(), driveList.end(), driveObjPath) ==
                driveList.end())
            {
                return ipmi::responseInvalidFieldRequest();
            }
            bool driveLedState = false;
            switch (action)
            {
                case SmActionSet::forceAsserted:
                {
                    driveLedState = true;
                }
                break;
                case SmActionSet::revert:
                {
                    driveLedState = false;
                }
                break;
                case SmActionSet::forceDeasserted:
                {
                    driveLedState = false;
                }
                break;
                default:
                {
                    return ipmi::responseInvalidFieldRequest();
                }
            }
            ret = mtm.setProperty(hsbpService, driveObjPath, driveLedIntf,
                                  "Asserted", driveLedState);
            if (ret < 0)
            {
                return ipmi::responseUnspecifiedError();
            }
        }
        break;
        default:
        {
            return ipmi::responseInvalidFieldRequest();
        }
    }
    if (retCode == ccSuccess)
    {
        resetMtmTimer(ctx);
    }
    return ipmi::response(retCode);
}

ipmi::RspType<> mtmKeepAlive(ipmi::Context::ptr ctx, uint8_t reserved,
                             const std::array<char, 5>& intentionalSignature)
{
    // mfg filter logic is used to allow MTM keep alive command only in
    // manfacturing mode

    constexpr std::array<char, 5> signatureOk = {'I', 'N', 'T', 'E', 'L'};
    if (intentionalSignature != signatureOk || reserved != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    return ipmi::response(resetMtmTimer(ctx));
}

static constexpr unsigned int makeCmdKey(unsigned int netFn, unsigned int cmd)
{
    return (netFn << 8) | cmd;
}

ipmi::Cc mfgFilterMessage(ipmi::message::Request::ptr request)
{
    // Restricted commands, must be executed only in Manufacturing mode
    switch (makeCmdKey(request->ctx->netFn, request->ctx->cmd))
    {
        // i2c master write read command needs additional checking
        case makeCmdKey(ipmi::netFnApp, ipmi::app::cmdMasterWriteRead):
            if (request->payload.size() > 4)
            {
                // Allow write data count > 1 only in Special mode
                if (mtm.getMfgMode() == SpecialMode::none)
                {
                    return ipmi::ccInsufficientPrivilege;
                }
            }
            return ipmi::ccSuccess;
        case makeCmdKey(ipmi::netFnOemOne,
                        ipmi::intel::general::cmdGetSmSignal):
        case makeCmdKey(ipmi::netFnOemOne,
                        ipmi::intel::general::cmdSetSmSignal):
        case makeCmdKey(ipmi::netFnOemOne,
                        ipmi::intel::general::cmdMtmKeepAlive):
        case makeCmdKey(ipmi::netFnOemOne,
                        ipmi::intel::general::cmdSetManufacturingData):
        case makeCmdKey(ipmi::netFnOemOne,
                        ipmi::intel::general::cmdGetManufacturingData):
        case makeCmdKey(ipmi::netFnStorage, ipmi::storage::cmdWriteFruData):
        case makeCmdKey(ipmi::netFnOemTwo, ipmi::intel::platform::cmdClearCMOS):

            // Check for Special mode
            if (mtm.getMfgMode() == SpecialMode::none)
            {
                return ipmi::ccInvalidCommand;
            }
            return ipmi::ccSuccess;
        case makeCmdKey(ipmi::netFnStorage, ipmi::storage::cmdDeleteSelEntry):
        {
            return ipmi::ccInvalidCommand;
        }
    }
    return ipmi::ccSuccess;
}

static constexpr uint8_t maxEthSize = 6;
static constexpr uint8_t maxSupportedEth = 3;
static constexpr const char* factoryEthAddrBaseFileName =
    "/var/sofs/factory-settings/network/mac/eth";

ipmi::RspType<> setManufacturingData(ipmi::Context::ptr ctx, uint8_t dataType,
                                     std::array<uint8_t, maxEthSize> ethData)
{
    // mfg filter logic will restrict this command executing only in mfg mode.
    if (dataType >= maxSupportedEth)
    {
        return ipmi::responseParmOutOfRange();
    }

    constexpr uint8_t invalidData = 0;
    constexpr uint8_t validData = 1;
    constexpr uint8_t ethAddrStrSize =
        19; // XX:XX:XX:XX:XX:XX + \n + null termination;
    std::vector<uint8_t> buff(ethAddrStrSize);
    std::snprintf(reinterpret_cast<char*>(buff.data()), ethAddrStrSize,
                  "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", ethData.at(0),
                  ethData.at(1), ethData.at(2), ethData.at(3), ethData.at(4),
                  ethData.at(5));
    std::ofstream oEthFile(factoryEthAddrBaseFileName +
                               std::to_string(dataType),
                           std::ofstream::out);
    if (!oEthFile.good())
    {
        return ipmi::responseUnspecifiedError();
    }

    oEthFile << reinterpret_cast<char*>(buff.data());
    oEthFile.flush();
    oEthFile.close();

    resetMtmTimer(ctx);
    return ipmi::responseSuccess();
}

ipmi::RspType<uint8_t, std::array<uint8_t, maxEthSize>>
    getManufacturingData(ipmi::Context::ptr ctx, uint8_t dataType)
{
    // mfg filter logic will restrict this command executing only in mfg mode.
    if (dataType >= maxSupportedEth)
    {
        return ipmi::responseParmOutOfRange();
    }
    std::array<uint8_t, maxEthSize> ethData{0};
    constexpr uint8_t invalidData = 0;
    constexpr uint8_t validData = 1;

    std::ifstream iEthFile(factoryEthAddrBaseFileName +
                               std::to_string(dataType),
                           std::ifstream::in);
    if (!iEthFile.good())
    {
        return ipmi::responseSuccess(invalidData, ethData);
    }
    std::string ethStr;
    iEthFile >> ethStr;
    uint8_t* data = ethData.data();
    std::sscanf(ethStr.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                data, (data + 1), (data + 2), (data + 3), (data + 4),
                (data + 5));

    resetMtmTimer(ctx);
    return ipmi::responseSuccess(validData, ethData);
}

/** @brief implements slot master write read IPMI command which can be used for
 * low-level I2C/SMBus write, read or write-read access for PCIE slots
 * @param reserved - skip 6 bit
 * @param addressType - address type
 * @param bbSlotNum - baseboard slot number
 * @param riserSlotNum - riser slot number
 * @param reserved2 - skip 2 bit
 * @param slaveAddr - slave address
 * @param readCount - number of bytes to be read
 * @param writeData - data to be written
 *
 * @returns IPMI completion code plus response data
 */
ipmi::RspType<std::vector<uint8_t>>
    appSlotI2CMasterWriteRead(uint6_t reserved, uint2_t addressType,
                              uint3_t bbSlotNum, uint3_t riserSlotNum,
                              uint2_t resvered2, uint8_t slaveAddr,
                              uint8_t readCount, std::vector<uint8_t> writeData)
{
    const size_t writeCount = writeData.size();
    std::string i2cBus;
    if (addressType == slotAddressTypeBus)
    {
        std::string path = "/dev/i2c-mux/Riser_" +
                           std::to_string(static_cast<uint8_t>(bbSlotNum)) +
                           "_Mux/Pcie_Slot_" +
                           std::to_string(static_cast<uint8_t>(riserSlotNum));

        if (std::filesystem::exists(path) && std::filesystem::is_symlink(path))
        {
            i2cBus = std::filesystem::read_symlink(path);
        }
        else
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Master write read command: Cannot get BusID");
            return ipmi::responseInvalidFieldRequest();
        }
    }
    else if (addressType == slotAddressTypeUniqueid)
    {
        i2cBus = "/dev/i2c-" +
                 std::to_string(static_cast<uint8_t>(bbSlotNum) |
                                (static_cast<uint8_t>(riserSlotNum) << 3));
    }
    else
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Master write read command: invalid request");
        return ipmi::responseInvalidFieldRequest();
    }

    // Allow single byte write as it is offset byte to read the data, rest allow
    // only in Special mode.
    if (writeCount > 1)
    {
        if (mtm.getMfgMode() == SpecialMode::none)
        {
            return ipmi::responseInsufficientPrivilege();
        }
    }

    if (readCount > slotI2CMaxReadSize)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Master write read command: Read count exceeds limit");
        return ipmi::responseParmOutOfRange();
    }

    if (!readCount && !writeCount)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Master write read command: Read & write count are 0");
        return ipmi::responseInvalidFieldRequest();
    }

    std::vector<uint8_t> readBuf(readCount);

    ipmi::Cc retI2C = ipmi::i2cWriteRead(i2cBus, slaveAddr, writeData, readBuf);
    if (retI2C != ipmi::ccSuccess)
    {
        return ipmi::response(retI2C);
    }

    return ipmi::responseSuccess(readBuf);
}

ipmi::RspType<> clearCMOS()
{
    // There is an i2c device on bus 4, the slave address is 0x38. Based on the
    // spec, writing 0x1 to address 0x61 on this device, will trigger the clear
    // CMOS action.
    constexpr uint8_t slaveAddr = 0x38;
    std::string i2cBus = "/dev/i2c-4";
    std::vector<uint8_t> writeData = {0x61, 0x1};
    std::vector<uint8_t> readBuf(0);

    ipmi::Cc retI2C = ipmi::i2cWriteRead(i2cBus, slaveAddr, writeData, readBuf);
    return ipmi::response(retI2C);
}
} // namespace ipmi

void register_mtm_commands() __attribute__((constructor));
void register_mtm_commands()
{
    // <Get SM Signal>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnGeneral,
                          ipmi::intel::general::cmdGetSmSignal,
                          ipmi::Privilege::Admin, ipmi::appMTMGetSignal);

    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnGeneral,
                          ipmi::intel::general::cmdSetSmSignal,
                          ipmi::Privilege::Admin, ipmi::appMTMSetSignal);

    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnGeneral,
                          ipmi::intel::general::cmdMtmKeepAlive,
                          ipmi::Privilege::Admin, ipmi::mtmKeepAlive);

    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnGeneral,
                          ipmi::intel::general::cmdSetManufacturingData,
                          ipmi::Privilege::Admin, ipmi::setManufacturingData);

    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnGeneral,
                          ipmi::intel::general::cmdGetManufacturingData,
                          ipmi::Privilege::Admin, ipmi::getManufacturingData);

    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::general::cmdSlotI2CMasterWriteRead,
                          ipmi::Privilege::Admin,
                          ipmi::appSlotI2CMasterWriteRead);

    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnPlatform,
                          ipmi::intel::platform::cmdClearCMOS,
                          ipmi::Privilege::Admin, ipmi::clearCMOS);

    ipmi::registerFilter(ipmi::prioOemBase,
                         [](ipmi::message::Request::ptr request) {
                             return ipmi::mfgFilterMessage(request);
                         });
}
