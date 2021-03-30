/**
 * Copyright © 2017 IBM Corporation
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

#include "util.hpp"

#include <algorithm>
#include <map>
#include <string>

using namespace std::literals::string_literals;

template <typename T>
struct BusMeetsMSL
{
    std::string path;

    BusMeetsMSL(const std::string& p) : path(p)
    {
    }

    auto operator()(const T& arg)
    {
        // Query each service hosting
        // xyz.openbmc_project.Inventory.Decorator.MeetsMinimumShipLevel.

        const auto& busName = arg.first;
        return util::sdbusplus::getProperty<bool>(
            busName, path,
            "xyz.openbmc_project.Inventory."
            "Decorator.MeetsMinimumShipLevel"s,
            "MeetsMinimumShipLevel"s);
    }
};

template <typename T>
struct PathMeetsMSL
{
    auto operator()(const T& arg)
    {
        // A given path in the mapper response is composed of
        // a map of services/interfaces.  Validate each service
        // that hosts the MSL interface meets the MSL.

        const auto& path = arg.first;
        return std::all_of(
            arg.second.begin(), arg.second.end(),
            BusMeetsMSL<typename decltype(arg.second)::value_type>(path));
    }
};

int main(void)
{
    auto mslVerificationRequired = util::sdbusplus::getProperty<bool>(
        "/xyz/openbmc_project/control/minimum_ship_level_required"s,
        "xyz.openbmc_project.Control.MinimumShipLevel"s,
        "MinimumShipLevelRequired"s);

    if (!mslVerificationRequired)
    {
        return 0;
    }

    // Obtain references to all objects hosting
    // xyz.openbmc_project.Inventory.Decorator.MeetsMinimumShipLevel
    // with a mapper subtree query.  For each object, validate that
    // the minimum ship level has been met.

    using SubTreeType =
        std::map<std::string, std::map<std::string, std::vector<std::string>>>;

    auto subtree = util::sdbusplus::callMethodAndRead<SubTreeType>(
        "xyz.openbmc_project.ObjectMapper"s,
        "/xyz/openbmc_project/object_mapper"s,
        "xyz.openbmc_project.ObjectMapper"s, "GetSubTree"s, "/"s, 0,
        std::vector<std::string>{"xyz.openbmc_project.Inventory"
                                 ".Decorator.MeetsMinimumShipLevel"s});

    auto result = std::all_of(subtree.begin(), subtree.end(),
                              PathMeetsMSL<SubTreeType::value_type>());

    if (!result)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "The physical system configuration does not "
            "satisfy the minimum ship level.");

        return 1;
    }

    return 0;
}
