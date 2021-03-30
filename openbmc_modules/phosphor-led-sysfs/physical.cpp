/**
 * Copyright © 2016,2018 IBM Corporation
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

#include "physical.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
namespace phosphor
{
namespace led
{

/** @brief Populates key parameters */
void Physical::setInitialState()
{
    assert = led.getMaxBrightness();
    auto trigger = led.getTrigger();
    if (trigger == "timer")
    {
        // LED is blinking. Get the on and off delays and derive percent duty
        auto delayOn = led.getDelayOn();
        uint16_t periodMs = delayOn + led.getDelayOff();
        auto percentScale = periodMs / 100;
        this->dutyOn(delayOn / percentScale);
        this->period(periodMs);
    }
    else
    {
        // Cache current LED state
        auto brightness = led.getBrightness();
        if (brightness == assert)
        {
            sdbusplus::xyz::openbmc_project::Led::server::Physical::state(
                Action::On);
        }
        else
        {
            sdbusplus::xyz::openbmc_project::Led::server::Physical::state(
                Action::Off);
        }
    }
    return;
}

auto Physical::state(Action value) -> Action
{
    auto current =
        sdbusplus::xyz::openbmc_project::Led::server::Physical::state();

    auto requested =
        sdbusplus::xyz::openbmc_project::Led::server::Physical::state(value);

    driveLED(current, requested);

    return value;
}

void Physical::driveLED(Action current, Action request)
{
    if (current == request)
    {
        return;
    }

    if (request == Action::On || request == Action::Off)
    {
        return stableStateOperation(request);
    }

    assert(request == Action::Blink);
    blinkOperation();
}

void Physical::stableStateOperation(Action action)
{
    auto value = (action == Action::On) ? assert : DEASSERT;

    led.setTrigger("none");
    led.setBrightness(value);
    return;
}

void Physical::blinkOperation()
{
    auto dutyOn = this->dutyOn();

    /*
      The configuration of the trigger type must precede the configuration of
      the trigger type properties. From the kernel documentation:
      "You can change triggers in a similar manner to the way an IO scheduler
      is chosen (via /sys/class/leds/<device>/trigger). Trigger specific
      parameters can appear in /sys/class/leds/<device> once a given trigger is
      selected."
      Refer:
      https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/leds/leds-class.txt?h=v5.2#n26
    */
    led.setTrigger("timer");
    // Convert percent duty to milliseconds for sysfs interface
    auto factor = this->period() / 100.0;
    led.setDelayOn(dutyOn * factor);
    led.setDelayOff((100 - dutyOn) * factor);

    return;
}

/** @brief set led color property in DBus*/
void Physical::setLedColor(const std::string& color)
{
    static const std::string prefix =
        "xyz.openbmc_project.Led.Physical.Palette.";
    if (!color.length())
        return;
    std::string tmp = color;
    tmp[0] = toupper(tmp[0]);
    try
    {
        auto palette = convertPaletteFromString(prefix + tmp);
        setPropertyByName("Color", palette);
    }
    catch (const sdbusplus::exception::InvalidEnumString&)
    {
        // if color var contains invalid color,
        // Color property will have default value
    }
}

} // namespace led
} // namespace phosphor
