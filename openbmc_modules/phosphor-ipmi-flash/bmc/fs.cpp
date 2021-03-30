/*
 * Copyright 2019 Google Inc.
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

#include "fs.hpp"

#include <filesystem>
#include <regex>
#include <string>
#include <vector>

namespace ipmi_flash
{
namespace fs = std::filesystem;

std::vector<std::string> GetJsonList(const std::string& directory)
{
    std::vector<std::string> output;

    for (const auto& p : fs::recursive_directory_iterator(directory))
    {
        auto ps = p.path().string();

        /** TODO: openbmc/phosphor-ipmi-blobs/blob/de8a16e2e8/fs.cpp#L27 is
         * nicer, may be worth finding a way to make this into a util.
         */
        if (std::regex_match(ps, std::regex(".+.json$")))
        {
            output.push_back(ps);
        }
    }

    return output;
}

} // namespace ipmi_flash
