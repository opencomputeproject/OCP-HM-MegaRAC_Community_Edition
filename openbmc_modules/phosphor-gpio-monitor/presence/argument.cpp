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
#include <iostream>
#include <iterator>

namespace phosphor
{
namespace gpio
{

using namespace std::string_literals;

const std::string ArgumentParser::trueString = "true"s;
const std::string ArgumentParser::emptyString = ""s;

const char* ArgumentParser::optionStr = "p:k:n:i:d:e:?h";
const option ArgumentParser::options[] = {
    {"path", required_argument, nullptr, 'p'},
    {"key", required_argument, nullptr, 'k'},
    {"name", required_argument, nullptr, 'n'},
    {"inventory", required_argument, nullptr, 'i'},
    {"drivers", required_argument, nullptr, 'd'},
    {"extra-ifaces", required_argument, nullptr, 'e'},
    {"help", no_argument, nullptr, 'h'},
    {0, 0, 0, 0},
};

ArgumentParser::ArgumentParser(int argc, char** argv)
{
    auto option = 0;
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
    std::cerr << "  --help                  Print this menu\n";
    std::cerr << "  --inventory=<inventory> Object path under inventory"
                 " that will be created\n";
    std::cerr << "  --path=<path>           Path of device to read for GPIO pin"
                 " state to determine presence of inventory item\n";
    std::cerr << "  --key=<key>             Input GPIO key number\n";
    std::cerr << "  --name=<name>           Pretty name of the inventory"
                 " item\n";
    std::cerr << "  --drivers=<drivers>     List of drivers to bind when card"
                 " is added and unbind when card is removed\n";
    std::cerr << "                          Format is a space separated list"
                 " of path,device pairs.  For example:\n";
    std::cerr << "                          "
                 "/sys/bus/i2c/drivers/some-driver,3-0068\n";
    std::cerr << "  --extra-ifaces=<ifaces> List of interfaces to associate to"
                 " inventory item\n";
    std::cerr << "                          Format is a comma separated list"
                 " of interfaces.         For example:\n";
    std::cerr << "                          "
                 "/xyz/openbmc_project/.../1,/xyz/openbmc_project/.../2\n";
    std::cerr << std::flush;
    exit(-1);
}
} // namespace gpio
} // namespace phosphor
