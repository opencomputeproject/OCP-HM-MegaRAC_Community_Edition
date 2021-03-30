#include "actions.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace action
{

using namespace phosphor::fan;

Action call_actions_based_on_timer(TimerConf&& tConf,
                                   std::vector<Action>&& actions)
{
    return [tConf = std::move(tConf), actions = std::move(actions)](
               control::Zone& zone, const Group& group) {
        try
        {
            auto it = zone.getTimerEvents().find(__func__);
            if (it != zone.getTimerEvents().end())
            {
                auto& timers = it->second;
                auto timerIter = zone.findTimer(group, actions, timers);
                if (timerIter == timers.end())
                {
                    // No timer exists yet for action, add timer
                    zone.addTimer(__func__, group, actions, tConf);
                }
                else if (timerIter != timers.end())
                {
                    // Remove any timer for this group
                    timers.erase(timerIter);
                    if (timers.empty())
                    {
                        zone.getTimerEvents().erase(it);
                    }
                }
            }
            else
            {
                // No timer exists yet for event, add timer
                zone.addTimer(__func__, group, actions, tConf);
            }
        }
        catch (const std::out_of_range& oore)
        {
            // Group not found, no timers set
        }
    };
}

void default_floor_on_missing_owner(Zone& zone, const Group& group)
{
    // Set/update the services of the group
    zone.setServices(&group);
    auto services = zone.getGroupServices(&group);
    auto defFloor =
        std::any_of(services.begin(), services.end(),
                    [](const auto& s) { return !std::get<hasOwnerPos>(s); });
    if (defFloor)
    {
        zone.setFloor(zone.getDefFloor());
    }
    // Update fan control floor change allowed
    zone.setFloorChangeAllow(&group, !defFloor);
}

Action set_speed_on_missing_owner(uint64_t speed)
{
    return [speed](control::Zone& zone, const Group& group) {
        // Set/update the services of the group
        zone.setServices(&group);
        auto services = zone.getGroupServices(&group);
        auto missingOwner =
            std::any_of(services.begin(), services.end(), [](const auto& s) {
                return !std::get<hasOwnerPos>(s);
            });
        if (missingOwner)
        {
            zone.setSpeed(speed);
        }
        // Update group's fan control active allowed based on action results
        zone.setActiveAllow(&group, !missingOwner);
    };
}

void set_request_speed_base_with_max(control::Zone& zone, const Group& group)
{
    int64_t base = 0;
    std::for_each(
        group.begin(), group.end(), [&zone, &base](auto const& entry) {
            try
            {
                auto value = zone.template getPropertyValue<int64_t>(
                    std::get<pathPos>(entry), std::get<intfPos>(entry),
                    std::get<propPos>(entry));
                base = std::max(base, value);
            }
            catch (const std::out_of_range& oore)
            {
                // Property value not found, base request speed unchanged
            }
        });
    // A request speed base of 0 defaults to the current target speed
    zone.setRequestSpeedBase(base);
}

} // namespace action
} // namespace control
} // namespace fan
} // namespace phosphor
