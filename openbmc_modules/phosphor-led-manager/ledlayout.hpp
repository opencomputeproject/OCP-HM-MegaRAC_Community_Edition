#pragma once

#include <map>
#include <set>
#include <string>

namespace phosphor
{
namespace led
{
/** @namespace Layout
 *  @brief Depicts the LED and their mappings and group actions
 */
namespace Layout
{
/** @brief Define possible actions on a given LED.
 *  For the BLINK operation, follow 50-50 duty cycle
 */
enum Action
{
    Off,
    On,
    Blink,
};

/** @brief Name of the LED and it's proposed action.
 *  This structure is supplied as configuration at build time
 */
struct LedAction
{
    std::string name;
    Action action;
    uint8_t dutyOn;
    uint16_t period;
    Action priority;

    // Order LEDs such that same LEDs are grouped next to
    // each other and the same LEDs are in priority order
    // with the highest priority coming first
    bool operator<(const LedAction& right) const
    {
        if (name == right.name)
        {
            if (action == right.action)
            {
                return false;
            }
            else if (action == priority)
            {
                return true;
            }
        }
        return name < right.name;
    }
};
} // namespace Layout
} // namespace led
} // namespace phosphor
