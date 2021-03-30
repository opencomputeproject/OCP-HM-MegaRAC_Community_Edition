#include "config.h"

#include "settings_manager.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();

    // Add sdbusplus ObjectManager. The settings objects are spread across
    // the object namespace and are not under a (settings) root. Hence register
    // "/" as the path.
    sdbusplus::server::manager::manager objManager(bus, "/");
    bus.request_name(SETTINGS_BUSNAME);

    phosphor::settings::Manager mgr(bus);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
