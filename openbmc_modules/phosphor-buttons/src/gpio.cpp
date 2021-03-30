/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "gpio.hpp"

#include "settings.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <experimental/filesystem>
#include <fstream>
#include <gpioplus/utility/aspeed.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <phosphor-logging/log.hpp>
#include <tuple>

const std::string gpioDev = "/sys/class/gpio";
static constexpr auto gpioDefs = "/etc/default/obmc/gpio/gpio_defs.json";

using namespace phosphor::logging;
namespace fs = std::experimental::filesystem;

void closeGpio(int fd)
{
    if (fd > 0)
    {
        ::close(fd);
    }
}

uint32_t getGpioBase()
{
    // Look for a /sys/class/gpio/gpiochip*/label file
    // with a value of GPIO_BASE_LABEL_NAME.  Then read
    // the base value from the 'base' file in that directory.
#ifdef LOOKUP_GPIO_BASE
    for (auto& f : fs::directory_iterator(gpioDev))
    {
        std::string path{f.path()};
        if (path.find("gpiochip") == std::string::npos)
        {
            continue;
        }

        std::ifstream labelStream{path + "/label"};
        std::string label;
        labelStream >> label;

        if (label == GPIO_BASE_LABEL_NAME)
        {
            uint32_t base;
            std::ifstream baseStream{path + "/base"};
            baseStream >> base;
            return base;
        }
    }

    log<level::ERR>("Could not find GPIO base");
    throw std::runtime_error("Could not find GPIO base!");
#else
    return 0;
#endif
}

uint32_t getGpioNum(const std::string& gpioPin)
{
    // gpioplus promises that they will figure out how to easily
    // support multiple BMC vendors when the time comes.
    auto offset = gpioplus::utility::aspeed::nameToOffset(gpioPin);

    return getGpioBase() + offset;
}

bool gpioDefined(const std::string& gpioName)
{
    try
    {
        std::ifstream gpios{gpioDefs};
        auto json = nlohmann::json::parse(gpios, nullptr, true);
        auto defs = json["gpio_definitions"];

        auto gpio =
            std::find_if(defs.begin(), defs.end(), [&gpioName](const auto g) {
                return gpioName == g["name"];
            });

        if (gpio != defs.end())
        {
            return true;
        }
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Error parsing GPIO JSON", entry("ERROR=%s", e.what()),
                        entry("GPIO_NAME=%s", gpioName.c_str()));
    }
    return false;
}

std::optional<std::tuple<int, std::string>>
    getGpioConfig(const std::string& gpioName)
{

    try
    {
        std::ifstream gpios{gpioDefs};
        auto json = nlohmann::json::parse(gpios, nullptr, true);
        auto defs = json["gpio_definitions"];

        auto gpio =
            std::find_if(defs.begin(), defs.end(), [&gpioName](const auto g) {
                return gpioName == g["name"];
            });

        if (gpio != defs.end())
        {
            return std::make_tuple(getGpioNum((*gpio)["pin"]),
                                   (*gpio)["direction"]);
        }
        else
        {
            log<level::ERR>("Unable to find GPIO in the definitions",
                            entry("GPIO_NAME=%s", gpioName.c_str()));
        }
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Error parsing GPIO JSON", entry("ERROR=%s", e.what()),
                        entry("GPIO_NAME=%s", gpioName.c_str()));
    }
    return {};
}

int configGpio(const char* gpioName, int* fd, sdbusplus::bus::bus& bus)
{
    auto config = getGpioConfig(gpioName);
    if (!config)
    {
        return -1;
    }

    auto [gpioNum, gpioDirection] = *config;

    std::string devPath{gpioDev};

    std::fstream stream;

    stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    devPath += "/gpio" + std::to_string(gpioNum) + "/value";

    fs::path fullPath(devPath);

    if (fs::exists(fullPath))
    {
        log<level::INFO>("GPIO exported", entry("PATH=%s", devPath.c_str()));
    }
    else
    {
        devPath = gpioDev + "/export";

        stream.open(devPath, std::fstream::out);
        try
        {
            stream << gpioNum;
            stream.close();
        }

        catch (const std::exception& e)
        {
            log<level::ERR>("Error in writing!",
                            entry("PATH=%s", devPath.c_str()),
                            entry("NUM=%d", gpioNum));
            return -1;
        }
    }

    if (gpioDirection == "out")
    {
        devPath = gpioDev + "/gpio" + std::to_string(gpioNum) + "/value";

        uint32_t currentValue;

        stream.open(devPath, std::fstream::in);
        try
        {
            stream >> currentValue;
            stream.close();
        }

        catch (const std::exception& e)
        {
            log<level::ERR>("Error in reading!",
                            entry("PATH=%s", devPath.c_str()));
            return -1;
        }

        const char* direction = currentValue ? "high" : "low";

        devPath.clear();
        devPath = gpioDev + "/gpio" + std::to_string(gpioNum) + "/direction";

        stream.open(devPath, std::fstream::out);
        try
        {
            stream << direction;
            stream.close();
        }

        catch (const std::exception& e)
        {
            log<level::ERR>("Error in writing!");
            return -1;
        }
    }
    else if (gpioDirection == "in")
    {
        devPath = gpioDev + "/gpio" + std::to_string(gpioNum) + "/direction";

        stream.open(devPath, std::fstream::out);
        try
        {
            stream << gpioDirection;
            stream.close();
        }

        catch (const std::exception& e)
        {
            log<level::ERR>("Error in writing!");
            return -1;
        }
    }
    else if ((gpioDirection == "both"))
    {
        // Before set gpio configure as an interrupt pin, need to set direction
        // as 'in' or edge can't set as 'rising', 'falling' and 'both'
        const char* in_direction = "in";
        devPath = gpioDev + "/gpio" + std::to_string(gpioNum) + "/direction";

        stream.open(devPath, std::fstream::out);
        try
        {
            stream << in_direction;
            stream.close();
        }

        catch (const std::exception& e)
        {
            log<level::ERR>("Error in writing!");
            return -1;
        }
        devPath.clear();

        // For gpio configured as ‘both’, it is an interrupt pin and trigged on
        // both rising and falling signals
        devPath = gpioDev + "/gpio" + std::to_string(gpioNum) + "/edge";

        stream.open(devPath, std::fstream::out);
        try
        {
            stream << gpioDirection;
            stream.close();
        }

        catch (const std::exception& e)
        {
            log<level::ERR>("Error in writing!");
            return -1;
        }
    }

    devPath = gpioDev + "/gpio" + std::to_string(gpioNum) + "/value";

    *fd = ::open(devPath.c_str(), O_RDWR | O_NONBLOCK);

    if (*fd < 0)
    {
        log<level::ERR>("open error!");
        return -1;
    }

    return 0;
}
