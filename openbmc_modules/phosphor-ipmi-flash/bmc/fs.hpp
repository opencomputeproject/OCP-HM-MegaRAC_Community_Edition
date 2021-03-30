#pragma once

#include <string>
#include <vector>

namespace ipmi_flash
{

/**
 * Given a directory, return the list of json file paths (full paths).
 *
 * @param[in] directory - the full path to the directory to search.
 * @return list of full paths to json files found in that directory.
 */
std::vector<std::string> GetJsonList(const std::string& directory);

} // namespace ipmi_flash
