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

#include "argument.hpp"
#include "evdevpp/evdev.hpp"
// TODO https://github.com/openbmc/phosphor-fan-presence/issues/22
// #include "sdevent/event.hpp"
// #include "sdevent/io.hpp"
#include "utility.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>

namespace phosphor
{
namespace fan
{
namespace util
{

ArgumentParser::ArgumentParser(int argc, char** argv)
{
    auto option = 0;
    while (-1 != (option = getopt_long(argc, argv, optionstr, options, NULL)))
    {
        if ((option == '?') || (option == 'h'))
        {
            usage(argv);
            exit(1);
        }

        auto i = &options[0];
        while ((i->val != option) && (i->val != 0))
        {
            ++i;
        }

        if (i->val)
        {
            arguments[i->name] = (i->has_arg ? optarg : true_string);
        }
    }
}

const std::string& ArgumentParser::operator[](const std::string& opt)
{
    auto i = arguments.find(opt);
    if (i == arguments.end())
    {
        return empty_string;
    }
    else
    {
        return i->second;
    }
}

void ArgumentParser::usage(char** argv)
{
    std::cerr << "Usage: " << argv[0] << " [options]\n";
    std::cerr << "Options:\n";
    std::cerr << "    --path               evdev devpath\n";
    std::cerr << "    --type               evdev type\n";
    std::cerr << "    --code               evdev code\n";
    std::cerr << std::flush;
}

const option ArgumentParser::options[] = {
    {"path", required_argument, NULL, 'p'},
    {"type", required_argument, NULL, 't'},
    {"code", required_argument, NULL, 'c'},
    {0, 0, 0, 0},
};

const char* ArgumentParser::optionstr = "p:t:c:";

const std::string ArgumentParser::true_string = "true";
const std::string ArgumentParser::empty_string = "";

static void exit_with_error(const char* err, char** argv)
{
    ArgumentParser::usage(argv);
    std::cerr << "\n";
    std::cerr << "ERROR: " << err << "\n";
    exit(1);
}

} // namespace util
} // namespace fan
} // namespace phosphor

int main(int argc, char* argv[])
{
    using namespace phosphor::fan::util;

    auto options = std::make_unique<ArgumentParser>(argc, argv);
    auto path = (*options)["path"];
    auto stype = (*options)["type"];
    auto scode = (*options)["code"];
    unsigned int type = EV_KEY;

    if (path == ArgumentParser::empty_string)
    {
        exit_with_error("Path not specified or invalid.", argv);
    }
    if (stype != ArgumentParser::empty_string)
    {
        type = stoul(stype);
    }

    if (scode == ArgumentParser::empty_string)
    {
        exit_with_error("Keycode not specified or invalid.", argv);
    }
    options.reset();

    // TODO https://github.com/openbmc/phosphor-fan-presence/issues/22
    // auto loop = sdevent::event::newDefault();
    phosphor::fan::util::FileDescriptor fd(
        open(path.c_str(), O_RDONLY | O_NONBLOCK));
    auto evdev = evdevpp::evdev::newFromFD(fd());
    // sdevent::event::io::IO callback(loop, fd(), [&evdev](auto& s) {
    //     unsigned int type, code, value;
    //     std::tie(type, code, value) = evdev.next();
    //     std::cout << "type: " << libevdev_event_type_get_name(type)
    //               << " code: " << libevdev_event_code_get_name(type, code)
    //               << " value: " << value << "\n";
    // });

    auto value = evdev.fetch(type, stoul(scode));
    std::cout << "type: " << libevdev_event_type_get_name(type)
              << " code: " << libevdev_event_code_get_name(type, stoul(scode))
              << " value: " << value << "\n";

    // loop.loop();

    return 0;
}
