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
#include <iostream>
#include <iterator>

namespace phosphor
{
namespace network
{
namespace ncsi
{

ArgumentParser::ArgumentParser(int argc, char** argv)
{
    int option = 0;
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
    std::cerr << "    --help            Print this menu.\n";
    std::cerr << "    --info=<info>     Retrieve info about NCSI topology.\n";
    std::cerr << "    --set=<set>       Set a specific package/channel.\n";
    std::cerr
        << "    --clear=<clear>   Clear all the settings on the interface.\n";
    std::cerr << "    --package=<package>  Specify a package.\n";
    std::cerr << "    --channel=<channel> Specify a channel.\n";
    std::cerr << "    --index=<device index> Specify device ifindex.\n";
    std::cerr << std::flush;
}

const option ArgumentParser::options[] = {
    {"info", no_argument, NULL, 'i'},
    {"set", no_argument, NULL, 's'},
    {"clear", no_argument, NULL, 'r'},
    {"package", required_argument, NULL, 'p'},
    {"channel", required_argument, NULL, 'c'},
    {"index", required_argument, NULL, 'x'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0},
};

const char* ArgumentParser::optionStr = "i:s:r:p:c:x:h?";

const std::string ArgumentParser::trueString = "true";
const std::string ArgumentParser::emptyString = "";

} // namespace ncsi
} // namespace network
} // namespace phosphor
