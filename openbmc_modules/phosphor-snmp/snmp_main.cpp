#include "config.h"
#include "snmp_conf_manager.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>

#include <memory>

/* Need a custom deleter for freeing up sd_event */
struct EventDeleter
{
    void operator()(sd_event *event) const
    {
        event = sd_event_unref(event);
    }
};

using EventPtr = std::unique_ptr<sd_event, EventDeleter>;

int main(int argc, char *argv[])
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

    // Attach the bus to sd_event to service user requests
    bus.attach_event(eventPtr.get(), SD_EVENT_PRIORITY_NORMAL);

    // Add sdbusplus Object Manager for the 'root' path of the snmp.
    sdbusplus::server::manager::manager objManager(bus, OBJ_NETWORK_SNMP);
    bus.request_name(BUSNAME_NETWORK_SNMP);

    auto manager = std::make_unique<phosphor::network::snmp::ConfManager>(
        bus, OBJ_NETWORK_SNMP);

    manager->restoreClients();

    return sd_event_loop(eventPtr.get());
}
