/**
 * Copyright © 2019 IBM Corporation
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
#include "src.hpp"

#include "device_callouts.hpp"
#include "json_utils.hpp"
#include "paths.hpp"
#include "pel_values.hpp"

#include <phosphor-logging/log.hpp>

namespace openpower
{
namespace pels
{
namespace pv = openpower::pels::pel_values;
namespace rg = openpower::pels::message;
using namespace phosphor::logging;
using namespace std::string_literals;

constexpr size_t ccinSize = 4;

void SRC::unflatten(Stream& stream)
{
    stream >> _header >> _version >> _flags >> _reserved1B >> _wordCount >>
        _reserved2B >> _size;

    for (auto& word : _hexData)
    {
        stream >> word;
    }

    _asciiString = std::make_unique<src::AsciiString>(stream);

    if (hasAdditionalSections())
    {
        // The callouts section is currently the only extra subsection type
        _callouts = std::make_unique<src::Callouts>(stream);
    }
}

void SRC::flatten(Stream& stream) const
{
    stream << _header << _version << _flags << _reserved1B << _wordCount
           << _reserved2B << _size;

    for (auto& word : _hexData)
    {
        stream << word;
    }

    _asciiString->flatten(stream);

    if (_callouts)
    {
        _callouts->flatten(stream);
    }
}

SRC::SRC(Stream& pel)
{
    try
    {
        unflatten(pel);
        validate();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Cannot unflatten SRC", entry("ERROR=%s", e.what()));
        _valid = false;
    }
}

SRC::SRC(const message::Entry& regEntry, const AdditionalData& additionalData,
         const DataInterfaceBase& dataIface)
{
    _header.id = static_cast<uint16_t>(SectionID::primarySRC);
    _header.version = srcSectionVersion;
    _header.subType = srcSectionSubtype;
    _header.componentID = regEntry.componentID;

    _version = srcVersion;

    _flags = 0;
    if (regEntry.src.powerFault.value_or(false))
    {
        _flags |= powerFaultEvent;
    }

    _reserved1B = 0;

    _wordCount = numSRCHexDataWords + 1;

    _reserved2B = 0;

    // There are multiple fields encoded in the hex data words.
    std::for_each(_hexData.begin(), _hexData.end(),
                  [](auto& word) { word = 0; });
    setBMCFormat();
    setBMCPosition();
    setMotherboardCCIN(dataIface);

    // Partition dump status and partition boot type always 0 for BMC errors.
    //
    // TODO: Fill in other fields that aren't available yet.

    // Fill in the last 4 words from the AdditionalData property contents.
    setUserDefinedHexWords(regEntry, additionalData);

    _asciiString = std::make_unique<src::AsciiString>(regEntry);

    addCallouts(regEntry, additionalData, dataIface);

    _size = baseSRCSize;
    _size += _callouts ? _callouts->flattenedSize() : 0;
    _header.size = Section::flattenedSize() + _size;

    _valid = true;
}

void SRC::setUserDefinedHexWords(const message::Entry& regEntry,
                                 const AdditionalData& ad)
{
    if (!regEntry.src.hexwordADFields)
    {
        return;
    }

    // Save the AdditionalData value corresponding to the
    // adName key in _hexData[wordNum].
    for (const auto& [wordNum, adName] : *regEntry.src.hexwordADFields)
    {
        // Can only set words 6 - 9
        if (!isUserDefinedWord(wordNum))
        {
            std::string msg =
                "SRC user data word out of range: " + std::to_string(wordNum);
            addDebugData(msg);
            continue;
        }

        auto value = ad.getValue(adName);
        if (value)
        {
            _hexData[getWordIndexFromWordNum(wordNum)] =
                std::strtoul(value.value().c_str(), nullptr, 0);
        }
        else
        {
            std::string msg =
                "Source for user data SRC word not found: " + adName;
            addDebugData(msg);
        }
    }
}

void SRC::setMotherboardCCIN(const DataInterfaceBase& dataIface)
{
    uint32_t ccin = 0;
    auto ccinString = dataIface.getMotherboardCCIN();

    try
    {
        if (ccinString.size() == ccinSize)
        {
            ccin = std::stoi(ccinString, 0, 16);
        }
    }
    catch (std::exception& e)
    {
        log<level::WARNING>("Could not convert motherboard CCIN to a number",
                            entry("CCIN=%s", ccinString.c_str()));
        return;
    }

    // Set the first 2 bytes
    _hexData[1] |= ccin << 16;
}

void SRC::validate()
{
    bool failed = false;

    if ((header().id != static_cast<uint16_t>(SectionID::primarySRC)) &&
        (header().id != static_cast<uint16_t>(SectionID::secondarySRC)))
    {
        log<level::ERR>("Invalid SRC section ID",
                        entry("ID=0x%X", header().id));
        failed = true;
    }

    // Check the version in the SRC, not in the header
    if (_version != srcVersion)
    {
        log<level::ERR>("Invalid SRC version", entry("VERSION=0x%X", _version));
        failed = true;
    }

    _valid = failed ? false : true;
}

bool SRC::isBMCSRC() const
{
    auto as = asciiString();
    if (as.length() >= 2)
    {
        uint8_t errorType = strtoul(as.substr(0, 2).c_str(), nullptr, 16);
        return (errorType == static_cast<uint8_t>(SRCType::bmcError) ||
                errorType == static_cast<uint8_t>(SRCType::powerError));
    }
    return false;
}

std::optional<std::string> SRC::getErrorDetails(message::Registry& registry,
                                                DetailLevel type,
                                                bool toCache) const
{
    const std::string jsonIndent(indentLevel, 0x20);
    std::string errorOut;
    if (isBMCSRC())
    {
        auto entry = registry.lookup("0x" + asciiString().substr(4, 4),
                                     rg::LookupType::reasonCode, toCache);
        if (entry)
        {
            errorOut.append(jsonIndent + "\"Error Details\": {\n");
            auto errorMsg = getErrorMessage(*entry);
            if (errorMsg)
            {
                if (type == DetailLevel::message)
                {
                    return errorMsg.value();
                }
                else
                {
                    jsonInsert(errorOut, "Message", errorMsg.value(), 2);
                }
            }
            if (entry->src.hexwordADFields)
            {
                std::map<size_t, std::string> adFields =
                    entry->src.hexwordADFields.value();
                for (const auto& hexwordMap : adFields)
                {
                    jsonInsert(errorOut, hexwordMap.second,
                               getNumberString("0x%X",
                                               _hexData[getWordIndexFromWordNum(
                                                   hexwordMap.first)]),
                               2);
                }
            }
            errorOut.erase(errorOut.size() - 2);
            errorOut.append("\n");
            errorOut.append(jsonIndent + "},\n");
            return errorOut;
        }
    }
    return std::nullopt;
}

std::optional<std::string>
    SRC::getErrorMessage(const message::Entry& regEntry) const
{
    try
    {
        if (regEntry.doc.messageArgSources)
        {
            size_t msgLen = regEntry.doc.message.length();
            char msg[msgLen + 1];
            strcpy(msg, regEntry.doc.message.c_str());
            std::vector<uint32_t> argSourceVals;
            std::string message;
            const auto& argValues = regEntry.doc.messageArgSources.value();
            for (size_t i = 0; i < argValues.size(); ++i)
            {
                argSourceVals.push_back(_hexData[getWordIndexFromWordNum(
                    argValues[i].back() - '0')]);
            }
            const char* msgPointer = msg;
            while (*msgPointer)
            {
                if (*msgPointer == '%')
                {
                    msgPointer++;
                    size_t wordIndex = *msgPointer - '0';
                    if (isdigit(*msgPointer) && wordIndex >= 1 &&
                        static_cast<uint16_t>(wordIndex) <=
                            argSourceVals.size())
                    {
                        message.append(getNumberString(
                            "0x%X", argSourceVals[wordIndex - 1]));
                    }
                    else
                    {
                        message.append("%" + std::string(1, *msgPointer));
                    }
                }
                else
                {
                    message.push_back(*msgPointer);
                }
                msgPointer++;
            }
            return message;
        }
        else
        {
            return regEntry.doc.message;
        }
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Cannot get error message from registry entry",
                        entry("ERROR=%s", e.what()));
    }
    return std::nullopt;
}

std::optional<std::string> SRC::getCallouts() const
{
    if (!_callouts)
    {
        return std::nullopt;
    }
    std::string printOut;
    const std::string jsonIndent(indentLevel, 0x20);
    const auto& callout = _callouts->callouts();
    const auto& compDescrp = pv::failingComponentType;
    printOut.append(jsonIndent + "\"Callout Section\": {\n");
    jsonInsert(printOut, "Callout Count", std::to_string(callout.size()), 2);
    printOut.append(jsonIndent + jsonIndent + "\"Callouts\": [");
    for (auto& entry : callout)
    {
        printOut.append("{\n");
        if (entry->fruIdentity())
        {
            jsonInsert(
                printOut, "FRU Type",
                compDescrp.at(entry->fruIdentity()->failingComponentType()), 3);
            jsonInsert(printOut, "Priority",
                       pv::getValue(entry->priority(),
                                    pel_values::calloutPriorityValues),
                       3);
            if (!entry->locationCode().empty())
            {
                jsonInsert(printOut, "Location Code", entry->locationCode(), 3);
            }
            if (entry->fruIdentity()->getPN().has_value())
            {
                jsonInsert(printOut, "Part Number",
                           entry->fruIdentity()->getPN().value(), 3);
            }
            if (entry->fruIdentity()->getMaintProc().has_value())
            {
                jsonInsert(printOut, "Procedure Number",
                           entry->fruIdentity()->getMaintProc().value(), 3);
                if (pv::procedureDesc.find(
                        entry->fruIdentity()->getMaintProc().value()) !=
                    pv::procedureDesc.end())
                {
                    jsonInsert(
                        printOut, "Description",
                        pv::procedureDesc.at(
                            entry->fruIdentity()->getMaintProc().value()),
                        3);
                }
            }
            if (entry->fruIdentity()->getCCIN().has_value())
            {
                jsonInsert(printOut, "CCIN",
                           entry->fruIdentity()->getCCIN().value(), 3);
            }
            if (entry->fruIdentity()->getSN().has_value())
            {
                jsonInsert(printOut, "Serial Number",
                           entry->fruIdentity()->getSN().value(), 3);
            }
        }
        if (entry->pceIdentity())
        {
            const auto& pceIdentMtms = entry->pceIdentity()->mtms();
            if (!pceIdentMtms.machineTypeAndModel().empty())
            {
                jsonInsert(printOut, "PCE MTMS",
                           pceIdentMtms.machineTypeAndModel() + "_" +
                               pceIdentMtms.machineSerialNumber(),
                           3);
            }
            if (!entry->pceIdentity()->enclosureName().empty())
            {
                jsonInsert(printOut, "PCE Name",
                           entry->pceIdentity()->enclosureName(), 3);
            }
        }
        if (entry->mru())
        {
            const auto& mruCallouts = entry->mru()->mrus();
            std::string mruId;
            for (auto& element : mruCallouts)
            {
                if (!mruId.empty())
                {
                    mruId.append(", " + getNumberString("%08X", element.id));
                }
                else
                {
                    mruId.append(getNumberString("%08X", element.id));
                }
            }
            jsonInsert(printOut, "MRU Id", mruId, 3);
        }
        printOut.erase(printOut.size() - 2);
        printOut.append("\n" + jsonIndent + jsonIndent + "}, ");
    };
    printOut.erase(printOut.size() - 2);
    printOut.append("]\n" + jsonIndent + "}");
    return printOut;
}

std::optional<std::string> SRC::getJSON(message::Registry& registry) const
{
    std::string ps;
    jsonInsert(ps, pv::sectionVer, getNumberString("%d", _header.version), 1);
    jsonInsert(ps, pv::subSection, getNumberString("%d", _header.subType), 1);
    jsonInsert(ps, pv::createdBy, getNumberString("0x%X", _header.componentID),
               1);
    jsonInsert(ps, "SRC Version", getNumberString("0x%02X", _version), 1);
    jsonInsert(ps, "SRC Format", getNumberString("0x%02X", _hexData[0] & 0xFF),
               1);
    jsonInsert(ps, "Virtual Progress SRC",
               pv::boolString.at(_flags & virtualProgressSRC), 1);
    jsonInsert(ps, "I5/OS Service Event Bit",
               pv::boolString.at(_flags & i5OSServiceEventBit), 1);
    jsonInsert(ps, "Hypervisor Dump Initiated",
               pv::boolString.at(_flags & hypDumpInit), 1);
    jsonInsert(ps, "Power Control Net Fault",
               pv::boolString.at(isPowerFaultEvent()), 1);

    if (isBMCSRC())
    {
        std::string ccinString;
        uint32_t ccin = _hexData[1] >> 16;

        if (ccin)
        {
            ccinString = getNumberString("%04X", ccin);
        }
        // The PEL spec calls it a backplane, so call it that here.
        jsonInsert(ps, "Backplane CCIN", ccinString, 1);
    }

    auto errorDetails = getErrorDetails(registry, DetailLevel::json, true);
    if (errorDetails)
    {
        ps.append(errorDetails.value());
    }
    jsonInsert(ps, "Valid Word Count", getNumberString("0x%02X", _wordCount),
               1);
    std::string refcode = asciiString();
    std::string extRefcode;
    size_t pos = refcode.find(0x20);
    if (pos != std::string::npos)
    {
        size_t nextPos = refcode.find_first_not_of(0x20, pos);
        if (nextPos != std::string::npos)
        {
            extRefcode = trimEnd(refcode.substr(nextPos));
        }
        refcode.erase(pos);
    }
    jsonInsert(ps, "Reference Code", refcode, 1);
    if (!extRefcode.empty())
    {
        jsonInsert(ps, "Extended Reference Code", extRefcode, 1);
    }
    for (size_t i = 2; i <= _wordCount; i++)
    {
        jsonInsert(
            ps, "Hex Word " + std::to_string(i),
            getNumberString("%08X", _hexData[getWordIndexFromWordNum(i)]), 1);
    }
    auto calloutJson = getCallouts();
    if (calloutJson)
    {
        ps.append(calloutJson.value());
    }
    else
    {
        ps.erase(ps.size() - 2);
    }
    return ps;
}

void SRC::addCallouts(const message::Entry& regEntry,
                      const AdditionalData& additionalData,
                      const DataInterfaceBase& dataIface)
{
    auto item = additionalData.getValue("CALLOUT_INVENTORY_PATH");
    if (item)
    {
        addInventoryCallout(*item, std::nullopt, std::nullopt, dataIface);
    }

    addDevicePathCallouts(additionalData, dataIface);

    if (regEntry.callouts)
    {
        addRegistryCallouts(regEntry, additionalData, dataIface);
    }
}

void SRC::addInventoryCallout(const std::string& inventoryPath,
                              const std::optional<CalloutPriority>& priority,
                              const std::optional<std::string>& locationCode,
                              const DataInterfaceBase& dataIface)
{
    std::string locCode;
    std::string fn;
    std::string ccin;
    std::string sn;
    std::unique_ptr<src::Callout> callout;

    try
    {
        // Use the passed in location code if there otherwise look it up
        if (locationCode)
        {
            locCode = *locationCode;
        }
        else
        {
            locCode = dataIface.getLocationCode(inventoryPath);
        }

        try
        {
            dataIface.getHWCalloutFields(inventoryPath, fn, ccin, sn);

            CalloutPriority p =
                priority ? priority.value() : CalloutPriority::high;

            callout = std::make_unique<src::Callout>(p, locCode, fn, ccin, sn);
        }
        catch (const SdBusError& e)
        {
            std::string msg =
                "No VPD found for " + inventoryPath + ": " + e.what();
            addDebugData(msg);

            // Just create the callout with empty FRU fields
            callout = std::make_unique<src::Callout>(CalloutPriority::high,
                                                     locCode, fn, ccin, sn);
        }
    }
    catch (const SdBusError& e)
    {
        std::string msg = "Could not get location code for " + inventoryPath +
                          ": " + e.what();
        addDebugData(msg);

        // If this were to happen, people would have to look in the UserData
        // section that contains CALLOUT_INVENTORY_PATH to see what failed.
        callout = std::make_unique<src::Callout>(CalloutPriority::high,
                                                 "no_vpd_for_fru");
    }

    createCalloutsObject();
    _callouts->addCallout(std::move(callout));
}

void SRC::addRegistryCallouts(const message::Entry& regEntry,
                              const AdditionalData& additionalData,
                              const DataInterfaceBase& dataIface)
{
    try
    {
        auto systemNames = dataIface.getSystemNames();

        auto regCallouts = message::Registry::getCallouts(
            regEntry.callouts.value(), systemNames, additionalData);

        for (const auto& regCallout : regCallouts)
        {
            addRegistryCallout(regCallout, dataIface);
        }
    }
    catch (std::exception& e)
    {
        std::string msg =
            "Error parsing PEL message registry callout JSON: "s + e.what();
        addDebugData(msg);
    }
}

void SRC::addRegistryCallout(const message::RegistryCallout& regCallout,
                             const DataInterfaceBase& dataIface)
{
    std::unique_ptr<src::Callout> callout;
    auto locCode = regCallout.locCode;

    if (!locCode.empty())
    {
        try
        {
            locCode = dataIface.expandLocationCode(locCode, 0);
        }
        catch (const std::exception& e)
        {
            auto msg =
                "Unable to expand location code " + locCode + ": " + e.what();
            addDebugData(msg);
            return;
        }
    }

    // Via the PEL values table, get the priority enum.
    // The schema will have validated the priority was a valid value.
    auto priorityIt =
        pv::findByName(regCallout.priority, pv::calloutPriorityValues);
    assert(priorityIt != pv::calloutPriorityValues.end());
    auto priority =
        static_cast<CalloutPriority>(std::get<pv::fieldValuePos>(*priorityIt));

    if (!regCallout.procedure.empty())
    {
        // Procedure callout
        callout =
            std::make_unique<src::Callout>(priority, regCallout.procedure);
    }
    else if (!regCallout.symbolicFRU.empty())
    {
        // Symbolic FRU callout
        callout = std::make_unique<src::Callout>(
            priority, regCallout.symbolicFRU, locCode, false);
    }
    else if (!regCallout.symbolicFRUTrusted.empty())
    {
        // Symbolic FRU with trusted location code callout

        // The registry wants it to be trusted, but that requires a valid
        // location code for it to actually be.
        callout = std::make_unique<src::Callout>(
            priority, regCallout.symbolicFRUTrusted, locCode, !locCode.empty());
    }
    else
    {
        // A hardware callout
        std::string inventoryPath;

        try
        {
            // Get the inventory item from the unexpanded location code
            inventoryPath =
                dataIface.getInventoryFromLocCode(regCallout.locCode, 0);
        }
        catch (const std::exception& e)
        {
            std::string msg =
                "Unable to get inventory path from location code: " + locCode +
                ": " + e.what();
            addDebugData(msg);
            return;
        }

        addInventoryCallout(inventoryPath, priority, locCode, dataIface);
    }

    if (callout)
    {
        createCalloutsObject();
        _callouts->addCallout(std::move(callout));
    }
}

void SRC::addDevicePathCallouts(const AdditionalData& additionalData,
                                const DataInterfaceBase& dataIface)
{
    std::vector<device_callouts::Callout> callouts;
    auto i2cBus = additionalData.getValue("CALLOUT_IIC_BUS");
    auto i2cAddr = additionalData.getValue("CALLOUT_IIC_ADDR");
    auto devPath = additionalData.getValue("CALLOUT_DEVICE_PATH");

    // A device callout contains either:
    // * CALLOUT_ERRNO, CALLOUT_DEVICE_PATH
    // * CALLOUT_ERRNO, CALLOUT_IIC_BUS, CALLOUT_IIC_ADDR
    // We don't care about the errno.

    if (devPath)
    {
        try
        {
            callouts = device_callouts::getCallouts(*devPath,
                                                    dataIface.getSystemNames());
        }
        catch (const std::exception& e)
        {
            addDebugData(e.what());
            callouts.clear();
        }
    }
    else if (i2cBus && i2cAddr)
    {
        size_t bus;
        uint8_t address;

        try
        {
            // If /dev/i2c- is prepended, remove it
            if (i2cBus->find("/dev/i2c-") != std::string::npos)
            {
                *i2cBus = i2cBus->substr(9);
            }

            bus = stoul(*i2cBus, nullptr, 0);
            address = stoul(*i2cAddr, nullptr, 0);
        }
        catch (const std::exception& e)
        {
            std::string msg = "Invalid CALLOUT_IIC_BUS " + *i2cBus +
                              " or CALLOUT_IIC_ADDR " + *i2cAddr +
                              " in AdditionalData property";
            addDebugData(msg);
            return;
        }

        try
        {
            callouts = device_callouts::getI2CCallouts(
                bus, address, dataIface.getSystemNames());
        }
        catch (const std::exception& e)
        {
            addDebugData(e.what());
            callouts.clear();
        }
    }

    for (const auto& callout : callouts)
    {
        // The priority shouldn't be invalid, but check just in case.
        CalloutPriority priority = CalloutPriority::high;

        if (!callout.priority.empty())
        {
            auto p = pel_values::findByValue(
                static_cast<uint32_t>(callout.priority[0]),
                pel_values::calloutPriorityValues);

            if (p != pel_values::calloutPriorityValues.end())
            {
                priority = static_cast<CalloutPriority>(callout.priority[0]);
            }
            else
            {
                std::string msg =
                    "Invalid priority found in dev callout JSON: " +
                    callout.priority[0];
                addDebugData(msg);
            }
        }

        try
        {
            auto inventoryPath =
                dataIface.getInventoryFromLocCode(callout.locationCode, 0);

            addInventoryCallout(inventoryPath, priority, std::nullopt,
                                dataIface);
        }
        catch (const std::exception& e)
        {
            std::string msg =
                "Unable to get inventory path from location code: " +
                callout.locationCode + ": " + e.what();
            addDebugData(msg);
        }

        // Until the code is there to convert these MRU value strings to
        // the official MRU values in the callout objects, just store
        // the MRU name in the debug UserData section.
        if (!callout.mru.empty())
        {
            std::string msg = "MRU: " + callout.mru;
            addDebugData(msg);
        }

        // getCallouts() may have generated some debug data it stored
        // in a callout object.  Save it as well.
        if (!callout.debug.empty())
        {
            addDebugData(callout.debug);
        }
    }
}

} // namespace pels
} // namespace openpower
