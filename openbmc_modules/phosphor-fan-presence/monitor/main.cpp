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
#include "config.h"

#include "argument.hpp"
#include "fan.hpp"
#include "system.hpp"
#include "trust_manager.hpp"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <stdplus/signal.hpp>

using namespace phosphor::fan::monitor;

int main(int argc, char* argv[])
{
    auto event = sdeventplus::Event::get_default();
    auto bus = sdbusplus::bus::new_default();
    phosphor::fan::util::ArgumentParser args(argc, argv);

    if (argc != 2)
    {
        args.usage(argv);
        return 1;
    }

    Mode mode;
    if (args["init"] == "true")
    {
        mode = Mode::init;
    }
    else if (args["monitor"] == "true")
    {
        mode = Mode::monitor;
    }
    else
    {
        args.usage(argv);
        return 1;
    }

    // Attach the event object to the bus object so we can
    // handle both sd_events (for the timers) and dbus signals.
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    System system(mode, bus, event);

#ifdef MONITOR_USE_JSON
    // Enable SIGHUP handling to reload JSON config
    stdplus::signal::block(SIGHUP);
    sdeventplus::source::Signal signal(event, SIGHUP,
                                       std::bind(&System::sighupHandler,
                                                 &system, std::placeholders::_1,
                                                 std::placeholders::_2));
#endif

    if (mode == Mode::init)
    {
        // Fans were initialized to be functional, exit
        return 0;
    }

    return event.loop();
}
