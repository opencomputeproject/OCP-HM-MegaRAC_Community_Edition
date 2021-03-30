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
#include "registry.hpp"

#include "pel_types.hpp"
#include "pel_values.hpp"

#include <fstream>
#include <phosphor-logging/log.hpp>

namespace openpower
{
namespace pels
{
namespace message
{

namespace pv = pel_values;
namespace fs = std::filesystem;
using namespace phosphor::logging;

constexpr auto debugFilePath = "/etc/phosphor-logging/";

namespace helper
{

uint8_t getSubsystem(const std::string& subsystemName)
{
    // Get the actual value to use in the PEL for the string name
    auto ss = pv::findByName(subsystemName, pv::subsystemValues);
    if (ss == pv::subsystemValues.end())
    {
        // Schema validation should be catching this.
        log<level::ERR>("Invalid subsystem name used in message registry",
                        entry("SUBSYSTEM=%s", subsystemName.c_str()));

        throw std::runtime_error("Invalid subsystem used in message registry");
    }

    return std::get<pv::fieldValuePos>(*ss);
}

uint8_t getSeverity(const std::string& severityName)
{
    auto s = pv::findByName(severityName, pv::severityValues);
    if (s == pv::severityValues.end())
    {
        // Schema validation should be catching this.
        log<level::ERR>("Invalid severity name used in message registry",
                        entry("SEVERITY=%s", severityName.c_str()));

        throw std::runtime_error("Invalid severity used in message registry");
    }

    return std::get<pv::fieldValuePos>(*s);
}

std::vector<RegistrySeverity> getSeverities(const nlohmann::json& severity)
{
    std::vector<RegistrySeverity> severities;

    // The plain string value, like "unrecoverable"
    if (severity.is_string())
    {
        RegistrySeverity s;
        s.severity = getSeverity(severity.get<std::string>());
        severities.push_back(std::move(s));
    }
    else
    {
        // An array, with an element like:
        // {
        //    "SevValue": "unrecoverable",
        //    "System", "systemA"
        // }
        for (const auto& sev : severity)
        {
            RegistrySeverity s;
            s.severity = getSeverity(sev["SevValue"].get<std::string>());

            if (sev.contains("System"))
            {
                s.system = sev["System"].get<std::string>();
            }

            severities.push_back(std::move(s));
        }
    }

    return severities;
}

uint16_t getActionFlags(const std::vector<std::string>& flags)
{
    uint16_t actionFlags = 0;

    // Make the bitmask based on the array of flag names
    for (const auto& flag : flags)
    {
        auto s = pv::findByName(flag, pv::actionFlagsValues);
        if (s == pv::actionFlagsValues.end())
        {
            // Schema validation should be catching this.
            log<level::ERR>("Invalid action flag name used in message registry",
                            entry("FLAG=%s", flag.c_str()));

            throw std::runtime_error(
                "Invalid action flag used in message registry");
        }

        actionFlags |= std::get<pv::fieldValuePos>(*s);
    }

    return actionFlags;
}

uint8_t getEventType(const std::string& eventTypeName)
{
    auto t = pv::findByName(eventTypeName, pv::eventTypeValues);
    if (t == pv::eventTypeValues.end())
    {
        log<level::ERR>("Invalid event type used in message registry",
                        entry("EVENT_TYPE=%s", eventTypeName.c_str()));

        throw std::runtime_error("Invalid event type used in message registry");
    }
    return std::get<pv::fieldValuePos>(*t);
}

uint8_t getEventScope(const std::string& eventScopeName)
{
    auto s = pv::findByName(eventScopeName, pv::eventScopeValues);
    if (s == pv::eventScopeValues.end())
    {
        log<level::ERR>("Invalid event scope used in registry",
                        entry("EVENT_SCOPE=%s", eventScopeName.c_str()));

        throw std::runtime_error(
            "Invalid event scope used in message registry");
    }
    return std::get<pv::fieldValuePos>(*s);
}

uint16_t getSRCReasonCode(const nlohmann::json& src, const std::string& name)
{
    std::string rc = src["ReasonCode"];
    uint16_t reasonCode = strtoul(rc.c_str(), nullptr, 16);
    if (reasonCode == 0)
    {
        log<phosphor::logging::level::ERR>(
            "Invalid reason code in message registry",
            entry("ERROR_NAME=%s", name.c_str()),
            entry("REASON_CODE=%s", rc.c_str()));

        throw std::runtime_error("Invalid reason code in message registry");
    }
    return reasonCode;
}

uint8_t getSRCType(const nlohmann::json& src, const std::string& name)
{
    // Looks like: "22"
    std::string srcType = src["Type"];
    size_t type = strtoul(srcType.c_str(), nullptr, 16);
    if ((type == 0) || (srcType.size() != 2)) // 1 hex byte
    {
        log<phosphor::logging::level::ERR>(
            "Invalid SRC Type in message registry",
            entry("ERROR_NAME=%s", name.c_str()),
            entry("SRC_TYPE=%s", srcType.c_str()));

        throw std::runtime_error("Invalid SRC Type in message registry");
    }

    return type;
}

std::optional<std::map<SRC::WordNum, SRC::AdditionalDataField>>
    getSRCHexwordFields(const nlohmann::json& src, const std::string& name)
{
    std::map<SRC::WordNum, SRC::AdditionalDataField> hexwordFields;

    // Build the map of which AdditionalData fields to use for which SRC words

    // Like:
    // {
    //   "8":
    //   {
    //     "AdditionalDataPropSource": "TEST"
    //   }
    //
    // }

    for (const auto& word : src["Words6To9"].items())
    {
        std::string num = word.key();
        size_t wordNum = std::strtoul(num.c_str(), nullptr, 10);

        if (wordNum == 0)
        {
            log<phosphor::logging::level::ERR>(
                "Invalid SRC word number in message registry",
                entry("ERROR_NAME=%s", name.c_str()),
                entry("SRC_WORD_NUM=%s", num.c_str()));

            throw std::runtime_error("Invalid SRC word in message registry");
        }

        auto attributes = word.value();
        std::string adPropName = attributes["AdditionalDataPropSource"];
        hexwordFields[wordNum] = std::move(adPropName);
    }

    if (!hexwordFields.empty())
    {
        return hexwordFields;
    }

    return std::nullopt;
}
std::optional<std::vector<SRC::WordNum>>
    getSRCSymptomIDFields(const nlohmann::json& src, const std::string& name)
{
    std::vector<SRC::WordNum> symptomIDFields;

    // Looks like:
    // "SymptomIDFields": ["SRCWord3", "SRCWord6"],

    for (const std::string& field : src["SymptomIDFields"])
    {
        // Just need the last digit off the end, e.g. SRCWord6.
        // The schema enforces the format of these.
        auto srcWordNum = field.substr(field.size() - 1);
        size_t num = std::strtoul(srcWordNum.c_str(), nullptr, 10);
        if (num == 0)
        {
            log<phosphor::logging::level::ERR>(
                "Invalid symptom ID field in message registry",
                entry("ERROR_NAME=%s", name.c_str()),
                entry("FIELD_NAME=%s", srcWordNum.c_str()));

            throw std::runtime_error("Invalid symptom ID in message registry");
        }
        symptomIDFields.push_back(num);
    }
    if (!symptomIDFields.empty())
    {
        return symptomIDFields;
    }

    return std::nullopt;
}

uint16_t getComponentID(uint8_t srcType, uint16_t reasonCode,
                        const nlohmann::json& pelEntry, const std::string& name)
{
    uint16_t id = 0;

    // If the ComponentID field is there, use that.  Otherwise, if it's a
    // 0xBD BMC error SRC, use the reasoncode.
    if (pelEntry.contains("ComponentID"))
    {
        std::string componentID = pelEntry["ComponentID"];
        id = strtoul(componentID.c_str(), nullptr, 16);
    }
    else
    {
        // On BMC error SRCs (BD), can just get the component ID from
        // the first byte of the reason code.
        if (srcType == static_cast<uint8_t>(SRCType::bmcError))
        {
            id = reasonCode & 0xFF00;
        }
        else
        {
            log<level::ERR>("Missing component ID field in message registry",
                            entry("ERROR_NAME=%s", name.c_str()));

            throw std::runtime_error(
                "Missing component ID field in message registry");
        }
    }

    return id;
}

/**
 * @brief Says if the JSON is the format that contains AdditionalData keys
 *        as in index into them.
 *
 * @param[in] json - The highest level callout JSON
 *
 * @return bool - If it is the AdditionalData format or not
 */
bool calloutUsesAdditionalData(const nlohmann::json& json)
{
    return (json.contains("ADName") &&
            json.contains("CalloutsWithTheirADValues"));
}

/**
 * @brief Finds the callouts to use when there is no AdditionalData,
 *        but the system type may be used as a key.
 *
 * One entry in the array looks like the following.  The System key
 * is optional and if not present it means that entry applies to
 * every configuration that doesn't have another entry with a matching
 * System key.
 *
 *    {
 *        "System": "system1",
 *        "CalloutList":
 *        [
 *            {
 *                "Priority": "high",
 *                "LocCode": "P1-C1"
 *            },
 *            {
 *                "Priority": "low",
 *                "LocCode": "P1"
 *            }
 *        ]
 *    }
 */
const nlohmann::json&
    findCalloutList(const nlohmann::json& json,
                    const std::vector<std::string>& systemNames)
{
    const nlohmann::json* callouts = nullptr;

    if (!json.is_array())
    {
        throw std::runtime_error{"findCalloutList was not passed a JSON array"};
    }

    // The entry with the system type match will take precedence over the entry
    // without any "System" field in it at all, which will match all other
    // cases.
    for (const auto& calloutList : json)
    {
        if (calloutList.contains("System"))
        {
            if (std::find(systemNames.begin(), systemNames.end(),
                          calloutList["System"].get<std::string>()) !=
                systemNames.end())
            {
                callouts = &calloutList["CalloutList"];
                break;
            }
        }
        else
        {
            // Any entry with no System key
            callouts = &calloutList["CalloutList"];
        }
    }

    if (!callouts)
    {
        std::string types;
        std::for_each(systemNames.begin(), systemNames.end(),
                      [&types](const auto& t) { types += t + '|'; });
        log<level::WARNING>(
            "No matching system name entry or default system name entry "
            " for PEL callout list",
            entry("SYSTEMNAMES=%s", types.c_str()));

        throw std::runtime_error{
            "Could not find a CalloutList JSON for this error and system name"};
    }

    return *callouts;
}

/**
 * @brief Creates a RegistryCallout based on the input JSON.
 *
 * The JSON looks like:
 *     {
 *          "Priority": "high",
 *          "LocCode": "E1"
 *          ...
 *     }
 *
 * Schema validation enforces what keys are present.
 *
 * @param[in] json - The JSON dictionary entry for a callout
 *
 * @return RegistryCallout - A filled in RegistryCallout
 */
RegistryCallout makeRegistryCallout(const nlohmann::json& json)
{
    RegistryCallout callout;

    callout.priority = "high";

    if (json.contains("Priority"))
    {
        callout.priority = json["Priority"].get<std::string>();
    }

    if (json.contains("LocCode"))
    {
        callout.locCode = json["LocCode"].get<std::string>();
    }

    if (json.contains("Procedure"))
    {
        callout.procedure = json["Procedure"].get<std::string>();
    }
    else if (json.contains("SymbolicFRU"))
    {
        callout.symbolicFRU = json["SymbolicFRU"].get<std::string>();
    }
    else if (json.contains("SymbolicFRUTrusted"))
    {
        callout.symbolicFRUTrusted =
            json["SymbolicFRUTrusted"].get<std::string>();
    }

    return callout;
}

/**
 * @brief Returns the callouts to use when an AdditionalData key is
 *        required to find the correct entries.
 *
 *       The System property is used to find which CalloutList to use.
 *       If System is missing, then that CalloutList is valid for
 *       everything.
 *
 * The JSON looks like:
 *    [
 *        {
 *            "System": "systemA",
 *            "CalloutList":
 *            [
 *                {
 *                    "Priority": "high",
 *                    "LocCode": "P1-C5"
 *                }
 *            ]
 *         }
 *    ]
 *
 * @param[in] json - The callout JSON
 * @param[in] systemNames - List of compatible system type names
 *
 * @return std::vector<RegistryCallout> - The callouts to use
 */
std::vector<RegistryCallout>
    getCalloutsWithoutAD(const nlohmann::json& json,
                         const std::vector<std::string>& systemNames)
{
    std::vector<RegistryCallout> calloutEntries;

    // Find the CalloutList to use based on the system type
    const auto& calloutList = findCalloutList(json, systemNames);

    // We finally found the callouts, make the objects.
    for (const auto& callout : calloutList)
    {
        calloutEntries.push_back(std::move(makeRegistryCallout(callout)));
    }

    return calloutEntries;
}

/**
 * @brief Returns the callouts to use when an AdditionalData key is
 *        required to find the correct entries.
 *
 * The JSON looks like:
 *    {
 *        "ADName": "PROC_NUM",
 *        "CalloutsWithTheirADValues":
 *        [
 *            {
 *                "ADValue": "0",
 *                "Callouts":
 *                [
 *                    {
 *                        "CalloutList":
 *                        [
 *                            {
 *                                "Priority": "high",
 *                                "LocCode": "P1-C5"
 *                            }
 *                        ]
 *                    }
 *                ]
 *            }
 *        ]
 *     }
 *
 * Note that the "Callouts" entry above is the same as the top level
 * entry used when there is no AdditionalData key.
 *
 * @param[in] json - The callout JSON
 * @param[in] systemNames - List of compatible system type names
 * @param[in] additionalData - The AdditionalData property
 *
 * @return std::vector<RegistryCallout> - The callouts to use
 */
std::vector<RegistryCallout>
    getCalloutsUsingAD(const nlohmann::json& json,
                       const std::vector<std::string>& systemNames,
                       const AdditionalData& additionalData)
{
    // This indicates which AD field we'll be using
    auto keyName = json["ADName"].get<std::string>();

    // Get the actual value from the AD data
    auto adValue = additionalData.getValue(keyName);

    if (!adValue)
    {
        // The AdditionalData did not contain the necessary key
        log<level::WARNING>(
            "The PEL message registry callouts JSON "
            "said to use an AdditionalData key that isn't in the "
            "AdditionalData event log property",
            entry("ADNAME=%s\n", keyName.c_str()));
        throw std::runtime_error{
            "Missing AdditionalData entry for this callout"};
    }

    const auto& callouts = json["CalloutsWithTheirADValues"];

    // find the entry with that AD value
    auto it = std::find_if(
        callouts.begin(), callouts.end(), [adValue](const nlohmann::json& j) {
            return *adValue == j["ADValue"].get<std::string>();
        });

    if (it == callouts.end())
    {
        log<level::WARNING>(
            "No callout entry found for the AdditionalData value used",
            entry("AD_VALUE=%s", adValue->c_str()));

        throw std::runtime_error{
            "No callout entry found for the AdditionalData value used"};
    }

    // Proceed to find the callouts possibly based on system type.
    return getCalloutsWithoutAD((*it)["Callouts"], systemNames);
}

} // namespace helper

std::optional<Entry> Registry::lookup(const std::string& name, LookupType type,
                                      bool toCache)
{
    std::optional<nlohmann::json> registryTmp;
    auto& registryOpt = (_registry) ? _registry : registryTmp;
    if (!registryOpt)
    {
        registryOpt = readRegistry(_registryFile);
        if (!registryOpt)
        {
            return std::nullopt;
        }
        else if (toCache)
        {
            // Save message registry in memory for peltool
            _registry = std::move(registryTmp);
        }
    }
    auto& reg = (_registry) ? _registry : registryTmp;
    const auto& registry = reg.value();
    // Find an entry with this name in the PEL array.
    auto e = std::find_if(
        registry["PELs"].begin(), registry["PELs"].end(),
        [&name, &type](const auto& j) {
            return ((name == j["Name"] && type == LookupType::name) ||
                    (name == j["SRC"]["ReasonCode"] &&
                     type == LookupType::reasonCode));
        });

    if (e != registry["PELs"].end())
    {
        // Fill in the Entry structure from the JSON.  Most, but not all, fields
        // are optional.

        try
        {
            Entry entry;
            entry.name = (*e)["Name"];
            entry.subsystem = helper::getSubsystem((*e)["Subsystem"]);

            if (e->contains("ActionFlags"))
            {
                entry.actionFlags = helper::getActionFlags((*e)["ActionFlags"]);
            }

            if (e->contains("MfgActionFlags"))
            {
                entry.mfgActionFlags =
                    helper::getActionFlags((*e)["MfgActionFlags"]);
            }

            if (e->contains("Severity"))
            {
                entry.severity = helper::getSeverities((*e)["Severity"]);
            }

            if (e->contains("MfgSeverity"))
            {
                entry.mfgSeverity = helper::getSeverities((*e)["MfgSeverity"]);
            }

            if (e->contains("EventType"))
            {
                entry.eventType = helper::getEventType((*e)["EventType"]);
            }

            if (e->contains("EventScope"))
            {
                entry.eventScope = helper::getEventScope((*e)["EventScope"]);
            }

            auto& src = (*e)["SRC"];
            entry.src.reasonCode = helper::getSRCReasonCode(src, name);

            if (src.contains("Type"))
            {
                entry.src.type = helper::getSRCType(src, name);
            }
            else
            {
                entry.src.type = static_cast<uint8_t>(SRCType::bmcError);
            }

            // Now that we know the SRC type and reason code,
            // we can get the component ID.
            entry.componentID = helper::getComponentID(
                entry.src.type, entry.src.reasonCode, *e, name);

            if (src.contains("Words6To9"))
            {
                entry.src.hexwordADFields =
                    helper::getSRCHexwordFields(src, name);
            }

            if (src.contains("SymptomIDFields"))
            {
                entry.src.symptomID = helper::getSRCSymptomIDFields(src, name);
            }

            if (src.contains("PowerFault"))
            {
                entry.src.powerFault = src["PowerFault"];
            }

            auto& doc = (*e)["Documentation"];
            entry.doc.message = doc["Message"];
            entry.doc.description = doc["Description"];
            if (doc.contains("MessageArgSources"))
            {
                entry.doc.messageArgSources = doc["MessageArgSources"];
            }

            // If there are callouts defined, save the JSON for later
            if (_loadCallouts)
            {
                if (e->contains("Callouts"))
                {
                    entry.callouts = (*e)["Callouts"];
                }
                else if (e->contains("CalloutsUsingAD"))
                {
                    entry.callouts = (*e)["CalloutsUsingAD"];
                }
            }

            return entry;
        }
        catch (std::exception& e)
        {
            log<level::ERR>("Found invalid message registry field",
                            entry("ERROR=%s", e.what()));
        }
    }

    return std::nullopt;
}

std::optional<nlohmann::json>
    Registry::readRegistry(const std::filesystem::path& registryFile)
{
    // Look in /etc first in case someone put a test file there
    fs::path debugFile{fs::path{debugFilePath} / registryFileName};
    nlohmann::json registry;
    std::ifstream file;

    if (fs::exists(debugFile))
    {
        log<level::INFO>("Using debug PEL message registry");
        file.open(debugFile);
    }
    else
    {
        file.open(registryFile);
    }

    try
    {
        registry = nlohmann::json::parse(file);
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Error parsing message registry JSON",
                        entry("JSON_ERROR=%s", e.what()));
        return std::nullopt;
    }
    return registry;
}

std::vector<RegistryCallout>
    Registry::getCallouts(const nlohmann::json& calloutJSON,
                          const std::vector<std::string>& systemNames,
                          const AdditionalData& additionalData)
{
    // The JSON may either use an AdditionalData key
    // as an index, or not.
    if (helper::calloutUsesAdditionalData(calloutJSON))
    {
        return helper::getCalloutsUsingAD(calloutJSON, systemNames,
                                          additionalData);
    }

    return helper::getCalloutsWithoutAD(calloutJSON, systemNames);
}

} // namespace message
} // namespace pels
} // namespace openpower
