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
#include "ucd90160.hpp"

#include "names_values.hpp"
#include "utility.hpp"

#include <elog-errors.hpp>
#include <org/open_power/Witherspoon/Fault/error.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/Device/error.hpp>

#include <map>
#include <memory>

namespace witherspoon
{
namespace power
{

using namespace std::string_literals;

const auto MFR_STATUS = "mfr_status"s;

const auto DEVICE_NAME = "UCD90160"s;
const auto DRIVER_NAME = "ucd9000"s;
constexpr auto NUM_PAGES = 16;

constexpr auto INVENTORY_OBJ_PATH = "/xyz/openbmc_project/inventory";

namespace fs = std::filesystem;
using namespace gpio;
using namespace pmbus;
using namespace phosphor::logging;

namespace device_error = sdbusplus::xyz::openbmc_project::Common::Device::Error;
namespace power_error = sdbusplus::org::open_power::Witherspoon::Fault::Error;

UCD90160::UCD90160(size_t instance, sdbusplus::bus::bus& bus) :
    Device(DEVICE_NAME, instance),
    interface(std::get<ucd90160::pathField>(deviceMap.find(instance)->second),
              DRIVER_NAME, instance),
    gpioDevice(findGPIODevice(interface.path())), bus(bus)
{}

void UCD90160::onFailure()
{
    try
    {
        auto voutError = checkVOUTFaults();

        auto pgoodError = checkPGOODFaults(false);

        // Not a voltage or PGOOD fault, but we know something
        // failed so still create an error log.
        if (!voutError && !pgoodError)
        {
            createPowerFaultLog();
        }
    }
    catch (device_error::ReadFailure& e)
    {
        if (!accessError)
        {
            commit<device_error::ReadFailure>();
            accessError = true;
        }
    }
}

void UCD90160::analyze()
{
    try
    {
        // Note: Voltage faults are always fatal, so they just
        // need to be analyzed in onFailure().

        checkPGOODFaults(true);
    }
    catch (device_error::ReadFailure& e)
    {
        if (!accessError)
        {
            commit<device_error::ReadFailure>();
            accessError = true;
        }
    }
}

uint16_t UCD90160::readStatusWord()
{
    return interface.read(STATUS_WORD, Type::Debug);
}

uint32_t UCD90160::readMFRStatus()
{
    return interface.read(MFR_STATUS, Type::HwmonDeviceDebug);
}

bool UCD90160::checkVOUTFaults()
{
    bool errorCreated = false;
    auto statusWord = readStatusWord();

    // The status_word register has a summary bit to tell us
    // if each page even needs to be checked
    if (!(statusWord & status_word::VOUT_FAULT))
    {
        return errorCreated;
    }

    for (size_t page = 0; page < NUM_PAGES; page++)
    {
        if (isVoutFaultLogged(page))
        {
            continue;
        }

        auto statusVout = interface.insertPageNum(STATUS_VOUT, page);
        uint8_t vout = interface.read(statusVout, Type::Debug);

        // If any bits are on log them, though some are just
        // warnings so they won't cause errors
        if (vout)
        {
            log<level::INFO>("A voltage rail has bits on in STATUS_VOUT",
                             entry("STATUS_VOUT=0x%X", vout),
                             entry("PAGE=%d", page));
        }

        // Log errors if any non-warning bits on
        if (vout & ~status_vout::WARNING_MASK)
        {
            auto& railNames = std::get<ucd90160::railNamesField>(
                deviceMap.find(getInstance())->second);
            auto railName = railNames.at(page);

            util::NamesValues nv;
            try
            {
                nv.add("STATUS_WORD", statusWord);
                nv.add("STATUS_VOUT", vout);
                nv.add("MFR_STATUS", readMFRStatus());
            }
            catch (device_error::ReadFailure& e)
            {
                log<level::ERR>("ReadFailure when collecting metadata");
                commit<device_error::ReadFailure>();
            }

            using metadata =
                org::open_power::Witherspoon::Fault::PowerSequencerVoltageFault;

            report<power_error::PowerSequencerVoltageFault>(
                metadata::RAIL(page), metadata::RAIL_NAME(railName.c_str()),
                metadata::RAW_STATUS(nv.get().c_str()));

            setVoutFaultLogged(page);
            errorCreated = true;
        }
    }

    return errorCreated;
}

bool UCD90160::checkPGOODFaults(bool polling)
{
    bool errorCreated = false;

    // While PGOOD faults could show up in MFR_STATUS (and we could then
    // check the summary bit in STATUS_WORD first), they are edge triggered,
    // and as the device driver sends a clear faults command every time we
    // do a read, we will never see them.  So, we'll have to just read the
    // real time GPI status GPIO.

    // Check only the GPIs configured on this system.
    auto& gpiConfigs = std::get<ucd90160::gpiConfigField>(
        deviceMap.find(getInstance())->second);

    for (const auto& gpiConfig : gpiConfigs)
    {
        auto gpiNum = std::get<ucd90160::gpiNumField>(gpiConfig);
        auto doPoll = std::get<ucd90160::pollField>(gpiConfig);

        // Can skip this one if there is already an error on this input,
        // or we are polling and these inputs don't need to be polled
        //(because errors on them are fatal).
        if (isPGOODFaultLogged(gpiNum) || (polling && !doPoll))
        {
            continue;
        }

        // The real time status is read via the pin ID
        auto pinID = std::get<ucd90160::pinIDField>(gpiConfig);
        auto gpio = gpios.find(pinID);
        Value gpiStatus;

        try
        {
            // The first time through, create the GPIO objects
            if (gpio == gpios.end())
            {
                gpios.emplace(pinID, std::make_unique<GPIO>(gpioDevice, pinID,
                                                            Direction::input));
                gpio = gpios.find(pinID);
            }

            gpiStatus = gpio->second->read();
        }
        catch (std::exception& e)
        {
            if (!accessError)
            {
                log<level::ERR>(e.what());
                accessError = true;
            }
            continue;
        }

        if (gpiStatus == Value::low)
        {
            // There may be some extra analysis we can do to narrow the
            // error down further.  Note that finding an error here won't
            // prevent us from checking this GPI again.
            errorCreated = doExtraAnalysis(gpiConfig);

            if (errorCreated)
            {
                continue;
            }

            auto& gpiName = std::get<ucd90160::gpiNameField>(gpiConfig);
            auto status = (gpiStatus == Value::low) ? 0 : 1;

            util::NamesValues nv;

            try
            {
                nv.add("STATUS_WORD", readStatusWord());
                nv.add("MFR_STATUS", readMFRStatus());
                nv.add("INPUT_STATUS", status);
            }
            catch (device_error::ReadFailure& e)
            {
                log<level::ERR>("ReadFailure when collecting metadata");
                commit<device_error::ReadFailure>();
            }

            using metadata =
                org::open_power::Witherspoon::Fault::PowerSequencerPGOODFault;

            report<power_error::PowerSequencerPGOODFault>(
                metadata::INPUT_NUM(gpiNum),
                metadata::INPUT_NAME(gpiName.c_str()),
                metadata::RAW_STATUS(nv.get().c_str()));

            setPGOODFaultLogged(gpiNum);
            errorCreated = true;
        }
    }

    return errorCreated;
}

void UCD90160::createPowerFaultLog()
{
    util::NamesValues nv;

    try
    {
        nv.add("STATUS_WORD", readStatusWord());
        nv.add("MFR_STATUS", readMFRStatus());
    }
    catch (device_error::ReadFailure& e)
    {
        log<level::ERR>("ReadFailure when collecting metadata");
        commit<device_error::ReadFailure>();
    }

    using metadata = org::open_power::Witherspoon::Fault::PowerSequencerFault;

    report<power_error::PowerSequencerFault>(
        metadata::RAW_STATUS(nv.get().c_str()));
}

fs::path UCD90160::findGPIODevice(const fs::path& path)
{
    fs::path gpioDevicePath;

    // In the driver directory, look for a subdirectory
    // named gpiochipX, where X is some number.  Then
    // we'll access the GPIO at /dev/gpiochipX.
    if (fs::is_directory(path))
    {
        for (auto& f : fs::directory_iterator(path))
        {
            if (f.path().filename().string().find("gpiochip") !=
                std::string::npos)
            {
                gpioDevicePath = "/dev" / f.path().filename();
                break;
            }
        }
    }

    if (gpioDevicePath.empty())
    {
        log<level::ERR>("Could not find GPIO device path",
                        entry("BASE_PATH=%s", path.c_str()));
    }

    return gpioDevicePath;
}

bool UCD90160::doExtraAnalysis(const ucd90160::GPIConfig& config)
{

    auto type = std::get<ucd90160::extraAnalysisField>(config);
    if (type == ucd90160::extraAnalysisType::none)
    {
        return false;
    }

    // Currently the only extra analysis to do is to check other GPIOs.
    return doGPIOAnalysis(type);
}

bool UCD90160::doGPIOAnalysis(ucd90160::extraAnalysisType type)
{
    bool errorFound = false;
    bool shutdown = false;

    const auto& analysisConfig = std::get<ucd90160::gpioAnalysisField>(
        deviceMap.find(getInstance())->second);

    auto gpioConfig = analysisConfig.find(type);
    if (gpioConfig == analysisConfig.end())
    {
        return errorFound;
    }

    auto path = std::get<ucd90160::gpioDevicePathField>(gpioConfig->second);

    // The /dev/gpiochipX device
    auto device = findGPIODevice(path);

    if (device.empty())
    {
        log<level::ERR>(
            "Missing GPIO device - cannot do GPIO analysis of fault",
            entry("ANALYSIS_TYPE=%d\n", type));
        return errorFound;
    }

    // The GPIO value of the fault condition
    auto polarity = std::get<ucd90160::gpioPolarityField>(gpioConfig->second);

    // The GPIOs to check
    auto& gpios = std::get<ucd90160::gpioDefinitionField>(gpioConfig->second);

    for (const auto& gpio : gpios)
    {
        gpio::Value value;

        try
        {
            GPIO g{device, std::get<ucd90160::gpioNumField>(gpio),
                   Direction::input};

            value = g.read();
        }
        catch (std::exception& e)
        {
            if (!gpioAccessError)
            {
                // GPIO only throws InternalErrors - not worth committing.
                log<level::ERR>(
                    "GPIO read failed while analyzing a power fault",
                    entry("CHIP_PATH=%s", path.c_str()));

                gpioAccessError = true;
            }
            continue;
        }

        if (value == polarity)
        {
            errorFound = true;

            std::string part{INVENTORY_OBJ_PATH};
            part = part + std::get<ucd90160::gpioCalloutField>(gpio);
            PartCallout callout{type, part};

            if (isPartCalledOut(callout))
            {
                continue;
            }

            // Look up and call the error creation function
            auto logError =
                std::get<ucd90160::errorFunctionField>(gpioConfig->second);

            logError(*this, part);

            // Save the part callout so we don't call it out again
            setPartCallout(callout);

            // Some errors (like overtemps) require a shutdown
            auto actions = static_cast<uint32_t>(
                std::get<ucd90160::optionFlagsField>(gpioConfig->second));

            if (actions & static_cast<decltype(actions)>(
                              ucd90160::optionFlags::shutdownOnFault))
            {
                shutdown = true;
            }
        }
    }

    if (shutdown)
    {
        // Will be replaced with a GPU specific error in a future commit
        util::powerOff<power_error::Shutdown>(bus);
    }

    return errorFound;
}

void UCD90160::gpuPGOODError(const std::string& callout)
{
    util::NamesValues nv;

    try
    {
        nv.add("STATUS_WORD", readStatusWord());
        nv.add("MFR_STATUS", readMFRStatus());
    }
    catch (device_error::ReadFailure& e)
    {
        log<level::ERR>("ReadFailure when collecting metadata");
        commit<device_error::ReadFailure>();
    }

    using metadata = org::open_power::Witherspoon::Fault::GPUPowerFault;

    report<power_error::GPUPowerFault>(
        metadata::RAW_STATUS(nv.get().c_str()),
        metadata::CALLOUT_INVENTORY_PATH(callout.c_str()));
}

void UCD90160::gpuOverTempError(const std::string& callout)
{
    util::NamesValues nv;

    try
    {
        nv.add("STATUS_WORD", readStatusWord());
        nv.add("MFR_STATUS", readMFRStatus());
    }
    catch (device_error::ReadFailure& e)
    {
        log<level::ERR>("ReadFailure when collecting metadata");
        commit<device_error::ReadFailure>();
    }

    using metadata = org::open_power::Witherspoon::Fault::GPUOverTemp;

    report<power_error::GPUOverTemp>(
        metadata::RAW_STATUS(nv.get().c_str()),
        metadata::CALLOUT_INVENTORY_PATH(callout.c_str()));
}

void UCD90160::memGoodError(const std::string& callout)
{
    util::NamesValues nv;

    try
    {
        nv.add("STATUS_WORD", readStatusWord());
        nv.add("MFR_STATUS", readMFRStatus());
    }
    catch (device_error::ReadFailure& e)
    {
        log<level::ERR>("ReadFailure when collecting metadata");
        commit<device_error::ReadFailure>();
    }

    using metadata = org::open_power::Witherspoon::Fault::MemoryPowerFault;

    report<power_error::MemoryPowerFault>(
        metadata::RAW_STATUS(nv.get().c_str()),
        metadata::CALLOUT_INVENTORY_PATH(callout.c_str()));
}

} // namespace power
} // namespace witherspoon
