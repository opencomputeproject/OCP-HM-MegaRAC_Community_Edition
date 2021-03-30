#include "config.h"

#include "cpldimage_manager.hpp"
#include "cpldwatch.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <exception>

int main()
{
    using namespace phosphor::cpld::manager;
    auto bus = sdbusplus::bus::new_default();

    sd_event* loop = nullptr;
    sd_event_default(&loop);

    sdbusplus::server::manager::manager objManager(bus, CPLD_OBJPATH);
    bus.request_name(CPLD_VERSION_BUSNAME);

    try
    {
        phosphor::cpld::manager::Manager imageManager(bus);
        phosphor::cpld::manager::Watch watch(
            loop, std::bind(std::mem_fn(&Manager::cpldprocessImage), &imageManager,
                            std::placeholders::_1));
        bus.attach_event(loop, SD_EVENT_PRIORITY_NORMAL);
        sd_event_loop(loop);
    }
    catch (std::exception& e)
    {
        using namespace phosphor::logging;
        log<level::ERR>(e.what());
        return -1;
    }

    sd_event_unref(loop);

    return 0;
}
