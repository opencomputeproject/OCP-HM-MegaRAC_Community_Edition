#include "routing_table.hpp"

#include "util.hpp"

#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <optional>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <stdexcept>
#include <string_view>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace network
{
namespace route
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

Table::Table()
{
    try
    {
        getRoutes();
    }
    catch (InternalFailure& e)
    {
        commit<InternalFailure>();
    }
}

int Table::readNetLinkSock(int sockFd, std::array<char, BUFSIZE>& buf)
{
    struct nlmsghdr* nlHdr = nullptr;
    int readLen{};
    int msgLen{};
    uint8_t seqNum = 1;
    uint8_t pID = getpid();
    char* bufPtr = buf.data();

    do
    {
        // Receive response from the kernel
        if ((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)
        {
            auto error = errno;
            log<level::ERR>("Socket recv failed:",
                            entry("ERROR=%s", strerror(error)));
            elog<InternalFailure>();
        }

        nlHdr = reinterpret_cast<nlmsghdr*>(bufPtr);

        // Check if the header is valid

        if ((NLMSG_OK(nlHdr, readLen) == 0) ||
            (nlHdr->nlmsg_type == NLMSG_ERROR))
        {

            auto error = errno;
            log<level::ERR>("Error validating header",
                            entry("NLMSGTYPE=%d", nlHdr->nlmsg_type),
                            entry("ERROR=%s", strerror(error)));
            elog<InternalFailure>();
        }

        // Check if the its the last message
        if (nlHdr->nlmsg_type == NLMSG_DONE)
        {
            break;
        }
        else
        {
            // Else move the pointer to buffer appropriately
            bufPtr += readLen;
            msgLen += readLen;
        }

        // Check if its a multi part message
        if ((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
        {
            break;
        }
    } while ((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pID));
    return msgLen;
}

void Table::parseRoutes(const nlmsghdr* nlHdr)
{
    rtmsg* rtMsg = nullptr;
    rtattr* rtAttr = nullptr;
    int rtLen{};
    std::optional<InAddrAny> dstAddr;
    std::optional<InAddrAny> gateWayAddr;
    char ifName[IF_NAMESIZE] = {};

    rtMsg = reinterpret_cast<rtmsg*>(NLMSG_DATA(nlHdr));

    // If the route is not for AF_INET{,6} or does not belong to main routing
    // table then return.
    if ((rtMsg->rtm_family != AF_INET && rtMsg->rtm_family != AF_INET6) ||
        rtMsg->rtm_table != RT_TABLE_MAIN)
    {
        return;
    }

    // get the rtattr field
    rtAttr = reinterpret_cast<rtattr*>(RTM_RTA(rtMsg));

    rtLen = RTM_PAYLOAD(nlHdr);

    for (; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen))
    {
        std::string_view attrData(reinterpret_cast<char*>(RTA_DATA(rtAttr)),
                                  RTA_PAYLOAD(rtAttr));
        switch (rtAttr->rta_type)
        {
            case RTA_OIF:
                if_indextoname(*reinterpret_cast<int*>(RTA_DATA(rtAttr)),
                               ifName);
                break;
            case RTA_GATEWAY:
                gateWayAddr = addrFromBuf(rtMsg->rtm_family, attrData);
                break;
            case RTA_DST:
                dstAddr = addrFromBuf(rtMsg->rtm_family, attrData);
                break;
        }
    }

    std::string dstStr;
    if (dstAddr)
    {
        dstStr = toString(*dstAddr);
    }
    std::string gatewayStr;
    if (gateWayAddr)
    {
        gatewayStr = toString(*gateWayAddr);
    }
    if (!dstAddr && gateWayAddr)
    {
        if (rtMsg->rtm_family == AF_INET)
        {
            defaultGateway = gatewayStr;
        }
        else if (rtMsg->rtm_family == AF_INET6)
        {
            defaultGateway6 = gatewayStr;
        }
    }

    Entry route(dstStr, gatewayStr, ifName);
    // if there is already existing route for this network
    // then ignore the next one as it would not be used by the
    // routing policy
    // So don't update the route entry for the network for which
    // there is already a route exist.
    if (routeList.find(dstStr) == routeList.end())
    {
        routeList.emplace(std::make_pair(dstStr, std::move(route)));
    }
}

Map Table::getRoutes()
{
    nlmsghdr* nlMsg = nullptr;
    std::array<char, BUFSIZE> msgBuf = {0};

    int sock = -1;
    int len{0};

    uint8_t msgSeq{0};

    // Create Socket
    if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
    {
        auto error = errno;
        log<level::ERR>("Error occurred during socket creation",
                        entry("ERRNO=%s", strerror(error)));
        elog<InternalFailure>();
    }

    phosphor::Descriptor smartSock(sock);
    sock = -1;

    // point the header and the msg structure pointers into the buffer.
    nlMsg = reinterpret_cast<nlmsghdr*>(msgBuf.data());
    // Length of message
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(rtmsg));
    // Get the routes from kernel routing table
    nlMsg->nlmsg_type = RTM_GETROUTE;
    // The message is a request for dump
    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;

    nlMsg->nlmsg_seq = msgSeq;
    nlMsg->nlmsg_pid = getpid();

    // Send the request
    if (send(smartSock(), nlMsg, nlMsg->nlmsg_len, 0) < 0)
    {
        auto error = errno;
        log<level::ERR>("Error occurred during send on netlink socket",
                        entry("ERRNO=%s", strerror(error)));
        elog<InternalFailure>();
    }

    // Read the response
    len = readNetLinkSock(smartSock(), msgBuf);

    // Parse and print the response
    for (; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len))
    {
        parseRoutes(nlMsg);
    }
    return routeList;
}

} // namespace route
} // namespace network
} // namespace phosphor
