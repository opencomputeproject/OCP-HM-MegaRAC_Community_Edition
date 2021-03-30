#include "config.h"

#include "extensions.hpp"
#include "log_manager.hpp"

#include <experimental/filesystem>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdeventplus/event.hpp>

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    // Add sdbusplus ObjectManager for the 'root' path of the logging manager.
    sdbusplus::server::manager::manager objManager(bus, OBJ_LOGGING);

    phosphor::logging::internal::Manager iMgr(bus, OBJ_INTERNAL);

    phosphor::logging::Manager mgr(bus, OBJ_LOGGING, iMgr);

    // Create a directory to persist errors.
    std::experimental::filesystem::create_directories(ERRLOG_PERSIST_PATH);

    // Recreate error d-bus objects from persisted errors.
    iMgr.restore();

    bus.request_name(BUSNAME_LOGGING);

    for (auto& startup : phosphor::logging::Extensions::getStartupFunctions())
    {
        try
        {
            startup(iMgr);
        }
        catch (std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "An extension's startup function threw an exception",
                phosphor::logging::entry("ERROR=%s", e.what()));
        }
    }

    return event.loop();
}
