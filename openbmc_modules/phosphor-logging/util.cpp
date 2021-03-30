/**
 * Copyright © 2020 IBM Corporation
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
#include "config.h"

#include "util.hpp"

namespace phosphor::logging::util
{

std::optional<std::string> getOSReleaseValue(const std::string& key)
{
    std::ifstream versionFile{BMC_VERSION_FILE};
    std::string line;
    std::string keyPattern{key + '='};

    while (std::getline(versionFile, line))
    {
        if (line.substr(0, keyPattern.size()).find(keyPattern) !=
            std::string::npos)
        {
            // If the value isn't surrounded by quotes, then pos will be
            // npos + 1 = 0, and the 2nd arg to substr() will be npos
            // which means get the rest of the string.
            auto value = line.substr(keyPattern.size());
            std::size_t pos = value.find_first_of('"') + 1;
            return value.substr(pos, value.find_last_of('"') - pos);
        }
    }

    return std::nullopt;
}

} // namespace phosphor::logging::util
