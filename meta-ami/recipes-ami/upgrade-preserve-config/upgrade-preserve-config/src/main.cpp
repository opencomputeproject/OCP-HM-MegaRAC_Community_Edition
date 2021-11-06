#include "config.h"

#include "conf_manager.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>

struct EventDeleter
{
    void operator()(sd_event *event) const
    {
        event = sd_event_unref(event);
    }
};

using EventPtr = std::unique_ptr<sd_event, EventDeleter>;

int main(int argc, char** argv)
{
    using namespace phosphor::logging;

    auto bus = sdbusplus::bus::new_default();

    sd_event *event = nullptr;
    auto r = sd_event_default(&event);
    if (r < 0)
    {
        log<level::ERR>("Error creating a default sd_event handler");
        return r;
    }

    EventPtr eventPtr{event};
    event = nullptr;

    bus.attach_event(eventPtr.get(), SD_EVENT_PRIORITY_NORMAL);

    sdbusplus::server::manager::manager objManager(bus, OBJ_UPGRADE_PRESERVE);
    bus.request_name(BUSNAME_UPGRADE_PRESERVE);

    auto manager = std::make_unique<
        phosphor::software::preserve::ConfManager>(
            bus, OBJ_UPGRADE_PRESERVE);

    manager->restoreConfig();

    return sd_event_loop(eventPtr.get());
}
