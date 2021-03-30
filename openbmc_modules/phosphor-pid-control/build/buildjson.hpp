#pragma once

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

/**
 * Given json data, validate the minimum.
 * The json data must be valid, and must contain two keys:
 * sensors, and zones.
 *
 * @param[in] data - the json data.
 * @return nothing - throws exceptions on invalid bits.
 */
void validateJson(const json& data);

/**
 * Given a json configuration file, parse it.
 *
 * There must be at least one sensor, and one zone.
 * That one zone must contain at least one PID.
 *
 * @param[in] path - path to the configuration
 * @return the json data.
 */
json parseValidateJson(const std::string& path);
