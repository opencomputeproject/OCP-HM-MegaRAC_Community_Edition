#include "config.h"

#include "host_state_manager.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdlib>
#include <exception>
#include <experimental/filesystem>
#include <iostream>

int main()
{
    namespace fs = std::experimental::filesystem;

    auto bus = sdbusplus::bus::new_default();

    // For now, we only have one instance of the host
    auto objPathInst = std::string{HOST_OBJPATH} + '0';

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, objPathInst.c_str());

    phosphor::state::manager::Host manager(bus, objPathInst.c_str());

    auto dir = fs::path(HOST_STATE_PERSIST_PATH).parent_path();
    fs::create_directories(dir);

    bus.request_name(HOST_BUSNAME);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
