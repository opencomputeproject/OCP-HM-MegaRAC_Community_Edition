#pragma once

#include "types.hpp"
#include "util.hpp"

#include <systemd/sd-event.h>

namespace phosphor
{
namespace network
{
namespace rtnetlink
{

constexpr auto BUFSIZE = 4096;

/** General rtnetlink server which waits for the POLLIN event
    and calls the  call back once it gets the event.
    Usage would be create the server with the  call back
    and call the run method.
 */

class Server
{

  public:
    /** @brief Constructor
     *
     *  @details Sets up the server to handle incoming RTNETLINK events
     *
     *  @param[in] eventPtr - Unique ptr reference to sd_event.
     *  @param[in] socket - netlink socket.
     */
    Server(EventPtr& eventPtr, const phosphor::Descriptor& socket);

    Server() = delete;
    ~Server() = default;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = default;
    Server& operator=(Server&&) = default;
};

} // namespace rtnetlink
} // namespace network
} // namespace phosphor
