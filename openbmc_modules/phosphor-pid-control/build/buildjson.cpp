#include "build/buildjson.hpp"

#include "errors/exception.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void validateJson(const json& data)
{
    if (data.count("sensors") != 1)
    {
        throw ConfigurationException(
            "KeyError: 'sensors' not found (or found repeatedly)");
    }

    if (data["sensors"].size() == 0)
    {
        throw ConfigurationException(
            "Invalid Configuration: At least one sensor required");
    }

    if (data.count("zones") != 1)
    {
        throw ConfigurationException(
            "KeyError: 'zones' not found (or found repeatedly)");
    }

    for (const auto& zone : data["zones"])
    {
        if (zone.count("pids") != 1)
        {
            throw ConfigurationException(
                "KeyError: should only have one 'pids' key per zone.");
        }

        if (zone["pids"].size() == 0)
        {
            throw ConfigurationException(
                "Invalid Configuration: must be at least one pid per zone.");
        }
    }
}

json parseValidateJson(const std::string& path)
{
    std::ifstream jsonFile(path);
    if (!jsonFile.is_open())
    {
        throw ConfigurationException("Unable to open json file");
    }

    auto data = json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        throw ConfigurationException("Invalid json - parse failed");
    }

    /* Check the data. */
    validateJson(data);

    return data;
}
