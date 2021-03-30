#include "config.h"

#include "ledlayout.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

using Json = nlohmann::json;
using LedAction = std::set<phosphor::led::Layout::LedAction>;
using LedMap = std::map<std::string, LedAction>;

// Priority for a particular LED needs to stay SAME across all groups
// phosphor::led::Layout::Action can only be one of `Blink` and `On`
using PriorityMap = std::map<std::string, phosphor::led::Layout::Action>;

/** @brief Parse LED JSON file and output Json object
 *
 *  @param[in] path - path of LED JSON file
 *
 *  @return const Json - Json object
 */
const Json readJson(const fs::path& path)
{
    using namespace phosphor::logging;

    if (!fs::exists(path) || fs::is_empty(path))
    {
        log<level::ERR>("Incorrect File Path or empty file",
                        entry("FILE_PATH=%s", path.c_str()));
        throw std::runtime_error("Incorrect File Path or empty file");
    }

    try
    {
        std::ifstream jsonFile(path);
        return Json::parse(jsonFile);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed to parse config file",
                        entry("ERROR=%s", e.what()),
                        entry("FILE_PATH=%s", path.c_str()));
        throw std::runtime_error("Failed to parse config file");
    }
}

/** @brief Returns action enum based on string
 *
 *  @param[in] action - action string
 *
 *  @return Action - action enum (On/Blink)
 */
phosphor::led::Layout::Action getAction(const std::string& action)
{
    assert(action == "On" || action == "Blink");

    return action == "Blink" ? phosphor::led::Layout::Blink
                             : phosphor::led::Layout::On;
}

/** @brief Validate the Priority of an LED is same across ALL groups
 *
 *  @param[in] name - led name member of each group
 *  @param[in] priority - member priority of each group
 *  @param[out] priorityMap - std::map, key:name, value:priority
 *
 *  @return
 */
void validatePriority(const std::string& name,
                      const phosphor::led::Layout::Action& priority,
                      PriorityMap& priorityMap)
{
    using namespace phosphor::logging;

    auto iter = priorityMap.find(name);
    if (iter == priorityMap.end())
    {
        priorityMap.emplace(name, priority);
        return;
    }

    if (iter->second != priority)
    {
        log<level::ERR>("Priority of LED is not same across all",
                        entry("Name=%s", name.c_str()),
                        entry(" Old Priority=%d", iter->second),
                        entry(" New priority=%d", priority));

        throw std::runtime_error(
            "Priority of at least one LED is not same across groups");
    }
}

/** @brief Load JSON config and return led map
 *
 *  @return LedMap - Generated an std::map of LedAction
 */
const LedMap loadJsonConfig(const fs::path& path)
{
    LedMap ledMap{};
    PriorityMap priorityMap{};

    // define the default JSON as empty
    const Json empty{};
    auto json = readJson(path);
    auto leds = json.value("leds", empty);

    for (const auto& entry : leds)
    {
        fs::path tmpPath(std::string{OBJPATH});
        tmpPath /= entry.value("group", "");
        auto objpath = tmpPath.string();
        auto members = entry.value("members", empty);

        LedAction ledActions{};
        for (const auto& member : members)
        {
            auto name = member.value("Name", "");
            auto action = getAction(member.value("Action", ""));
            uint8_t dutyOn = member.value("DutyOn", 50);
            uint16_t period = member.value("Period", 0);

            // Since only have Blink/On and default priority is Blink
            auto priority = getAction(member.value("Priority", "Blink"));

            // Same LEDs can be part of multiple groups. However, their
            // priorities across groups need to match.
            validatePriority(name, priority, priorityMap);

            phosphor::led::Layout::LedAction ledAction{name, action, dutyOn,
                                                       period, priority};
            ledActions.emplace(ledAction);
        }

        // Generated an std::map of LedGroupNames to std::set of LEDs
        // containing the name and properties.
        ledMap.emplace(objpath, ledActions);
    }

    return ledMap;
}
