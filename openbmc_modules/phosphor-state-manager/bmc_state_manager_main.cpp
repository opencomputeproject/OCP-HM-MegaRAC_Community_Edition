#include "config.h"

#include "bmc_state_manager.hpp"

#include <sdbusplus/bus.hpp>

int main()
{
    auto bus = sdbusplus::bus::new_default();

    // For now, we only have one instance of the BMC
    // 0 is for the current instance
    auto objPathInst = std::string(BMC_OBJPATH) + '0';

    // Add sdbusplus ObjectManager.
    sdbusplus::server::manager::manager objManager(bus, objPathInst.c_str());

    phosphor::state::manager::BMC manager(bus, objPathInst.c_str());

    bus.request_name(BMC_BUSNAME);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    exit(EXIT_SUCCESS);
}
