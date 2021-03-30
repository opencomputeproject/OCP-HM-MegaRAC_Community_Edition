/**
 * Copyright 2017 Google Inc.
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

#include <string>

static constexpr auto external_sensor =
    "/xyz/openbmc_project/extsensors/";                         // type/
static constexpr auto openbmc_sensor = "/xyz/openbmc_project/"; // type/
static constexpr auto sysfs = "/sys/";

IOInterfaceType getWriteInterfaceType(const std::string& path)
{
    if (path.empty() || "None" == path)
    {
        return IOInterfaceType::NONE;
    }

    if (path.find(sysfs) != std::string::npos)
    {
        // A sysfs read sensor.
        return IOInterfaceType::SYSFS;
    }

    if (path.find(openbmc_sensor) != std::string::npos)
    {
        return IOInterfaceType::DBUSACTIVE;
    }

    return IOInterfaceType::UNKNOWN;
}

IOInterfaceType getReadInterfaceType(const std::string& path)
{
    if (path.empty() || "None" == path)
    {
        return IOInterfaceType::NONE;
    }

    if (path.find(external_sensor) != std::string::npos)
    {
        return IOInterfaceType::EXTERNAL;
    }

    if (path.find(openbmc_sensor) != std::string::npos)
    {
        return IOInterfaceType::DBUSPASSIVE;
    }

    if (path.find(sysfs) != std::string::npos)
    {
        return IOInterfaceType::SYSFS;
    }

    return IOInterfaceType::UNKNOWN;
}
