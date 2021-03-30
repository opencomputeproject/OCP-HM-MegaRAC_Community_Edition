#include "preconditions.hpp"

#include "zone.hpp"

#include <phosphor-logging/log.hpp>

#include <algorithm>

namespace phosphor
{
namespace fan
{
namespace control
{
namespace precondition
{

using namespace phosphor::fan;
using namespace phosphor::logging;

Action property_states_match(std::vector<PrecondGroup>&& pg,
                             std::vector<SetSpeedEvent>&& sse)
{
    return [pg = std::move(pg), sse = std::move(sse)](auto& zone, auto& group) {
        // Compare given precondition entries
        auto precondState =
            std::all_of(pg.begin(), pg.end(), [&zone](auto const& entry) {
                try
                {
                    return zone.getPropValueVariant(
                               std::get<pcPathPos>(entry),
                               std::get<pcIntfPos>(entry),
                               std::get<pcPropPos>(entry)) ==
                           std::get<pcValuePos>(entry);
                }
                catch (const std::out_of_range& oore)
                {
                    // Default to property variants not equal when not found
                    return false;
                }
            });

        if (precondState)
        {
            log<level::DEBUG>(
                "Preconditions passed, init the associated events",
                entry("EVENT_COUNT=%u", sse.size()));
            // Init the events when all the precondition(s) are true
            std::for_each(sse.begin(), sse.end(), [&zone](auto const& entry) {
                zone.initEvent(entry);
            });
        }
        else
        {
            log<level::DEBUG>(
                "Preconditions not met for events, events removed if present",
                entry("EVENT_COUNT=%u", sse.size()));
            // Unsubscribe the events' signals when any precondition is false
            std::for_each(sse.begin(), sse.end(), [&zone](auto const& entry) {
                zone.removeEvent(entry);
            });
            zone.setFullSpeed();
        }
        // Update group's fan control active allowed
        zone.setActiveAllow(&group, precondState);
    };
}

Action services_missing_owner(std::vector<SetSpeedEvent>&& sse)
{
    return [sse = std::move(sse)](auto& zone, auto& group) {
        // Set/update the services of the group
        zone.setServices(&group);
        const auto& services = zone.getGroupServices(&group);
        auto precondState =
            std::any_of(services.begin(), services.end(), [](const auto& s) {
                return !std::get<hasOwnerPos>(s);
            });

        if (precondState)
        {
            // Init the events when all the precondition(s) are true
            std::for_each(sse.begin(), sse.end(), [&zone](auto const& entry) {
                zone.initEvent(entry);
            });
        }
        else
        {
            // Unsubscribe the events' signals when any precondition is false
            std::for_each(sse.begin(), sse.end(), [&zone](auto const& entry) {
                zone.removeEvent(entry);
            });
        }
    };
}

} // namespace precondition
} // namespace control
} // namespace fan
} // namespace phosphor
