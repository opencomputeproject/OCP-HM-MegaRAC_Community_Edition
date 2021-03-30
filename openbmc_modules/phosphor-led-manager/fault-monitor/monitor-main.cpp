#include "fru-fault-monitor.hpp"

int main(void)
{
    /** @brief Dbus constructs used by Fault Monitor */
    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();

    phosphor::led::fru::fault::monitor::Add monitor(bus);
    /** @brief Wait for client requests */
    while (true)
    {
        /** @brief process dbus calls / signals discarding unhandled */
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
