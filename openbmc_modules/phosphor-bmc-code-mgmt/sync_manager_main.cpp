#include "config.h"

#include "sync_manager.hpp"
#include "sync_watch.hpp"

#include <systemd/sd-event.h>

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>

#include <exception>

int main()
{
    auto bus = sdbusplus::bus::new_default();

    sd_event* loop = nullptr;
    sd_event_default(&loop);

    sdbusplus::server::manager::manager objManager(bus, SOFTWARE_OBJPATH);

    try
    {
        phosphor::software::manager::Sync syncManager;

        using namespace phosphor::software::manager;
        phosphor::software::manager::SyncWatch watch(
            *loop, std::bind(std::mem_fn(&Sync::processEntry), &syncManager,
                             std::placeholders::_1, std::placeholders::_2));
        bus.attach_event(loop, SD_EVENT_PRIORITY_NORMAL);
        sd_event_loop(loop);
    }
    catch (std::exception& e)
    {
        using namespace phosphor::logging;
        log<level::ERR>(e.what());
        sd_event_unref(loop);
        return -1;
    }

    sd_event_unref(loop);

    return 0;
}
