#include "config.h"

#include "neighbor.hpp"

#include "ethernet_interface.hpp"
#include "netlink.hpp"
#include "util.hpp"

#include <linux/neighbour.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <stdexcept>
#include <stdplus/raw.hpp>
#include <string_view>
#include <utility>
#include <vector>

namespace phosphor
{
namespace network
{
namespace detail
{

void parseNeighbor(const NeighborFilter& filter, const nlmsghdr& hdr,
                   std::string_view msg, std::vector<NeighborInfo>& neighbors)
{
    if (hdr.nlmsg_type != RTM_NEWNEIGH)
    {
        throw std::runtime_error("Not a neighbor msg");
    }
    auto ndm = stdplus::raw::extract<ndmsg>(msg);

    // Filter out neighbors we don't care about
    unsigned ifindex = ndm.ndm_ifindex;
    if (filter.interface != 0 && filter.interface != ifindex)
    {
        return;
    }
    if ((ndm.ndm_state & filter.state) == 0)
    {
        return;
    }

    // Build the neighbor info for our valid neighbor
    NeighborInfo neighbor;
    neighbor.interface = ifindex;
    neighbor.state = ndm.ndm_state;
    bool set_addr = false;
    while (!msg.empty())
    {
        auto [hdr, data] = netlink::extractRtAttr(msg);
        if (hdr.rta_type == NDA_LLADDR)
        {
            neighbor.mac = stdplus::raw::copyFrom<ether_addr>(data);
        }
        else if (hdr.rta_type == NDA_DST)
        {
            neighbor.address = addrFromBuf(ndm.ndm_family, data);
            set_addr = true;
        }
    }
    if (!set_addr)
    {
        throw std::runtime_error("Missing address");
    }
    neighbors.push_back(std::move(neighbor));
}

} // namespace detail

std::vector<NeighborInfo> getCurrentNeighbors(const NeighborFilter& filter)
{
    std::vector<NeighborInfo> neighbors;
    auto cb = [&filter, &neighbors](const nlmsghdr& hdr, std::string_view msg) {
        detail::parseNeighbor(filter, hdr, msg, neighbors);
    };
    ndmsg msg{};
    msg.ndm_ifindex = filter.interface;
    netlink::performRequest(NETLINK_ROUTE, RTM_GETNEIGH, NLM_F_DUMP, msg, cb);
    return neighbors;
}

Neighbor::Neighbor(sdbusplus::bus::bus& bus, const char* objPath,
                   EthernetInterface& parent, const std::string& ipAddress,
                   const std::string& macAddress, State state) :
    NeighborObj(bus, objPath, true),
    parent(parent)
{
    this->iPAddress(ipAddress);
    this->mACAddress(macAddress);
    this->state(state);

    // Emit deferred signal.
    emit_object_added();
}

void Neighbor::delete_()
{
    parent.deleteStaticNeighborObject(iPAddress());
}

} // namespace network
} // namespace phosphor
