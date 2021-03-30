#include "rtnetlink_server.hpp"

#include "types.hpp"
#include "util.hpp"

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <systemd/sd-daemon.h>
#include <unistd.h>

#include <memory>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <string_view>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace network
{

extern std::unique_ptr<Timer> refreshObjectTimer;

namespace rtnetlink
{

static bool shouldRefresh(const struct nlmsghdr& hdr, std::string_view data)
{
    switch (hdr.nlmsg_type)
    {
        case RTM_NEWADDR:
        case RTM_DELADDR:
        case RTM_NEWROUTE:
        case RTM_DELROUTE:
        {
            return true;
        }
        case RTM_NEWNEIGH:
        case RTM_DELNEIGH:
        {
            struct ndmsg ndm;
            if (data.size() < sizeof(ndm))
            {
                return false;
            }
            memcpy(&ndm, data.data(), sizeof(ndm));
            // We only want to refresh for static neighbors
            return ndm.ndm_state & NUD_PERMANENT;
        }
    }

    return false;
}

/* Call Back for the sd event loop */
static int eventHandler(sd_event_source* /*es*/, int fd, uint32_t /*revents*/,
                        void* /*userdata*/)
{
    char buffer[phosphor::network::rtnetlink::BUFSIZE]{};
    int len{};

    auto netLinkHeader = reinterpret_cast<struct nlmsghdr*>(buffer);
    while ((len = recv(fd, netLinkHeader, phosphor::network::rtnetlink::BUFSIZE,
                       0)) > 0)
    {
        for (; (NLMSG_OK(netLinkHeader, len)) &&
               (netLinkHeader->nlmsg_type != NLMSG_DONE);
             netLinkHeader = NLMSG_NEXT(netLinkHeader, len))
        {
            std::string_view data(
                reinterpret_cast<const char*>(NLMSG_DATA(netLinkHeader)),
                netLinkHeader->nlmsg_len - NLMSG_HDRLEN);
            if (shouldRefresh(*netLinkHeader, data))
            {
                // starting the timer here to make sure that we don't want
                // create the child objects multiple times.
                if (!refreshObjectTimer->isEnabled())
                {
                    // if start timer throws exception then let the application
                    // crash
                    refreshObjectTimer->restartOnce(refreshTimeout);
                } // end if
            }     // end if

        } // end for

    } // end while

    return 0;
}

Server::Server(EventPtr& eventPtr, const phosphor::Descriptor& smartSock)
{
    using namespace phosphor::logging;
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
    struct sockaddr_nl addr
    {
    };
    int r{};

    sigset_t ss{};
    // check that the given socket is valid or not.
    if (smartSock() < 0)
    {
        r = -EBADF;
        goto finish;
    }

    if (sigemptyset(&ss) < 0 || sigaddset(&ss, SIGTERM) < 0 ||
        sigaddset(&ss, SIGINT) < 0)
    {
        r = -errno;
        goto finish;
    }
    /* Block SIGTERM first, so that the event loop can handle it */
    if (sigprocmask(SIG_BLOCK, &ss, NULL) < 0)
    {
        r = -errno;
        goto finish;
    }

    /* Let's make use of the default handler and "floating"
       reference features of sd_event_add_signal() */

    r = sd_event_add_signal(eventPtr.get(), NULL, SIGTERM, NULL, NULL);
    if (r < 0)
    {
        goto finish;
    }

    r = sd_event_add_signal(eventPtr.get(), NULL, SIGINT, NULL, NULL);
    if (r < 0)
    {
        goto finish;
    }

    std::memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR |
                     RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE | RTMGRP_NEIGH;

    if (bind(smartSock(), (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        r = -errno;
        goto finish;
    }

    r = sd_event_add_io(eventPtr.get(), nullptr, smartSock(), EPOLLIN,
                        eventHandler, nullptr);
    if (r < 0)
    {
        goto finish;
    }

finish:

    if (r < 0)
    {
        log<level::ERR>("Failure Occurred in starting of server:",
                        entry("ERRNO=%d", errno));
        elog<InternalFailure>();
    }
}

} // namespace rtnetlink
} // namespace network
} // namespace phosphor
