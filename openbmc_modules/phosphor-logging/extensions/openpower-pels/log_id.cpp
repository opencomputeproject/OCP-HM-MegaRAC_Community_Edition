/**
 * Copyright © 2019 IBM Corporation
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
#include "log_id.hpp"

#include "paths.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <phosphor-logging/log.hpp>

namespace openpower
{
namespace pels
{

namespace fs = std::filesystem;
using namespace phosphor::logging;

constexpr uint32_t startingLogID = 1;
constexpr uint32_t bmcLogIDPrefix = 0x50000000;

namespace detail
{

uint32_t addLogIDPrefix(uint32_t id)
{
    // If redundant BMCs are ever a thing, may need a different prefix.
    return (id & 0x00FFFFFF) | bmcLogIDPrefix;
}

uint32_t getTimeBasedLogID()
{
    using namespace std::chrono;

    // Use 3 bytes of the nanosecond count since the epoch.
    uint32_t id =
        duration_cast<nanoseconds>(system_clock::now().time_since_epoch())
            .count();

    return addLogIDPrefix(id);
}

} // namespace detail

uint32_t generatePELID()
{
    // Note: there isn't a need to be thread safe.

    static std::string idFilename;
    if (idFilename.empty())
    {
        idFilename = getPELIDFile();
    }

    uint32_t id = 0;

    if (!fs::exists(idFilename))
    {
        auto path = fs::path(idFilename).parent_path();
        if (!fs::exists(path))
        {
            fs::create_directories(path);
        }

        id = startingLogID;
    }
    else
    {
        std::ifstream idFile{idFilename};
        idFile >> id;
        if (idFile.fail())
        {
            // Just make up an ID
            log<level::ERR>("Unable to read PEL ID File!");
            return detail::getTimeBasedLogID();
        }
    }

    // Wrapping shouldn't be a problem, but check anyway
    if (id == 0x00FFFFFF)
    {
        id = startingLogID;
    }

    std::ofstream idFile{idFilename};
    idFile << (id + 1);
    if (idFile.fail())
    {
        // Just make up an ID so we don't reuse one next time
        log<level::ERR>("Unable to write PEL ID File!");
        return detail::getTimeBasedLogID();
    }

    return detail::addLogIDPrefix(id);
}

} // namespace pels
} // namespace openpower
