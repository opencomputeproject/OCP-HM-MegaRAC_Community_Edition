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

#include "handler.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace ethstats
{

bool EthStats::validIfNameAndField(const std::string& path) const
{
    namespace fs = std::filesystem;

    // TODO: Transition to using the netlink api.
    std::error_code ec;
    if (!fs::exists(path, ec))
    {
        std::fprintf(stderr, "Path: '%s' doesn't exist. ec(%d, %s)\n",
                     path.c_str(), ec.value(), ec.message().c_str());
        return false;
    }

    return true;
}

std::uint64_t EthStats::readStatistic(const std::string& path) const
{
    // We know the file exists, so just read it.
    std::uint64_t value = 0;
    std::ifstream ifs;

    ifs.open(path);
    ifs >> value;

    return value;
}

} // namespace ethstats
