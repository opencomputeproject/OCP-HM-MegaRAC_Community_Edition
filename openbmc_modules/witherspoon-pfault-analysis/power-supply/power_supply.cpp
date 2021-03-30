/**
 * Copyright © 2017 IBM Corporation
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
#include "power_supply.hpp"

#include "elog-errors.hpp"
#include "gpio.hpp"
#include "names_values.hpp"
#include "pmbus.hpp"
#include "utility.hpp"

#include <org/open_power/Witherspoon/Fault/error.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/Device/error.hpp>
#include <xyz/openbmc_project/Software/Version/server.hpp>

#include <functional>

namespace witherspoon
{
namespace power
{
namespace psu
{

using namespace phosphor::logging;
using namespace sdbusplus::org::open_power::Witherspoon::Fault::Error;
using namespace sdbusplus::xyz::openbmc_project::Common::Device::Error;
namespace version = sdbusplus::xyz::openbmc_project::Software::server;

constexpr auto ASSOCIATION_IFACE = "xyz.openbmc_project.Association";
constexpr auto LOGGING_IFACE = "xyz.openbmc_project.Logging.Entry";
constexpr auto INVENTORY_IFACE = "xyz.openbmc_project.Inventory.Item";
constexpr auto POWER_IFACE = "org.openbmc.control.Power";
constexpr auto INVENTORY_MGR_IFACE = "xyz.openbmc_project.Inventory.Manager";
constexpr auto ASSET_IFACE = "xyz.openbmc_project.Inventory.Decorator.Asset";
constexpr auto VERSION_IFACE = "xyz.openbmc_project.Software.Version";

constexpr auto ENDPOINTS_PROP = "endpoints";
constexpr auto MESSAGE_PROP = "Message";
constexpr auto RESOLVED_PROP = "Resolved";
constexpr auto PRESENT_PROP = "Present";
constexpr auto SN_PROP = "SerialNumber";
constexpr auto PN_PROP = "PartNumber";
constexpr auto MODEL_PROP = "Model";
constexpr auto VERSION_PROP = "Version";
constexpr auto VERSION_PURPOSE_PROP = "Purpose";

constexpr auto INVENTORY_OBJ_PATH = "/xyz/openbmc_project/inventory";
constexpr auto POWER_OBJ_PATH = "/org/openbmc/control/power0";

constexpr auto SERIAL_NUMBER = "serial_number";
constexpr auto PART_NUMBER = "part_number";
constexpr auto FW_VERSION = "fw_version";
constexpr auto CCIN = "ccin";
constexpr auto INPUT_HISTORY = "input_history";

PowerSupply::PowerSupply(const std::string& name, size_t inst,
                         const std::string& objpath, const std::string& invpath,
                         sdbusplus::bus::bus& bus, const sdeventplus::Event& e,
                         std::chrono::seconds& t, std::chrono::seconds& p) :
    Device(name, inst),
    monitorPath(objpath), pmbusIntf(objpath),
    inventoryPath(INVENTORY_OBJ_PATH + invpath), bus(bus), presentInterval(p),
    presentTimer(e, std::bind([this]() {
                     // The hwmon path may have changed.
                     pmbusIntf.findHwmonDir();
                     this->present = true;

                     // Sync the INPUT_HISTORY data for all PSs
                     syncHistory();

                     // Update the inventory for the new device
                     updateInventory();
                 })),
    powerOnInterval(t),
    powerOnTimer(e, std::bind([this]() { this->powerOn = true; }))
{
    using namespace sdbusplus::bus;
    using namespace witherspoon::pmbus;
    std::uint16_t statusWord = 0;
    try
    {
        // Read the 2 byte STATUS_WORD value to check for faults.
        statusWord = pmbusIntf.read(STATUS_WORD, Type::Debug);
        if (!((statusWord & status_word::INPUT_FAULT_WARN) ||
              (statusWord & status_word::VIN_UV_FAULT)))
        {
            resolveError(inventoryPath,
                         std::string(PowerSupplyInputFault::errName));
        }
    }
    catch (ReadFailure& e)
    {
        log<level::INFO>("Unable to read the 2 byte STATUS_WORD value to check "
                         "for power-supply input faults.");
    }
    presentMatch = std::make_unique<match_t>(
        bus, match::rules::propertiesChanged(inventoryPath, INVENTORY_IFACE),
        [this](auto& msg) { this->inventoryChanged(msg); });
    // Get initial presence state.
    updatePresence();

    // Write the SN, PN, etc to the inventory
    updateInventory();

    // Subscribe to power state changes
    powerOnMatch = std::make_unique<match_t>(
        bus, match::rules::propertiesChanged(POWER_OBJ_PATH, POWER_IFACE),
        [this](auto& msg) { this->powerStateChanged(msg); });
    // Get initial power state.
    updatePowerState();
}

void PowerSupply::captureCmd(util::NamesValues& nv, const std::string& cmd,
                             witherspoon::pmbus::Type type)
{
    if (pmbusIntf.exists(cmd, type))
    {
        try
        {
            auto val = pmbusIntf.read(cmd, type);
            nv.add(cmd, val);
        }
        catch (std::exception& e)
        {
            log<level::INFO>("Unable to capture metadata",
                             entry("CMD=%s", cmd.c_str()));
        }
    }
}

void PowerSupply::analyze()
{
    using namespace witherspoon::pmbus;

    try
    {
        if (present)
        {
            std::uint16_t statusWord = 0;

            // Read the 2 byte STATUS_WORD value to check for faults.
            statusWord = pmbusIntf.read(STATUS_WORD, Type::Debug);
            readFail = 0;

            checkInputFault(statusWord);

            if (powerOn && (inputFault == 0) && !faultFound)
            {
                checkFanFault(statusWord);
                checkTemperatureFault(statusWord);
                checkOutputOvervoltageFault(statusWord);
                checkCurrentOutOverCurrentFault(statusWord);
                checkPGOrUnitOffFault(statusWord);
            }

            updateHistory();
        }
    }
    catch (ReadFailure& e)
    {
        if (readFail < FAULT_COUNT)
        {
            readFail++;
        }

        if (!readFailLogged && readFail >= FAULT_COUNT)
        {
            commit<ReadFailure>();
            readFailLogged = true;
        }
    }

    return;
}

void PowerSupply::inventoryChanged(sdbusplus::message::message& msg)
{
    std::string msgSensor;
    std::map<std::string, std::variant<uint32_t, bool>> msgData;
    msg.read(msgSensor, msgData);

    // Check if it was the Present property that changed.
    auto valPropMap = msgData.find(PRESENT_PROP);
    if (valPropMap != msgData.end())
    {
        if (std::get<bool>(valPropMap->second))
        {
            clearFaults();
            presentTimer.restartOnce(presentInterval);
        }
        else
        {
            present = false;
            presentTimer.setEnabled(false);

            // Clear out the now outdated inventory properties
            updateInventory();
        }
    }

    return;
}

void PowerSupply::updatePresence()
{
    // Use getProperty utility function to get presence status.
    std::string service = "xyz.openbmc_project.Inventory.Manager";
    util::getProperty(INVENTORY_IFACE, PRESENT_PROP, inventoryPath, service,
                      bus, this->present);
}

void PowerSupply::powerStateChanged(sdbusplus::message::message& msg)
{
    int32_t state = 0;
    std::string msgSensor;
    std::map<std::string, std::variant<int32_t>> msgData;
    msg.read(msgSensor, msgData);

    // Check if it was the Present property that changed.
    auto valPropMap = msgData.find("state");
    if (valPropMap != msgData.end())
    {
        state = std::get<int32_t>(valPropMap->second);

        // Power is on when state=1. Set the fault logged variables to false
        // and start the power on timer when the state changes to 1.
        if (state)
        {
            clearFaults();
            powerOnTimer.restartOnce(powerOnInterval);
        }
        else
        {
            powerOnTimer.setEnabled(false);
            powerOn = false;
        }
    }
}

void PowerSupply::updatePowerState()
{
    // When state = 1, system is powered on
    int32_t state = 0;

    try
    {
        auto service = util::getService(POWER_OBJ_PATH, POWER_IFACE, bus);

        // Use getProperty utility function to get power state.
        util::getProperty<int32_t>(POWER_IFACE, "state", POWER_OBJ_PATH,
                                   service, bus, state);

        if (state)
        {
            powerOn = true;
        }
        else
        {
            powerOn = false;
        }
    }
    catch (std::exception& e)
    {
        log<level::INFO>("Failed to get power state. Assuming it is off.");
        powerOn = false;
    }
}

void PowerSupply::checkInputFault(const uint16_t statusWord)
{
    using namespace witherspoon::pmbus;

    if ((inputFault < FAULT_COUNT) &&
        ((statusWord & status_word::INPUT_FAULT_WARN) ||
         (statusWord & status_word::VIN_UV_FAULT)))
    {
        if (inputFault == 0)
        {
            log<level::INFO>("INPUT or VIN_UV fault",
                             entry("STATUS_WORD=0x%04X", statusWord));
        }

        inputFault++;
    }
    else
    {
        if ((inputFault > 0) && !(statusWord & status_word::INPUT_FAULT_WARN) &&
            !(statusWord & status_word::VIN_UV_FAULT))
        {
            inputFault = 0;
            faultFound = false;
            // When an input fault occurs, the power supply cannot be on.
            // However, the check for the case where the power supply should be
            // on will stop when there is a fault found.
            // Clear the powerOnFault when the inputFault is cleared to reset
            // the powerOnFault de-glitching.
            powerOnFault = 0;

            log<level::INFO>("INPUT_FAULT_WARN cleared",
                             entry("POWERSUPPLY=%s", inventoryPath.c_str()));

            resolveError(inventoryPath,
                         std::string(PowerSupplyInputFault::errName));

            if (powerOn)
            {
                // The power supply will not be immediately powered on after
                // the input power is restored.
                powerOn = false;
                // Start up the timer that will set the state to indicate we
                // are ready for the powered on fault checks.
                powerOnTimer.restartOnce(powerOnInterval);
            }
        }
    }

    if (!faultFound && (inputFault >= FAULT_COUNT))
    {
        // If the power is on, report the fault in an error log entry.
        if (powerOn)
        {
            util::NamesValues nv;
            nv.add("STATUS_WORD", statusWord);
            captureCmd(nv, STATUS_INPUT, Type::Debug);

            using metadata =
                org::open_power::Witherspoon::Fault::PowerSupplyInputFault;

            report<PowerSupplyInputFault>(
                metadata::RAW_STATUS(nv.get().c_str()),
                metadata::CALLOUT_INVENTORY_PATH(inventoryPath.c_str()));

            faultFound = true;
        }
    }
}

void PowerSupply::checkPGOrUnitOffFault(const uint16_t statusWord)
{
    using namespace witherspoon::pmbus;

    if (powerOnFault < FAULT_COUNT)
    {
        // Check PG# and UNIT_IS_OFF
        if ((statusWord & status_word::POWER_GOOD_NEGATED) ||
            (statusWord & status_word::UNIT_IS_OFF))
        {
            log<level::INFO>("PGOOD or UNIT_IS_OFF bit bad",
                             entry("STATUS_WORD=0x%04X", statusWord));
            powerOnFault++;
        }
        else
        {
            if (powerOnFault > 0)
            {
                log<level::INFO>("PGOOD and UNIT_IS_OFF bits good");
                powerOnFault = 0;
            }
        }

        if (!faultFound && (powerOnFault >= FAULT_COUNT))
        {
            faultFound = true;

            util::NamesValues nv;
            nv.add("STATUS_WORD", statusWord);
            captureCmd(nv, STATUS_INPUT, Type::Debug);
            auto status0Vout = pmbusIntf.insertPageNum(STATUS_VOUT, 0);
            captureCmd(nv, status0Vout, Type::Debug);
            captureCmd(nv, STATUS_IOUT, Type::Debug);
            captureCmd(nv, STATUS_MFR, Type::Debug);

            using metadata =
                org::open_power::Witherspoon::Fault::PowerSupplyShouldBeOn;

            // A power supply is OFF (or pgood low) but should be on.
            report<PowerSupplyShouldBeOn>(
                metadata::RAW_STATUS(nv.get().c_str()),
                metadata::CALLOUT_INVENTORY_PATH(inventoryPath.c_str()));
        }
    }
}

void PowerSupply::checkCurrentOutOverCurrentFault(const uint16_t statusWord)
{
    using namespace witherspoon::pmbus;

    if (outputOCFault < FAULT_COUNT)
    {
        // Check for an output overcurrent fault.
        if ((statusWord & status_word::IOUT_OC_FAULT))
        {
            outputOCFault++;
        }
        else
        {
            if (outputOCFault > 0)
            {
                outputOCFault = 0;
            }
        }

        if (!faultFound && (outputOCFault >= FAULT_COUNT))
        {
            util::NamesValues nv;
            nv.add("STATUS_WORD", statusWord);
            captureCmd(nv, STATUS_INPUT, Type::Debug);
            auto status0Vout = pmbusIntf.insertPageNum(STATUS_VOUT, 0);
            captureCmd(nv, status0Vout, Type::Debug);
            captureCmd(nv, STATUS_IOUT, Type::Debug);
            captureCmd(nv, STATUS_MFR, Type::Debug);

            using metadata = org::open_power::Witherspoon::Fault::
                PowerSupplyOutputOvercurrent;

            report<PowerSupplyOutputOvercurrent>(
                metadata::RAW_STATUS(nv.get().c_str()),
                metadata::CALLOUT_INVENTORY_PATH(inventoryPath.c_str()));

            faultFound = true;
        }
    }
}

void PowerSupply::checkOutputOvervoltageFault(const uint16_t statusWord)
{
    using namespace witherspoon::pmbus;

    if (outputOVFault < FAULT_COUNT)
    {
        // Check for an output overvoltage fault.
        if (statusWord & status_word::VOUT_OV_FAULT)
        {
            outputOVFault++;
        }
        else
        {
            if (outputOVFault > 0)
            {
                outputOVFault = 0;
            }
        }

        if (!faultFound && (outputOVFault >= FAULT_COUNT))
        {
            util::NamesValues nv;
            nv.add("STATUS_WORD", statusWord);
            captureCmd(nv, STATUS_INPUT, Type::Debug);
            auto status0Vout = pmbusIntf.insertPageNum(STATUS_VOUT, 0);
            captureCmd(nv, status0Vout, Type::Debug);
            captureCmd(nv, STATUS_IOUT, Type::Debug);
            captureCmd(nv, STATUS_MFR, Type::Debug);

            using metadata = org::open_power::Witherspoon::Fault::
                PowerSupplyOutputOvervoltage;

            report<PowerSupplyOutputOvervoltage>(
                metadata::RAW_STATUS(nv.get().c_str()),
                metadata::CALLOUT_INVENTORY_PATH(inventoryPath.c_str()));

            faultFound = true;
        }
    }
}

void PowerSupply::checkFanFault(const uint16_t statusWord)
{
    using namespace witherspoon::pmbus;

    if (fanFault < FAULT_COUNT)
    {
        // Check for a fan fault or warning condition
        if (statusWord & status_word::FAN_FAULT)
        {
            fanFault++;
        }
        else
        {
            if (fanFault > 0)
            {
                fanFault = 0;
            }
        }

        if (!faultFound && (fanFault >= FAULT_COUNT))
        {
            util::NamesValues nv;
            nv.add("STATUS_WORD", statusWord);
            captureCmd(nv, STATUS_MFR, Type::Debug);
            captureCmd(nv, STATUS_TEMPERATURE, Type::Debug);
            captureCmd(nv, STATUS_FANS_1_2, Type::Debug);

            using metadata =
                org::open_power::Witherspoon::Fault::PowerSupplyFanFault;

            report<PowerSupplyFanFault>(
                metadata::RAW_STATUS(nv.get().c_str()),
                metadata::CALLOUT_INVENTORY_PATH(inventoryPath.c_str()));

            faultFound = true;
        }
    }
}

void PowerSupply::checkTemperatureFault(const uint16_t statusWord)
{
    using namespace witherspoon::pmbus;

    // Due to how the PMBus core device driver sends a clear faults command
    // the bit in STATUS_WORD will likely be cleared when we attempt to examine
    // it for a Thermal Fault or Warning. So, check the STATUS_WORD and the
    // STATUS_TEMPERATURE bits. If either indicates a fault, proceed with
    // logging the over-temperature condition.
    std::uint8_t statusTemperature = 0;
    statusTemperature = pmbusIntf.read(STATUS_TEMPERATURE, Type::Debug);
    if (temperatureFault < FAULT_COUNT)
    {
        if ((statusWord & status_word::TEMPERATURE_FAULT_WARN) ||
            (statusTemperature & status_temperature::OT_FAULT))
        {
            temperatureFault++;
        }
        else
        {
            if (temperatureFault > 0)
            {
                temperatureFault = 0;
            }
        }

        if (!faultFound && (temperatureFault >= FAULT_COUNT))
        {
            // The power supply has had an over-temperature condition.
            // This may not result in a shutdown if experienced for a short
            // duration.
            // This should not occur under normal conditions.
            // The power supply may be faulty, or the paired supply may be
            // putting out less current.
            // Capture command responses with potentially relevant information,
            // and call out the power supply reporting the condition.
            util::NamesValues nv;
            nv.add("STATUS_WORD", statusWord);
            captureCmd(nv, STATUS_MFR, Type::Debug);
            captureCmd(nv, STATUS_IOUT, Type::Debug);
            nv.add("STATUS_TEMPERATURE", statusTemperature);
            captureCmd(nv, STATUS_FANS_1_2, Type::Debug);

            using metadata = org::open_power::Witherspoon::Fault::
                PowerSupplyTemperatureFault;

            report<PowerSupplyTemperatureFault>(
                metadata::RAW_STATUS(nv.get().c_str()),
                metadata::CALLOUT_INVENTORY_PATH(inventoryPath.c_str()));

            faultFound = true;
        }
    }
}

void PowerSupply::clearFaults()
{
    readFail = 0;
    readFailLogged = false;
    inputFault = 0;
    powerOnFault = 0;
    outputOCFault = 0;
    outputOVFault = 0;
    fanFault = 0;
    temperatureFault = 0;
    faultFound = false;

    return;
}

void PowerSupply::resolveError(const std::string& callout,
                               const std::string& message)
{
    using EndpointList = std::vector<std::string>;

    try
    {
        auto path = callout + "/fault";
        // Get the service name from the mapper for the fault callout
        auto service = util::getService(path, ASSOCIATION_IFACE, bus);

        // Use getProperty utility function to get log entries (endpoints)
        EndpointList logEntries;
        util::getProperty(ASSOCIATION_IFACE, ENDPOINTS_PROP, path, service, bus,
                          logEntries);

        // It is possible that all such entries for this callout have since
        // been deleted.
        if (logEntries.empty())
        {
            return;
        }

        auto logEntryService =
            util::getService(logEntries[0], LOGGING_IFACE, bus);
        if (logEntryService.empty())
        {
            return;
        }

        // go through each log entry that matches this callout path
        std::string logMessage;
        for (const auto& logEntry : logEntries)
        {
            // Check to see if this logEntry has a message that matches.
            util::getProperty(LOGGING_IFACE, MESSAGE_PROP, logEntry,
                              logEntryService, bus, logMessage);

            if (message == logMessage)
            {
                // Log entry matches call out and message, set Resolved to true
                bool resolved = true;
                util::setProperty(LOGGING_IFACE, RESOLVED_PROP, logEntry,
                                  logEntryService, bus, resolved);
            }
        }
    }
    catch (std::exception& e)
    {
        log<level::INFO>("Failed to resolve error",
                         entry("CALLOUT=%s", callout.c_str()),
                         entry("ERROR=%s", message.c_str()));
    }
}

void PowerSupply::updateInventory()
{
    using namespace witherspoon::pmbus;
    using namespace sdbusplus::message;

    // If any of these accesses fail, the fields will just be
    // blank in the inventory.  Leave logging ReadFailure errors
    // to analyze() as it runs continuously and will most
    // likely hit and threshold them first anyway.  The
    // readString() function will do the tracing of the failing
    // path so this code doesn't need to.
    std::string pn;
    std::string sn;
    std::string ccin;
    std::string version;

    if (present)
    {
        try
        {
            sn = pmbusIntf.readString(SERIAL_NUMBER, Type::HwmonDeviceDebug);
        }
        catch (ReadFailure& e)
        {}

        try
        {
            pn = pmbusIntf.readString(PART_NUMBER, Type::HwmonDeviceDebug);
        }
        catch (ReadFailure& e)
        {}

        try
        {
            ccin = pmbusIntf.readString(CCIN, Type::HwmonDeviceDebug);
        }
        catch (ReadFailure& e)
        {}

        try
        {
            version = pmbusIntf.readString(FW_VERSION, Type::HwmonDeviceDebug);
        }
        catch (ReadFailure& e)
        {}
    }

    // Build the object map and send it to the inventory
    using Properties = std::map<std::string, std::variant<std::string>>;
    using Interfaces = std::map<std::string, Properties>;
    using Object = std::map<object_path, Interfaces>;
    Properties assetProps;
    Properties versionProps;
    Interfaces interfaces;
    Object object;

    assetProps.emplace(SN_PROP, sn);
    assetProps.emplace(PN_PROP, pn);
    assetProps.emplace(MODEL_PROP, ccin);
    interfaces.emplace(ASSET_IFACE, std::move(assetProps));

    versionProps.emplace(VERSION_PROP, version);
    interfaces.emplace(VERSION_IFACE, std::move(versionProps));

    // For Notify(), just send the relative path of the inventory
    // object so remove the INVENTORY_OBJ_PATH prefix
    auto path = inventoryPath.substr(strlen(INVENTORY_OBJ_PATH));

    object.emplace(path, std::move(interfaces));

    try
    {
        auto service =
            util::getService(INVENTORY_OBJ_PATH, INVENTORY_MGR_IFACE, bus);

        if (service.empty())
        {
            log<level::ERR>("Unable to get inventory manager service");
            return;
        }

        auto method = bus.new_method_call(service.c_str(), INVENTORY_OBJ_PATH,
                                          INVENTORY_MGR_IFACE, "Notify");

        method.append(std::move(object));

        auto reply = bus.call(method);

        // TODO: openbmc/openbmc#2756
        // Calling Notify() with an enumerated property crashes inventory
        // manager, so let it default to Unknown and now set it to the
        // right value.
        auto purpose =
            version::convertForMessage(version::Version::VersionPurpose::Other);

        util::setProperty(VERSION_IFACE, VERSION_PURPOSE_PROP, inventoryPath,
                          service, bus, purpose);
    }
    catch (std::exception& e)
    {
        log<level::ERR>(e.what(), entry("PATH=%s", inventoryPath.c_str()));
    }
}

void PowerSupply::syncHistory()
{
    using namespace witherspoon::gpio;

    if (syncGPIODevPath.empty())
    {
        // Sync not implemented
        return;
    }

    GPIO gpio{syncGPIODevPath, static_cast<gpioNum_t>(syncGPIONumber),
              Direction::output};

    try
    {
        gpio.set(Value::low);

        std::this_thread::sleep_for(std::chrono::milliseconds{5});

        gpio.set(Value::high);

        recordManager->clear();
    }
    catch (std::exception& e)
    {
        // Do nothing.  There would already be a journal entry.
    }
}

void PowerSupply::enableHistory(const std::string& objectPath,
                                size_t numRecords,
                                const std::string& syncGPIOPath,
                                size_t syncGPIONum)
{
    historyObjectPath = objectPath;
    syncGPIODevPath = syncGPIOPath;
    syncGPIONumber = syncGPIONum;

    recordManager = std::make_unique<history::RecordManager>(numRecords);

    auto avgPath = historyObjectPath + '/' + history::Average::name;
    auto maxPath = historyObjectPath + '/' + history::Maximum::name;

    average = std::make_unique<history::Average>(bus, avgPath);

    maximum = std::make_unique<history::Maximum>(bus, maxPath);
}

void PowerSupply::updateHistory()
{
    if (!recordManager)
    {
        // Not enabled
        return;
    }

    // Read just the most recent average/max record
    auto data =
        pmbusIntf.readBinary(INPUT_HISTORY, pmbus::Type::HwmonDeviceDebug,
                             history::RecordManager::RAW_RECORD_SIZE);

    // Update D-Bus only if something changed (a new record ID, or cleared out)
    auto changed = recordManager->add(data);
    if (changed)
    {
        average->values(std::move(recordManager->getAverageRecords()));
        maximum->values(std::move(recordManager->getMaximumRecords()));
    }
}

} // namespace psu
} // namespace power
} // namespace witherspoon
