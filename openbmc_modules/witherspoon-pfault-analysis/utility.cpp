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
#include "utility.hpp"

namespace witherspoon
{
namespace power
{
namespace util
{

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

using namespace phosphor::logging;

std::string getService(const std::string& path, const std::string& interface,
                       sdbusplus::bus::bus& bus)
{
    auto method = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetObject");

    method.append(path);
    method.append(std::vector<std::string>({interface}));

    auto reply = bus.call(method);

    std::map<std::string, std::vector<std::string>> response;
    reply.read(response);

    if (response.empty())
    {
        log<level::ERR>("Error in mapper response for getting service name",
                        entry("PATH=%s", path.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
        return std::string{};
    }

    return response.begin()->first;
}

} // namespace util
} // namespace power
} // namespace witherspoon
