/*
 * Copyright 2018 Google Inc.
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
#include <string>
#include <vector>

namespace blobs
{
namespace fs = std::filesystem;

std::vector<std::string> getLibraryList(const std::string& path,
                                        PathMatcher check)
{
    std::vector<std::string> output;

    for (const auto& p : fs::recursive_directory_iterator(path))
    {
        auto ps = p.path().string();

        if (check(ps))
        {
            output.push_back(ps);
        }
    }

    /* Algorithm ends up being two-pass, but expectation is a list under 10. */
    return output;
}

} // namespace blobs
