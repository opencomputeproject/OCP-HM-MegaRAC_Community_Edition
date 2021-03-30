#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

using std::uint32_t;
using json = nlohmann::json;

namespace conf
{

struct BinaryBlobConfig
{
    std::string blobBaseId;  // Required
    std::string sysFilePath; // Required
    uint32_t offsetBytes;    // Optional
    uint32_t maxSizeBytes;   // Optional
};

/**
 * @brief Parse parameters from a config json
 * @param j: input json object
 * @param config: output BinaryBlobConfig
 * @throws: exception if config doesn't have required fields
 */
static inline void parseFromConfigFile(const json& j, BinaryBlobConfig& config)
{
    j.at("blobBaseId").get_to(config.blobBaseId);
    j.at("sysFilePath").get_to(config.sysFilePath);
    config.offsetBytes = j.value("offsetBytes", 0);
    config.maxSizeBytes = j.value("maxSizeBytes", 0);
}

} // namespace conf
