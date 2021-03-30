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

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>

namespace phosphor
{
namespace gpio
{

using namespace std::string_literals;

const std::string ArgumentParser::trueString = "true"s;
const std::string ArgumentParser::emptyString = ""s;

const char* ArgumentParser::optionStr = "p:k:r:t:?h";
const option ArgumentParser::options[] = {
    {"path", required_argument, nullptr, 'p'},
    {"key", required_argument, nullptr, 'k'},
    {"polarity", required_argument, nullptr, 'r'},
    {"target", required_argument, nullptr, 't'},
    {"continue", no_argument, nullptr, 'c'},
    {"help", no_argument, nullptr, 'h'},
    {0, 0, 0, 0},
};

ArgumentParser::ArgumentParser(int argc, char** argv)
{
    int option = 0;
    while (-1 !=
           (option = getopt_long(argc, argv, optionStr, options, nullptr)))
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
            // optional argument may get nullptr for optarg
            // make it empty string in such case
            auto arg = (optarg == nullptr ? "" : optarg);
            arguments[i->name] = (i->has_arg ? arg : trueString);
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
    std::cerr << "  --help                  Print this menu\n";
    std::cerr << "  --path=<path>           Path of input device."
                 " Ex: /dev/input/event2\n";
    std::cerr << "  --key=<key>             Input GPIO key number\n";
    std::cerr << "  --polarity=<polarity>   Asertion polarity to look for."
                 " This is 0 / 1 \n";
    std::cerr << "  --target=<systemd unit> Systemd unit to be called on GPIO"
                 " state change\n";
    std::cerr << "  [--continue]            Whether or not to continue"
                 " after key pressed\n";
}
} // namespace gpio
} // namespace phosphor
