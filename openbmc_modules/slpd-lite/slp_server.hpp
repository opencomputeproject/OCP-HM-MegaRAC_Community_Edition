#pragma once

#include "slp.hpp"
#include "slp_meta.hpp"

#include <sys/types.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-daemon.h>
#include <systemd/sd-event.h>

#include <iostream>
#include <string>

namespace slp
{

namespace udp
{
/** General udp server which waits for the POLLIN event
    on the port and calls the call back once it gets the event.
    usage would be create the server with the port and the call back
    and call the run method.
 */
class Server
{

  public:
    Server() : Server(slp::PORT, nullptr){};

    Server(uint16_t port, sd_event_io_handler_t cb) : port(port), callme(cb){};

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = default;
    Server& operator=(Server&&) = default;

    uint16_t port;
    sd_event_io_handler_t callme;

    int run();
};
} // namespace udp
} // namespace slp
