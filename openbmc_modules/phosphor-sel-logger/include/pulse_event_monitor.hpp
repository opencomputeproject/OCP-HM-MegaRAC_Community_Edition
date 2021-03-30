/*
// Copyright (c) 2019 Intel Corporation
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

#pragma once
#include <sdbusplus/asio/object_server.hpp>
#include <sel_logger.hpp>
#include <sensorutils.hpp>

inline static sdbusplus::bus::match::match
    startPulseEventMonitor(std::shared_ptr<sdbusplus::asio::connection> conn)
{
    auto pulseEventMatcherCallback = [](sdbusplus::message::message& msg) {
        std::string thresholdInterface;
        boost::container::flat_map<std::string, std::variant<std::string>>
            propertiesChanged;
        msg.read(thresholdInterface, propertiesChanged);

        if (propertiesChanged.empty())
        {
            return;
        }

        std::string event = propertiesChanged.begin()->first;

        auto variant =
            std::get_if<std::string>(&propertiesChanged.begin()->second);

        if (event.empty() || nullptr == variant)
        {
            return;
        }

        if (event == "CurrentHostState")
        {
            if (*variant == "xyz.openbmc_project.State.Host.HostState.Off")
            {
                std::string message("Host system DC power is off");
                std::string redfishMsgId("OpenBMC.0.1.DCPowerOff");

                sd_journal_send("MESSAGE=%s", message.c_str(),
                                "REDFISH_MESSAGE_ID=%s", redfishMsgId.c_str(),
                                NULL);
            }
            else if (*variant ==
                     "xyz.openbmc_project.State.Host.HostState.Running")
            {
                std::string message("Host system DC power is on");
                std::string redfishMsgId("OpenBMC.0.1.DCPowerOn");

                sd_journal_send("MESSAGE=%s", message.c_str(),
                                "REDFISH_MESSAGE_ID=%s", redfishMsgId.c_str(),
                                NULL);
            }
        }
    };

    sdbusplus::bus::match::match pulseEventMatcher(
        static_cast<sdbusplus::bus::bus&>(*conn),
        "type='signal',interface='org.freedesktop.DBus.Properties',member='"
        "PropertiesChanged',arg0namespace='xyz.openbmc_project.State.Host'",
        std::move(pulseEventMatcherCallback));

    return pulseEventMatcher;
}