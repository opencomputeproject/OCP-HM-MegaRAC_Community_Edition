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
#include "ncsi_util.hpp"

#include <iostream>
#include <string>

static void exitWithError(const char* err, char** argv)
{
    phosphor::network::ncsi::ArgumentParser::usage(argv);
    std::cerr << "ERROR: " << err << "\n";
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    using namespace phosphor::network;
    using namespace phosphor::network::ncsi;
    // Read arguments.
    auto options = ArgumentParser(argc, argv);
    int packageInt{};
    int channelInt{};
    int indexInt{};

    // Parse out interface argument.
    auto ifIndex = (options)["index"];
    try
    {
        indexInt = stoi(ifIndex, nullptr);
    }
    catch (const std::exception& e)
    {
        exitWithError("Interface not specified.", argv);
    }

    if (indexInt < 0)
    {
        exitWithError("Interface value should be greater than equal to 0",
                      argv);
    }

    // Parse out package argument.
    auto package = (options)["package"];
    try
    {
        packageInt = stoi(package, nullptr);
    }
    catch (const std::exception& e)
    {
        packageInt = DEFAULT_VALUE;
    }

    if (packageInt < 0)
    {
        packageInt = DEFAULT_VALUE;
    }

    // Parse out channel argument.
    auto channel = (options)["channel"];
    try
    {
        channelInt = stoi(channel, nullptr);
    }
    catch (const std::exception& e)
    {
        channelInt = DEFAULT_VALUE;
    }

    if (channelInt < 0)
    {
        channelInt = DEFAULT_VALUE;
    }

    auto setCmd = (options)["set"];
    if (setCmd == "true")
    {
        // Can not perform set operation without package.
        if (packageInt == DEFAULT_VALUE)
        {
            exitWithError("Package not specified.", argv);
        }
        return ncsi::setChannel(indexInt, packageInt, channelInt);
    }
    else if ((options)["info"] == "true")
    {
        return ncsi::getInfo(indexInt, packageInt);
    }
    else if ((options)["clear"] == "true")
    {
        return ncsi::clearInterface(indexInt);
    }
    else
    {
        exitWithError("No Command specified", argv);
    }
    return 0;
}
