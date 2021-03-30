#include "triggers.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace trigger
{

using namespace phosphor::fan;

Trigger timer(TimerConf&& tConf)
{
    return [tConf = std::move(tConf)](
               control::Zone& zone, const std::string& name, const Group& group,
               const std::vector<Action>& actions) {
        zone.addTimer(name, group, actions, tConf);
    };
}

Trigger signal(const std::string& match, SignalHandler&& handler)
{
    return [match = std::move(match), handler = std::move(handler)](
               control::Zone& zone, const std::string& name, const Group& group,
               const std::vector<Action>& actions) {
        // Setup signal matches of the property for event
        std::unique_ptr<EventData> eventData =
            std::make_unique<EventData>(group, match, handler, actions);
        std::unique_ptr<sdbusplus::server::match::match> mPtr = nullptr;
        if (!match.empty())
        {
            // Subscribe to signal match
            mPtr = std::make_unique<sdbusplus::server::match::match>(
                zone.getBus(), match.c_str(),
                std::bind(std::mem_fn(&Zone::handleEvent), &zone,
                          std::placeholders::_1, eventData.get()));
        }
        else
        {
            // When match is empty, handle if zone object member
            // Set event data for each host group member
            for (auto& entry : group)
            {
                if (std::get<pathPos>(entry) == zone.getPath())
                {
                    auto ifaces = zone.getIfaces();
                    // Group member interface in list owned by zone
                    if (std::find(ifaces.begin(), ifaces.end(),
                                  std::get<intfPos>(entry)) != ifaces.end())
                    {
                        // Store path,interface,property as a managed object
                        zone.setObjectData(
                            std::get<pathPos>(entry), std::get<intfPos>(entry),
                            std::get<propPos>(entry), eventData.get());
                    }
                }
            }
        }
        zone.addSignal(name, std::move(eventData), std::move(mPtr));
    };
}

Trigger init(MethodHandler&& handler)
{
    return [handler = std::move(handler)](
               control::Zone& zone, const std::string& name, const Group& group,
               const std::vector<Action>& actions) {
        // A handler function is optional
        if (handler)
        {
            handler(zone, group);
        }

        // Run action functions for initial event state
        std::for_each(
            actions.begin(), actions.end(),
            [&zone, &group](auto const& action) { action(zone, group); });
    };
}

} // namespace trigger
} // namespace control
} // namespace fan
} // namespace phosphor
