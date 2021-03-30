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

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>

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

// TODO openbmc/phosphor-fan-presence#6
//      gpio parameter need something to indicate 0=water & air, 1=air?
void ArgumentParser::usage(char** argv)
{
    std::cerr << "Usage: " << argv[0] << " [options]\n";
    std::cerr << "Options:\n";
    std::cerr << "    --help               print this menu\n";
    std::cerr << "    --air                Force 'AirCooled' property to be set"
                 " to true.\n";
    std::cerr << "    --water              Force 'WaterCooled' property to be "
                 "set to true.\n";
    std::cerr << "    --dev=<pin>          Device to read for GPIO pin state to"
                 " determine 'WaterCooled' (true) and 'AirCooled' (false)\n";
    std::cerr << "    --event=<keycode>    Keycode for pin to read\n";
    std::cerr
        << "    --path=<objpath>     *Required* object path under inventory "
           "to have CoolingType updated\n";
    std::cerr
        << "\nThe --air / --water options may be given in addition to "
           "--gpio, in which case both their setting and the GPIO will take "
           "effect.\n";
    std::cerr << std::flush;
}

const option ArgumentParser::options[] = {
    {"path", required_argument, NULL, 'p'},
    {"dev", required_argument, NULL, 'd'},
    {"event", required_argument, NULL, 'e'},
    {"air", no_argument, NULL, 'a'},
    {"water", no_argument, NULL, 'w'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0},
};

const char* ArgumentParser::optionstr = "p:d:e:aw?h";

const std::string ArgumentParser::true_string = "true";
const std::string ArgumentParser::empty_string = "";

} // namespace util
} // namespace fan
} // namespace phosphor
// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
