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
#include "user_header.hpp"

#include "json_utils.hpp"
#include "pel_types.hpp"
#include "pel_values.hpp"
#include "severity.hpp"

#include <iostream>
#include <phosphor-logging/log.hpp>

namespace openpower
{
namespace pels
{

namespace pv = openpower::pels::pel_values;
using namespace phosphor::logging;

void UserHeader::unflatten(Stream& stream)
{
    stream >> _header >> _eventSubsystem >> _eventScope >> _eventSeverity >>
        _eventType >> _reserved4Byte1 >> _problemDomain >> _problemVector >>
        _actionFlags >> _states;
}

void UserHeader::flatten(Stream& stream) const
{
    stream << _header << _eventSubsystem << _eventScope << _eventSeverity
           << _eventType << _reserved4Byte1 << _problemDomain << _problemVector
           << _actionFlags << _states;
}

UserHeader::UserHeader(const message::Entry& entry,
                       phosphor::logging::Entry::Level severity,
                       const DataInterfaceBase& dataIface)
{
    _header.id = static_cast<uint16_t>(SectionID::userHeader);
    _header.size = UserHeader::flattenedSize();
    _header.version = userHeaderVersion;
    _header.subType = 0;
    _header.componentID = static_cast<uint16_t>(ComponentID::phosphorLogging);

    _eventSubsystem = entry.subsystem;

    _eventScope = entry.eventScope.value_or(
        static_cast<uint8_t>(EventScope::entirePlatform));

    // Get the severity from the registry if it's there, otherwise get it
    // from the OpenBMC event log severity value.
    if (!entry.severity)
    {
        _eventSeverity = convertOBMCSeverityToPEL(severity);
    }
    else
    {
        // Find the severity possibly dependent on the system type.
        auto sev =
            getSeverity(entry.severity.value(), dataIface.getSystemNames());
        if (sev)
        {
            _eventSeverity = *sev;
        }
        else
        {
            // Someone screwed up the message registry.
            std::string types;
            const auto& compatibles = dataIface.getSystemNames();
            std::for_each(compatibles.begin(), compatibles.end(),
                          [&types](const auto& t) { types += t + '|'; });

            log<level::ERR>(
                "No severity entry found for this error and system name",
                phosphor::logging::entry("ERROR=%s", entry.name.c_str()),
                phosphor::logging::entry("SYSTEMNAMES=%s", types.c_str()));

            // Have to choose something, just use informational.
            _eventSeverity = 0;
        }
    }

    // TODO: ibm-dev/dev/#1144 Handle manufacturing sev & action flags

    if (entry.eventType)
    {
        _eventType = *entry.eventType;
    }
    else
    {
        // There are different default event types for info errors
        // vs non info ones.
        auto sevType = static_cast<SeverityType>(_eventSeverity & 0xF0);

        _eventType = (sevType == SeverityType::nonError)
                         ? static_cast<uint8_t>(EventType::miscInformational)
                         : static_cast<uint8_t>(EventType::notApplicable);
    }

    _reserved4Byte1 = 0;

    // No uses for problem domain or vector
    _problemDomain = 0;
    _problemVector = 0;

    // These will be cleaned up later in pel_rules::check()
    _actionFlags = entry.actionFlags.value_or(0);

    _states = 0;

    _valid = true;
}

UserHeader::UserHeader(Stream& pel)
{
    try
    {
        unflatten(pel);
        validate();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Cannot unflatten user header",
                        entry("ERROR=%s", e.what()));
        _valid = false;
    }
}

void UserHeader::validate()
{
    bool failed = false;
    if (header().id != static_cast<uint16_t>(SectionID::userHeader))
    {
        log<level::ERR>("Invalid user header section ID",
                        entry("ID=0x%X", header().id));
        failed = true;
    }

    if (header().version != userHeaderVersion)
    {
        log<level::ERR>("Invalid user header version",
                        entry("VERSION=0x%X", header().version));
        failed = true;
    }

    _valid = (failed) ? false : true;
}

std::optional<std::string> UserHeader::getJSON() const
{
    std::string severity;
    std::string subsystem;
    std::string eventScope;
    std::string eventType;
    std::vector<std::string> actionFlags;
    severity = pv::getValue(_eventSeverity, pel_values::severityValues);
    subsystem = pv::getValue(_eventSubsystem, pel_values::subsystemValues);
    eventScope = pv::getValue(_eventScope, pel_values::eventScopeValues);
    eventType = pv::getValue(_eventType, pel_values::eventTypeValues);
    actionFlags =
        pv::getValuesBitwise(_actionFlags, pel_values::actionFlagsValues);

    std::string hostState{"Invalid"};
    auto iter = pv::transmissionStates.find(
        static_cast<TransmissionState>(hostTransmissionState()));
    if (iter != pv::transmissionStates.end())
    {
        hostState = iter->second;
    }

    std::string uh;
    jsonInsert(uh, pv::sectionVer, getNumberString("%d", userHeaderVersion), 1);
    jsonInsert(uh, pv::subSection, getNumberString("%d", _header.subType), 1);
    jsonInsert(uh, "Log Committed by",
               getNumberString("0x%X", _header.componentID), 1);
    jsonInsert(uh, "Subsystem", subsystem, 1);
    jsonInsert(uh, "Event Scope", eventScope, 1);
    jsonInsert(uh, "Event Severity", severity, 1);
    jsonInsert(uh, "Event Type", eventType, 1);
    jsonInsertArray(uh, "Action Flags", actionFlags, 1);
    jsonInsert(uh, "Host Transmission", hostState, 1);
    uh.erase(uh.size() - 2);
    return uh;
}

std::optional<uint8_t> UserHeader::getSeverity(
    const std::vector<message::RegistrySeverity>& severities,
    const std::vector<std::string>& systemNames) const
{
    const uint8_t* s = nullptr;

    // Find the severity to use for this system type, or use the default
    // entry (where no system type is specified).
    for (const auto& sev : severities)
    {
        if (std::find(systemNames.begin(), systemNames.end(), sev.system) !=
            systemNames.end())
        {
            s = &sev.severity;
            break;
        }
        else if (sev.system.empty())
        {
            s = &sev.severity;
        }
    }

    if (s)
    {
        return *s;
    }

    return std::nullopt;
}

} // namespace pels
} // namespace openpower
