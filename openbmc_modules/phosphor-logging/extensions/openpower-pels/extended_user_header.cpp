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
#include "extended_user_header.hpp"

#include "json_utils.hpp"
#include "pel_types.hpp"
#include "pel_values.hpp"

#include <phosphor-logging/log.hpp>

namespace openpower
{
namespace pels
{

namespace pv = openpower::pels::pel_values;
using namespace phosphor::logging;
const size_t defaultSymptomIDWord = 3;
const size_t symptomIDMaxSize = 80;

ExtendedUserHeader::ExtendedUserHeader(Stream& pel)
{
    try
    {
        unflatten(pel);
        validate();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Cannot unflatten extended user header",
                        entry("ERROR=%s", e.what()));
        _valid = false;
    }
}

ExtendedUserHeader::ExtendedUserHeader(const DataInterfaceBase& dataIface,
                                       const message::Entry& regEntry,
                                       const SRC& src) :
    _mtms(dataIface.getMachineTypeModel(), dataIface.getMachineSerialNumber())
{
    _header.id = static_cast<uint16_t>(SectionID::extendedUserHeader);
    _header.version = extendedUserHeaderVersion;
    _header.subType = 0;
    _header.componentID = static_cast<uint16_t>(ComponentID::phosphorLogging);

    memset(_serverFWVersion.data(), 0, _serverFWVersion.size());
    auto version = dataIface.getServerFWVersion();

    // The last byte must always be the NULL terminator
    for (size_t i = 0; i < version.size() && i < firmwareVersionSize - 1; i++)
    {
        _serverFWVersion[i] = version[i];
    }

    memset(_subsystemFWVersion.data(), 0, _subsystemFWVersion.size());
    version = dataIface.getBMCFWVersion();

    // The last byte must always be the NULL terminator
    for (size_t i = 0; i < version.size() && i < firmwareVersionSize - 1; i++)
    {
        _subsystemFWVersion[i] = version[i];
    }

    createSymptomID(regEntry, src);

    _header.size = flattenedSize();
    _valid = true;
}

void ExtendedUserHeader::flatten(Stream& pel) const
{
    pel << _header << _mtms;
    pel.write(_serverFWVersion.data(), _serverFWVersion.size());
    pel.write(_subsystemFWVersion.data(), _subsystemFWVersion.size());
    pel << _reserved4B << _refTime << _reserved1B1 << _reserved1B2
        << _reserved1B3 << _symptomIDSize << _symptomID;
}

void ExtendedUserHeader::unflatten(Stream& pel)
{
    pel >> _header >> _mtms;
    pel.read(_serverFWVersion.data(), _serverFWVersion.size());
    pel.read(_subsystemFWVersion.data(), _subsystemFWVersion.size());
    pel >> _reserved4B >> _refTime >> _reserved1B1 >> _reserved1B2 >>
        _reserved1B3 >> _symptomIDSize;

    _symptomID.resize(_symptomIDSize);
    pel >> _symptomID;
}

void ExtendedUserHeader::validate()
{
    bool failed = false;

    if (header().id != static_cast<uint16_t>(SectionID::extendedUserHeader))
    {
        log<level::ERR>("Invalid failing Extended User Header section ID",
                        entry("ID=0x%X", header().id));
        failed = true;
    }

    if (header().version != extendedUserHeaderVersion)
    {
        log<level::ERR>("Invalid Extended User Header version",
                        entry("VERSION=0x%X", header().version));
        failed = true;
    }

    _valid = (failed) ? false : true;
}

void ExtendedUserHeader::createSymptomID(const message::Entry& regEntry,
                                         const SRC& src)
{
    // Contains the first 8 characters of the ASCII string plus additional
    // words from the SRC, separated by underscores.  The message registry
    // says which words to use, though that's optional and if not present
    // then use a default word.
    std::vector<size_t> idWords;

    if (regEntry.src.symptomID)
    {
        idWords = regEntry.src.symptomID.value();
    }
    else
    {
        idWords.push_back(defaultSymptomIDWord);
    }

    auto symptomID = src.asciiString().substr(0, 8);

    const auto& hexWords = src.hexwordData();

    for (auto wordNum : idWords)
    {
        symptomID.push_back('_');

        // Get the hexword array index for this SRC word
        auto index = src.getWordIndexFromWordNum(wordNum);

        // Convert to ASCII
        char word[20];
        sprintf(word, "%08X", hexWords[index]);
        symptomID += word;
    }

    std::copy(symptomID.begin(), symptomID.end(),
              std::back_inserter(_symptomID));

    // Max total size is 80, including the upcoming NULL
    if (_symptomID.size() > (symptomIDMaxSize - 1))
    {
        _symptomID.resize(symptomIDMaxSize - 1);
    }

    // NULL terminated
    _symptomID.push_back(0);

    // PAD with NULLs to a 4 byte boundary
    while ((_symptomID.size() % 4) != 0)
    {
        _symptomID.push_back(0);
    }

    _symptomIDSize = _symptomID.size();
}

std::optional<std::string> ExtendedUserHeader::getJSON() const
{
    std::string json;
    jsonInsert(json, pv::sectionVer, getNumberString("%d", _header.version), 1);
    jsonInsert(json, pv::subSection, getNumberString("%d", _header.subType), 1);
    jsonInsert(json, pv::createdBy,
               getNumberString("0x%X", _header.componentID), 1);
    jsonInsert(json, "Reporting Machine Type", machineTypeModel(), 1);
    jsonInsert(json, "Reporting Serial Number", trimEnd(machineSerialNumber()),
               1);
    jsonInsert(json, "FW Released Ver", serverFWVersion(), 1);
    jsonInsert(json, "FW SubSys Version", subsystemFWVersion(), 1);
    jsonInsert(json, "Common Ref Time",
               getNumberString("%02X", _refTime.month) + '/' +
                   getNumberString("%02X", _refTime.day) + '/' +
                   getNumberString("%02X", _refTime.yearMSB) +
                   getNumberString("%02X", _refTime.yearLSB) + ' ' +
                   getNumberString("%02X", _refTime.hour) + ':' +
                   getNumberString("%02X", _refTime.minutes) + ':' +
                   getNumberString("%02X", _refTime.seconds),
               1);
    jsonInsert(json, "Symptom Id Len", getNumberString("%d", _symptomIDSize),
               1);
    jsonInsert(json, "Symptom Id", symptomID(), 1);
    json.erase(json.size() - 2);
    return json;
}

} // namespace pels
} // namespace openpower
