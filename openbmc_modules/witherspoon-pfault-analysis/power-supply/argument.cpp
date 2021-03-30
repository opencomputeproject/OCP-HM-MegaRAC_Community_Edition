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

namespace witherspoon
{
namespace power
{

ArgumentParser::ArgumentParser(int argc, char** argv)
{
    auto option = 0;
    while (-1 != (option = getopt_long(argc, argv, optionStr, options, NULL)))
    {
        if ((option == '?') || (option == 'h'))
        {
            usage(argv);
            exit(-1);
        }

        auto i = &options[0];
        while ((i->val != option) && (i->val != 0))
        {
            ++i;
        }

        if (i->val)
        {
            arguments[i->name] = (i->has_arg ? optarg : trueString);
        }
    }
}

const std::string& ArgumentParser::operator[](const std::string& opt)
{
    auto i = arguments.find(opt);
    if (i == arguments.end())
    {
        return emptyString;
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
    std::cerr << "    --help                              Print this menu\n";
    std::cerr << "    --path=<objpath>                    Path to location to"
                 " monitor\n";
    std::cerr << "    --instance=<instance number>        Instance number for"
                 " this power supply\n";
    std::cerr << "    --inventory=<inventory path>        Inventory path for"
                 " this power supply\n";
    std::cerr << "    --num-history-records=<num records> Number of input"
                 " power history records to provide on D-Bus\n";
    std::cerr << "    --sync-gpio-path=<path>             GPIO chip device"
                 " for the GPIO that performs the sync function\n";
    std::cerr << "    --sync-gpio-num=<path>              GPIO number for the"
                 " GPIO that performs the sync function\n";
    std::cerr << std::flush;
}

const option ArgumentParser::options[] = {
    {"path", required_argument, NULL, 'p'},
    {"instance", required_argument, NULL, 'n'},
    {"inventory", required_argument, NULL, 'i'},
    {"num-history-records", required_argument, NULL, 'r'},
    {"sync-gpio-path", required_argument, NULL, 'a'},
    {"sync-gpio-num", required_argument, NULL, 'u'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0},
};

const char* ArgumentParser::optionStr = "p:n:i:r:a:u:h";

const std::string ArgumentParser::trueString = "true";
const std::string ArgumentParser::emptyString = "";

} // namespace power
} // namespace witherspoon
