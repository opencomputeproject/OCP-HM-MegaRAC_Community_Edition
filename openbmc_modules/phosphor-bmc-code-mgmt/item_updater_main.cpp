#include "config.h"

#include "item_updater.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>

int main()
{
    auto bus = sdbusplus::bus::new_default();

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, SOFTWARE_OBJPATH);

    phosphor::software::updater::ItemUpdater updater(bus, SOFTWARE_OBJPATH);

    bus.request_name(BUSNAME_UPDATER);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
