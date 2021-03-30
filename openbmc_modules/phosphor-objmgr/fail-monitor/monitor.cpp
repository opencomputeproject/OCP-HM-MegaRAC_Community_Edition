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
#include "monitor.hpp"

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace unit
{
namespace failure
{

using namespace phosphor::logging;

constexpr auto FAILED_STATE = "failed";
constexpr auto START_METHOD = "StartUnit";
constexpr auto STOP_METHOD = "StopUnit";

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr auto SYSTEMD_PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto SYSTEMD_UNIT_INTERFACE = "org.freedesktop.systemd1.Unit";

void Monitor::analyze()
{
    if (inFailedState(std::move(getSourceUnitPath())))
    {
        runTargetAction();
    }
}

bool Monitor::inFailedState(const std::string&& path)
{
    std::variant<std::string> property;

    auto method = bus.new_method_call(SYSTEMD_SERVICE, path.c_str(),
                                      SYSTEMD_PROPERTY_INTERFACE, "Get");

    method.append(SYSTEMD_UNIT_INTERFACE, "ActiveState");

    auto reply = bus.call(method);
    if (reply.is_method_error())
    {
        log<level::ERR>("Failed reading ActiveState DBus property",
                        entry("UNIT=%s", source.c_str()));
        // TODO openbmc/openbmc#851 - Once available, throw returned error
        throw std::runtime_error("Failed reading ActiveState DBus property");
    }

    reply.read(property);

    auto value = std::get<std::string>(property);
    return (value == FAILED_STATE);
}

std::string Monitor::getSourceUnitPath()
{
    sdbusplus::message::object_path path;

    auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                      SYSTEMD_INTERFACE, "GetUnit");
    method.append(source);
    auto reply = bus.call(method);

    if (reply.is_method_error())
    {
        log<level::ERR>("Failed GetUnit DBus method call",
                        entry("UNIT=%s", source.c_str()));
        // TODO openbmc/openbmc#851 - Once available, throw returned error
        throw std::runtime_error("Failed GetUnit DBus method call");
    }

    reply.read(path);

    return static_cast<std::string>(path);
}

void Monitor::runTargetAction()
{
    // Start or stop the target unit
    auto methodCall = (action == Action::start) ? START_METHOD : STOP_METHOD;

    log<level::INFO>("The source unit is in failed state, "
                     "running target action",
                     entry("SOURCE=%s", source.c_str()),
                     entry("TARGET=%s", target.c_str()),
                     entry("ACTION=%s", methodCall));

    auto method = this->bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                            SYSTEMD_INTERFACE, methodCall);
    method.append(target);
    method.append("replace");

    auto reply = bus.call(method);

    if (reply.is_method_error())
    {
        log<level::ERR>("Failed to run action on the target unit",
                        entry("UNIT=%s", target.c_str()));
        // TODO openbmc/openbmc#851 - Once available, throw returned error
        throw std::runtime_error("Failed to run action on the target unit");
    }
}
} // namespace failure
} // namespace unit
} // namespace phosphor
