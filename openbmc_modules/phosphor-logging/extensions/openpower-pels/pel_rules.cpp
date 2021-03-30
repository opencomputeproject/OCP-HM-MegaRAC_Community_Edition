#include "pel_rules.hpp"

#include "pel_types.hpp"

#include <bitset>

namespace openpower
{
namespace pels
{
namespace pel_rules
{

std::tuple<uint16_t, uint8_t> check(uint16_t actionFlags, uint8_t eventType,
                                    uint8_t severity)
{
    std::bitset<16> newActionFlags{actionFlags};
    uint8_t newEventType = eventType;
    auto sevType = static_cast<SeverityType>(severity & 0xF0);

    // TODO: This code covers the most common cases.  The final tweaking
    // will be done with ibm-openbmc/dev#1333.

    // Always report, unless specifically told not to
    if (!newActionFlags.test(dontReportToHostFlagBit))
    {
        newActionFlags.set(reportFlagBit);
    }
    else
    {
        newActionFlags.reset(reportFlagBit);
    }

    // Call home by BMC not supported
    newActionFlags.reset(spCallHomeFlagBit);

    switch (sevType)
    {
        case SeverityType::nonError:
        {
            // Informational errors never need service actions or call home.
            newActionFlags.reset(serviceActionFlagBit);
            newActionFlags.reset(callHomeFlagBit);

            // Ensure event type isn't 'not applicable'
            if (newEventType == static_cast<uint8_t>(EventType::notApplicable))
            {
                newEventType =
                    static_cast<uint8_t>(EventType::miscInformational);
            }

            // The misc info and tracing event types are always hidden.
            // For other event types, it's up to the creator.
            if ((newEventType ==
                 static_cast<uint8_t>(EventType::miscInformational)) ||
                (newEventType == static_cast<uint8_t>(EventType::tracing)))
            {
                newActionFlags.set(hiddenFlagBit);
            }
            break;
        }
        case SeverityType::recovered:
        {
            // Recovered errors are hidden, and by definition need no
            // service action or call home.
            newActionFlags.set(hiddenFlagBit);
            newActionFlags.reset(serviceActionFlagBit);
            newActionFlags.reset(callHomeFlagBit);
            break;
        }
        case SeverityType::predictive:
        case SeverityType::unrecoverable:
        case SeverityType::critical:
        case SeverityType::diagnostic:
        case SeverityType::symptom:
        {
            // Report these others as normal errors.
            newActionFlags.reset(hiddenFlagBit);
            newActionFlags.set(serviceActionFlagBit);
            newActionFlags.set(callHomeFlagBit);
            break;
        }
    }

    return {static_cast<uint16_t>(newActionFlags.to_ulong()), newEventType};
}

} // namespace pel_rules
} // namespace pels
} // namespace openpower
