#include "utility.hpp"

#include <algorithm>
#include <cstring>
#include <experimental/filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

using json = nlohmann::json;
using namespace phosphor::logging;
using namespace utility;

constexpr auto gpioDefsFile = "/etc/default/obmc/gpio/gpio_defs.json";

int main(void)
{
    std::ifstream gpioDefsStream(gpioDefsFile);

    if (!gpioDefsStream.is_open())
    {
        log<level::ERR>("Error opening gpio definitions file",
                        entry("FILE=%s", gpioDefsFile));
        return 1;
    }

    auto data = json::parse(gpioDefsStream, nullptr, false);

    if (data.is_discarded())
    {
        log<level::ERR>("Error parsing gpio definitions file",
                        entry("FILE=%s", gpioDefsFile));
        return 1;
    }

    // To determine what pins are needed to deassert, look in the
    // gpioDefsFile for the defined "power_up_outs" under
    // gpio_configs->power_config. Then match the name up with
    // its definition in "gpio_definitions" to determine the pin id's.
    auto gpios = data["gpio_configs"]["power_config"]["power_up_outs"];

    if (gpios.size() <= 0)
    {
        log<level::ERR>("Could not find power_up_outs defs",
                        entry("FILE=%s", gpioDefsFile));
        return 1;
    }

    auto defs = data["gpio_definitions"];

    for (const auto& gpio : gpios)
    {
        auto gpioEntry = std::find_if(defs.begin(), defs.end(),
                            [&gpio](const auto& g) {
                                return g["name"] == gpio["name"];
                            });

        if (gpioEntry != defs.end())
        {
            std::string pin = (*gpioEntry)["pin"];
            bool activeLow = gpio["polarity"];

            if (!gpioSetValue(pin, !activeLow, false))
            {
                log<level::ERR>("chassiskill::gpioSetValue() failed",
                                entry("PIN=%s", pin.c_str()));
                return 1;
            }
            else
            {
                log<level::INFO>("'chassiskill' operation complete",
                                 entry("PIN=%s", pin.c_str()));
            }
        }
    }

    return 0;
}
