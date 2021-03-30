/**
 * Copyright © 2018 IBM Corporation
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

const std::string ArgumentParser::true_string = "true";
const std::string ArgumentParser::empty_string = "";

const char* ArgumentParser::optionstr = "s:b:i:?h";
const option ArgumentParser::options[] = {
    {"service-namespaces", required_argument, nullptr, 's'},
    {"service-blacklists", required_argument, nullptr, 'b'},
    {"interface-namespaces", required_argument, nullptr, 'i'},
    {"help", no_argument, nullptr, 'h'},
    {0, 0, 0, 0},
};

ArgumentParser::ArgumentParser(int argc, char** argv)
{
    int option = 0;
    while (-1 !=
           (option = getopt_long(argc, argv, optionstr, options, nullptr)))
    {
        if ((option == '?') || (option == 'h'))
        {
            usage(argv);
            exit(-1);
        }

        auto i = &options[0];
        while ((i->val != option) && (i->val != 0))
            ++i;

        if (i->val)
            arguments[i->name] = (i->has_arg ? optarg : true_string);
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
    std::cerr << "Usage: " << argv[0] << " [options]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "    --help               Print this menu" << std::endl;
    std::cerr << "    --service-namespaces=<services> Space separated list of ";
    std::cerr << "service namespaces to whitelist\n";
    std::cerr << "    --service-blacklists=<services> Space separated list of ";
    std::cerr << "service names to blacklist\n";
    std::cerr << "    --interface-namespaces=<ifaces> Space separated list of ";
    std::cerr << "interface namespaces to whitelist\n";
}
