/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "dbus/dbuswrite.hpp"

#include <iostream>
#include <memory>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <string>
#include <variant>

constexpr const char* pwmInterface = "xyz.openbmc_project.Control.FanPwm";

using namespace phosphor::logging;

std::unique_ptr<WriteInterface>
    DbusWritePercent::createDbusWrite(const std::string& path, int64_t min,
                                      int64_t max, DbusHelperInterface& helper)
{
    auto tempBus = sdbusplus::bus::new_system();
    std::string connectionName;

    try
    {
        connectionName = helper.getService(tempBus, pwmInterface, path);
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }

    return std::make_unique<DbusWritePercent>(path, min, max, connectionName);
}

void DbusWritePercent::write(double value)
{
    double minimum = getMin();
    double maximum = getMax();

    double range = maximum - minimum;
    double offset = range * value;
    double ovalue = offset + minimum;

    if (oldValue == static_cast<int64_t>(ovalue))
    {
        return;
    }
    auto writeBus = sdbusplus::bus::new_default();
    auto mesg =
        writeBus.new_method_call(connectionName.c_str(), path.c_str(),
                                 "org.freedesktop.DBus.Properties", "Set");
    mesg.append(pwmInterface, "Target",
                std::variant<uint64_t>(static_cast<uint64_t>(ovalue)));

    try
    {
        // TODO: if we don't use the reply, call_noreply()
        auto resp = writeBus.call(mesg);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("Dbus Call Failure", entry("PATH=%s", path.c_str()),
                        entry("WHAT=%s", ex.what()));
    }

    oldValue = static_cast<int64_t>(ovalue);
    return;
}

std::unique_ptr<WriteInterface>
    DbusWrite::createDbusWrite(const std::string& path, int64_t min,
                               int64_t max, DbusHelperInterface& helper)
{
    auto tempBus = sdbusplus::bus::new_system();
    std::string connectionName;

    try
    {
        connectionName = helper.getService(tempBus, pwmInterface, path);
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }

    return std::make_unique<DbusWrite>(path, min, max, connectionName);
}

void DbusWrite::write(double value)
{
    if (oldValue == static_cast<int64_t>(value))
    {
        return;
    }
    auto writeBus = sdbusplus::bus::new_default();
    auto mesg =
        writeBus.new_method_call(connectionName.c_str(), path.c_str(),
                                 "org.freedesktop.DBus.Properties", "Set");
    mesg.append(pwmInterface, "Target",
                std::variant<uint64_t>(static_cast<uint64_t>(value)));

    try
    {
        // TODO: consider call_noreplly
        auto resp = writeBus.call(mesg);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("Dbus Call Failure", entry("PATH=%s", path.c_str()),
                        entry("WHAT=%s", ex.what()));
    }

    oldValue = static_cast<int64_t>(value);
    return;
}
