#include "systemd_target_parser.hpp"

#include <cassert>
#include <fstream>
#include <iostream>

void validateErrorsToMonitor(std::vector<std::string>& errorsToMonitor)
{
    assert(errorsToMonitor.size());

    const std::vector<std::string> validErrorsToMonitor = {
        "default", "timeout", "failed", "dependency"};
    for (const auto& errorToMonitor : errorsToMonitor)
    {
        if (std::find(validErrorsToMonitor.begin(), validErrorsToMonitor.end(),
                      errorToMonitor) == validErrorsToMonitor.end())
        {
            throw std::out_of_range("Found invalid error to monitor");
        }
    }
    // See if default was in the errors to monitor, if so replace with defaults
    auto errorItr =
        std::find(errorsToMonitor.begin(), errorsToMonitor.end(), "default");
    if (errorItr != errorsToMonitor.end())
    {
        // Verify default is the only entry
        if (errorsToMonitor.size() != 1)
        {
            throw std::invalid_argument(
                "default must be only error to monitor");
        }
        // delete "default" and insert defaults
        errorsToMonitor.erase(errorItr);
        errorsToMonitor.push_back("timeout");
        errorsToMonitor.push_back("failed");
        errorsToMonitor.push_back("dependency");
    }
}

TargetErrorData parseFiles(const std::vector<std::string>& filePaths)
{
    TargetErrorData systemdTargetMap;
    for (const auto& jsonFile : filePaths)
    {
        if (gVerbose)
        {
            std::cout << "Parsing input file " << jsonFile << std::endl;
        }
        std::ifstream fileStream(jsonFile);
        auto j = json::parse(fileStream);

        for (auto it = j["targets"].begin(); it != j["targets"].end(); ++it)
        {
            targetEntry entry;
            if (gVerbose)
            {
                std::cout << "target: " << it.key() << " | " << it.value()
                          << std::endl;
            }

            // Be unforgiving on invalid json files. Just throw or allow
            // nlohmann to throw an exception if something is off
            auto errorsToMonitor = it.value().find("errorsToMonitor");
            entry.errorsToMonitor =
                errorsToMonitor->get<std::vector<std::string>>();

            validateErrorsToMonitor(entry.errorsToMonitor);

            auto errorToLog = it.value().find("errorToLog");
            entry.errorToLog = errorToLog->get<std::string>();

            systemdTargetMap[it.key()] = entry;
        }
    }
    return systemdTargetMap;
}
