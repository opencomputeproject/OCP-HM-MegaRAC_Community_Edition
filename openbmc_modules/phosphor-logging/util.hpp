#pragma once

#include <fstream>
#include <optional>
#include <string>

namespace phosphor::logging::util
{

/**
 * @brief Return a value found in the /etc/os-release file
 *
 * @param[in] key - The key name, like "VERSION"
 *
 * @return std::optional<std::string> - The value
 */
std::optional<std::string> getOSReleaseValue(const std::string& key);

} // namespace phosphor::logging::util
