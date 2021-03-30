#pragma once

#include <gpioplus/chip.hpp>
#include <gpioplus/handle.hpp>
#include <gpioplus/utility/aspeed.hpp>
#include <phosphor-logging/log.hpp>
#include <exception>
#include <string>

namespace utility
{

/** @brief Set the value of a specified gpio
 *
 *  @param[in] gpioName - GPIO to set
 *  @param[in] activeLow - is pin active at low voltage
 *  @param[in] asserted - is pin in active state
 *
 *  @return bool - success of setting the GPIO
 */
bool gpioSetValue(const std::string& gpioName, bool activeLow, bool asserted)
{
    uint32_t gpioOffset;
    try
    {
        gpioOffset = gpioplus::utility::aspeed::nameToOffset(gpioName);
    }
    catch(const std::logic_error& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
                            "Error in gpioplus - nameToOffset",
                            phosphor::logging::entry("ERROR=%s", e.what()));
        return false;
    }

    try
    {
        //TODO: openbmc/phosphor-power-control#1 - Handle cases where gpiochip
        //      could be non-zero.
        gpioplus::Chip chip(0);
        gpioplus::HandleFlags flags(chip.getLineInfo(gpioOffset).flags);
        flags.output = true;
        gpioplus::Handle handle(chip, {{gpioOffset, 0}}, flags,
                                "chassiskill");

        bool value = (asserted ^ activeLow);
        handle.setValues({value});
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
                            "Error in gpioplus",
                            phosphor::logging::entry("ERROR=%s", e.what()));
        return false;
    }

    return true;
}

} // namespace utility
