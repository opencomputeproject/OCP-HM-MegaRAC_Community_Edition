#include "config.h"

#include "server-conf.hpp"

#include <sdbusplus/bus.hpp>

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();

    phosphor::rsyslog_config::Server serverConf(
        bus, BUSPATH_REMOTE_LOGGING_CONFIG, RSYSLOG_SERVER_CONFIG_FILE);

    bus.request_name(BUSNAME_SYSLOG_CONFIG);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
