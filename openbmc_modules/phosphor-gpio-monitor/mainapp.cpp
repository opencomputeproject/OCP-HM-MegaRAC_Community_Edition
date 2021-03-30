/**
 * Copyright © 2016 IBM Corporation
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

#include "argument.hpp"
#include "monitor.hpp"

#include <systemd/sd-event.h>

#include <iostream>
#include <phosphor-logging/log.hpp>
#include <string>

using namespace phosphor::logging;
static void exitWithError(const char* err, char** argv)
{
    phosphor::gpio::ArgumentParser::usage(argv);
    std::cerr << "ERROR: " << err << "\n";
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    // Read arguments.
    auto options = phosphor::gpio::ArgumentParser(argc, argv);

    // Parse out path argument.
    auto path = (options)["path"];
    if (path == phosphor::gpio::ArgumentParser::emptyString)
    {
        exitWithError("path not specified.", argv);
    }

    // Parse out key number that we are interested in
    // Its integer mapping to the GPIO key configured by the kernel
    auto key = (options)["key"];
    if (key == phosphor::gpio::ArgumentParser::emptyString)
    {
        exitWithError("Key not specified.", argv);
    }

    // Parse out assertion polarity interested in
    // Its either 1 or 0 for press / release
    auto polarity = (options)["polarity"];
    if (polarity == phosphor::gpio::ArgumentParser::emptyString)
    {
        exitWithError("Polarity not specified.", argv);
    }

    // Parse out target argument. It is fine if the caller does not
    // pass this if they are not interested in calling into any target
    // on meeting a condition.
    auto target = (options)["target"];

    bool continueRun =
        (options["continue"] == phosphor::gpio::ArgumentParser::trueString);

    sd_event* event = nullptr;
    auto r = sd_event_default(&event);
    if (r < 0)
    {
        log<level::ERR>("Error creating a default sd_event handler");
        return r;
    }
    phosphor::gpio::EventPtr eventP{event};
    event = nullptr;

    // Create a monitor object and let it do all the rest
    phosphor::gpio::Monitor monitor(path, std::stoi(key), std::stoi(polarity),
                                    target, eventP, continueRun);

    // Wait for client requests until this application has processed
    // at least one expected GPIO state change
    while (!monitor.completed())
    {
        // -1 denotes wait for ever
        r = sd_event_run(eventP.get(), (uint64_t)-1);
        if (r < 0)
        {
            log<level::ERR>("Failure in processing request",
                            entry("ERROR=%s", strerror(-r)));
            break;
        }
    }

    return 0;
}
