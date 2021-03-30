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

#include "mdrv2.hpp"

#include <systemd/sd-event.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>

int main(void)
{
    sd_event* events = nullptr;
    sd_event_default(&events);
    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();
    sdbusplus::server::manager::manager objManager(
        bus, "/xyz/openbmc_project/inventory");
    bus.attach_event(events, SD_EVENT_PRIORITY_NORMAL);
    bus.request_name("xyz.openbmc_project.Smbios.MDR_V2");

    phosphor::smbios::MDR_V2 mdrV2(bus, mdrV2Path, events);

    while (true)
    {
        int r = sd_event_run(events, (uint64_t)-1);
        if (r < 0)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failure processing request",
                phosphor::logging::entry("errno=0x%X", -r));
            return -1;
        }
    }
    return 0;
}
