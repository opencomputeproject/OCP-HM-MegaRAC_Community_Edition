#pragma once

#include <nlohmann/json.hpp>

#include <map>
#include <string>
#include <vector>

/** @brief Stores the error to log if errors to monitor is found */
struct targetEntry
{
    std::string errorToLog;
    std::vector<std::string> errorsToMonitor;
};

/** @brief A map of the systemd target to its corresponding targetEntry*/
using TargetErrorData = std::map<std::string, targetEntry>;

using json = nlohmann::json;

extern bool gVerbose;

/** @brief Parse input json files
 *
 * Will return the parsed data in the TargetErrorData object
 *
 * @note This function will throw exceptions for an invalid json file
 * @note See phosphor-target-monitor-default.json for example of json file
 *       format
 *
 * @param[in] filePaths - The file(s) to parse
 *
 * @return Map of target to error log relationships
 */
TargetErrorData parseFiles(const std::vector<std::string>& filePaths);
