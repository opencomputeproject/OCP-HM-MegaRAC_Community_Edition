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
 * This program is a utility for accessing GPIOs.
 * Actions:
 *   low:  Set a GPIO low
 *   high: Set a GPIO high
 *   low_high: Set a GPIO low, delay if requested, set it high
 *   high_low: Set a GPIO high, delay if requested, set it low
 */

#include "argument.hpp"
#include "gpio.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <phosphor-logging/log.hpp>
#include <thread>

using namespace phosphor::gpio;
using namespace phosphor::logging;

typedef void (*gpioFunction)(GPIO&, unsigned int);
using gpioFunctionMap = std::map<std::string, gpioFunction>;

/**
 * Sets a GPIO low
 *
 * @param[in] gpio - the GPIO object
 * @param[in] delayInMS - Unused in this function
 */
void low(GPIO& gpio, unsigned int)
{
    gpio.set(GPIO::Value::low);
}

/**
 * Sets a GPIO high
 *
 * @param[in] gpio - the GPIO object
 * @param[in] delayInMS - Unused in this function
 */
void high(GPIO& gpio, unsigned int)
{
    gpio.set(GPIO::Value::high);
}

/**
 * Sets a GPIO high, then delays, then sets it low
 *
 * @param[in] gpio - the GPIO object
 * @param[in] delayInMS - The delay in between the sets
 */
void highLow(GPIO& gpio, unsigned int delayInMS)
{
    gpio.set(GPIO::Value::high);

    std::chrono::milliseconds delay{delayInMS};
    std::this_thread::sleep_for(delay);

    gpio.set(GPIO::Value::low);
}

/**
 * Sets a GPIO low, then delays, then sets it high
 *
 * @param[in] gpio - the GPIO to write
 * @param[in] delayInMS - The delay in between the sets
 */
void lowHigh(GPIO& gpio, unsigned int delayInMS)
{
    gpio.set(GPIO::Value::low);

    std::chrono::milliseconds delay{delayInMS};
    std::this_thread::sleep_for(delay);

    gpio.set(GPIO::Value::high);
}

/**
 * The actions supported by this program
 */
static const gpioFunctionMap functions{
    {"low", low}, {"high", high}, {"low_high", lowHigh}, {"high_low", highLow}};

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

/**
 * Returns the number value of the argument passed in.
 *
 * @param[in] name - the argument name
 * @param[in] parser - the argument parser
 * @param[in] argv - arv from main()
 */
template <typename T>
T getValueFromArg(const char* name, ArgumentParser& parser, char** argv)
{
    char* p = NULL;
    auto val = strtol(parser[name].c_str(), &p, 10);

    // strol sets p on error, also we don't allow negative values
    if (*p || (val < 0))
    {
        using namespace std::string_literals;
        std::string msg = "Invalid "s + name + " value passed in";
        exitWithError(msg.c_str(), argv);
    }
    return static_cast<T>(val);
}

int main(int argc, char** argv)
{
    ArgumentParser args(argc, argv);

    auto path = args["path"];
    if (path == ArgumentParser::emptyString)
    {
        exitWithError("GPIO device path not specified", argv);
    }

    auto action = args["action"];
    if (action == ArgumentParser::emptyString)
    {
        exitWithError("Action not specified", argv);
    }

    if (args["gpio"] == ArgumentParser::emptyString)
    {
        exitWithError("GPIO not specified", argv);
    }

    auto gpioNum = getValueFromArg<gpioNum_t>("gpio", args, argv);

    // Not all actions require a delay, so not required
    unsigned int delay = 0;
    if (args["delay"] != ArgumentParser::emptyString)
    {
        delay = getValueFromArg<decltype(delay)>("delay", args, argv);
    }

    auto function = functions.find(action);
    if (function == functions.end())
    {
        exitWithError("Invalid action value passed in", argv);
    }

    GPIO gpio{path, gpioNum, GPIO::Direction::output};

    try
    {
        function->second(gpio, delay);
    }
    catch (std::runtime_error& e)
    {
        std::cerr << e.what();
        return -1;
    }

    return 0;
}
