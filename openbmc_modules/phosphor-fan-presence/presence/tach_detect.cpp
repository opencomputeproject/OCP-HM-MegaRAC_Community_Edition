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
#ifdef PRESENCE_USE_JSON
#include "json_config.hpp"
#else
#include "generated.hpp"
#endif
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <stdplus/signal.hpp>

#include <functional>

int main(void)
{
    using namespace phosphor::fan;

    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

#ifdef PRESENCE_USE_JSON
    // Use json file for presence config
    presence::JsonConfig config(bus);
    for (auto& p : presence::JsonConfig::get())
    {
        p->monitor();
    }

    stdplus::signal::block(SIGHUP);
    sdeventplus::source::Signal signal(
        event, SIGHUP,
        std::bind(&presence::JsonConfig::sighupHandler, &config,
                  std::placeholders::_1, std::placeholders::_2));
#else
    for (auto& p : presence::ConfigPolicy::get())
    {
        p->monitor();
    }
#endif

    return event.loop();
}
