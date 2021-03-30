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
#include "resolve_errors.hpp"

#include "sdbusplus.hpp"

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

constexpr auto LOGGING_IFACE = "xyz.openbmc_project.Logging.Entry";
constexpr auto PROPERTY_IFACE = "org.freedesktop.DBus.Properties";
constexpr auto ASSOCIATION_IFACE = "xyz.openbmc_project.Association";
constexpr auto ENDPOINTS_PROPERTY = "endpoints";
constexpr auto RESOLVED_PROPERTY = "Resolved";

using namespace phosphor::logging;
using EndpointList = std::vector<std::string>;
using EndpointsProperty = std::variant<EndpointList>;

void ResolveCallout::operator()(Context ctx)
{
    // Resolve all errors for this callout:
    // 1) Read the 'endpoints' property for the callout/fault object
    //
    // 2) Follow each endpoint to its log entry
    //
    // 3) Set the Resolved property to true on the entry

    try
    {
        auto path = callout + "/fault";
        auto busName = SDBusPlus::getBusName(path, ASSOCIATION_IFACE);

        if (busName.empty())
        {
            // Just means there are no error logs with this callout
            return;
        }

        auto endpoints = SDBusPlus::callMethodAndRead<EndpointsProperty>(
            busName, path, PROPERTY_IFACE, "Get", ASSOCIATION_IFACE,
            ENDPOINTS_PROPERTY);

        const auto& logEntries = std::get<EndpointList>(endpoints);

        // Resolve each log entry
        for (const auto& logEntry : logEntries)
        {
            resolve(logEntry);
        }
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed getting callout fault associations",
                        entry("CALLOUT=%s", callout.c_str()),
                        entry("ERROR=%s", e.what()));
    }
}

void ResolveCallout::resolve(const std::string& logEntry)
{
    try
    {
        static std::string busName;
        if (busName.empty())
        {
            busName = SDBusPlus::getBusName(logEntry, LOGGING_IFACE);
            if (busName.empty())
            {
                return;
            }
        }

        std::variant<bool> resolved = true;

        auto response =
            SDBusPlus::callMethod(busName, logEntry, PROPERTY_IFACE, "Set",
                                  LOGGING_IFACE, RESOLVED_PROPERTY, resolved);

        if (response.is_method_error())
        {
            log<level::ERR>(
                "Failed to set Resolved property on an error log entry",
                entry("ENTRY=%s", logEntry.c_str()));
        }
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Unable to resolve error log entry",
                        entry("ENTRY=%s", logEntry.c_str()),
                        entry("ERROR=%s", e.what()));
    }
}

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
