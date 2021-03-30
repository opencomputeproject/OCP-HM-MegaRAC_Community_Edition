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

/**
 * This application will check the ActiveState property
 * on the source unit passed in.  If that state is 'failed',
 * then it will either stop or start the target unit, depending
 * on the command line arguments.
 */
#include "argument.hpp"
#include "monitor.hpp"

#include <iostream>
#include <map>

using namespace phosphor::unit::failure;

/**
 * Prints usage and exits the program
 *
 * @param[in] err - the error message to print
 * @param[in] argv - argv from main()
 */
void exitWithError(const char* err, char** argv)
{
    std::cerr << "ERROR: " << err << "\n";
    ArgumentParser::usage(argv);
    exit(EXIT_FAILURE);
}

static const std::map<std::string, Monitor::Action> actions = {
    {"start", Monitor::Action::start}, {"stop", Monitor::Action::stop}};

int main(int argc, char** argv)
{
    ArgumentParser args(argc, argv);

    auto source = args["source"];
    if (source == ArgumentParser::emptyString)
    {
        exitWithError("Source not specified", argv);
    }

    auto target = args["target"];
    if (target == ArgumentParser::emptyString)
    {
        exitWithError("Target not specified", argv);
    }

    auto a = actions.find(args["action"]);
    if (a == actions.end())
    {
        exitWithError("Missing or invalid action specified", argv);
    }

    Monitor monitor{source, target, a->second};

    monitor.analyze();

    return 0;
}
