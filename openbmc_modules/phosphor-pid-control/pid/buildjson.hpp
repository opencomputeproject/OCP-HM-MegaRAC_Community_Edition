#pragma once

#include "conf.hpp"

#include <map>
#include <nlohmann/json.hpp>
#include <tuple>

using json = nlohmann::json;

/**
 * Given the json "zones" data, create the map of PIDs and the map of zones.
 *
 * @param[in] data - the json data
 * @return the pidConfig, and the zoneConfig
 */
std::pair<std::map<int64_t, conf::PIDConf>,
          std::map<int64_t, struct conf::ZoneConfig>>
    buildPIDsFromJson(const json& data);
