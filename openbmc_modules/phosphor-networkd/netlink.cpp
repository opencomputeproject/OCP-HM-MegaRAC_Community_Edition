#include "netlink.hpp"

#include "util.hpp"

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <stdexcept>
#include <stdplus/raw.hpp>
#include <system_error>

namespace phosphor
{
namespace network
{
namespace netlink
{
namespace detail
{

void processMsg(std::string_view& msgs, bool& done, const ReceiveCallback& cb)
{
    // Parse and update the message buffer
    auto hdr = stdplus::raw::copyFrom<nlmsghdr>(msgs);
    if (hdr.nlmsg_len < sizeof(hdr))
    {
        throw std::runtime_error("Invalid nlmsg length");
    }
    if (msgs.size() < hdr.nlmsg_len)
    {
        throw std::runtime_error("Bad nlmsg payload");
    }
    auto msg = msgs.substr(NLMSG_HDRLEN, hdr.nlmsg_len - NLMSG_HDRLEN);
    msgs.remove_prefix(NLMSG_ALIGN(hdr.nlmsg_len));

    // Figure out how to handle the individual message
    bool doCallback = true;
    if (hdr.nlmsg_flags & NLM_F_MULTI)
    {
        done = false;
    }
    if (hdr.nlmsg_type == NLMSG_NOOP)
    {
        doCallback = false;
    }
    else if (hdr.nlmsg_type == NLMSG_DONE)
    {
        if (done)
        {
            throw std::runtime_error("Got done for non-multi msg");
        }
        done = true;
        doCallback = false;
    }
    else if (hdr.nlmsg_type == NLMSG_ERROR)
    {
        auto err = stdplus::raw::copyFrom<nlmsgerr>(msg);
        // This is just an ACK so don't do the callback
        if (err.error <= 0)
        {
            doCallback = false;
        }
    }
    // All multi-msg headers must have the multi flag
    if (!done && !(hdr.nlmsg_flags & NLM_F_MULTI))
    {
        throw std::runtime_error("Got non-multi msg before done");
    }
    if (doCallback)
    {
        cb(hdr, msg);
    }
}

static void receive(int sock, const ReceiveCallback& cb)
{
    // We need to make sure we have enough room for an entire packet otherwise
    // it gets truncated. The netlink docs guarantee packets will not exceed 8K
    std::array<char, 8192> buf;

    iovec iov{};
    iov.iov_base = buf.data();
    iov.iov_len = buf.size();

    sockaddr_nl from{};
    from.nl_family = AF_NETLINK;

    msghdr hdr{};
    hdr.msg_name = &from;
    hdr.msg_namelen = sizeof(from);
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;

    // We only do multiple recvs if we have a MULTI type message
    bool done = true;
    do
    {
        ssize_t recvd = recvmsg(sock, &hdr, 0);
        if (recvd < 0)
        {
            throw std::system_error(errno, std::generic_category(),
                                    "netlink recvmsg");
        }
        if (recvd == 0)
        {
            throw std::runtime_error("netlink recvmsg: Got empty payload");
        }

        std::string_view msgs(buf.data(), recvd);
        do
        {
            processMsg(msgs, done, cb);
        } while (!done && !msgs.empty());

        if (done && !msgs.empty())
        {
            throw std::runtime_error("Extra unprocessed netlink messages");
        }
    } while (!done);
}

static void requestSend(int sock, void* data, size_t size)
{
    sockaddr_nl dst{};
    dst.nl_family = AF_NETLINK;

    iovec iov{};
    iov.iov_base = data;
    iov.iov_len = size;

    msghdr hdr{};
    hdr.msg_name = reinterpret_cast<sockaddr*>(&dst);
    hdr.msg_namelen = sizeof(dst);
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;

    if (sendmsg(sock, &hdr, 0) < 0)
    {
        throw std::system_error(errno, std::generic_category(),
                                "netlink sendmsg");
    }
}

static int newRequestSocket(int protocol)
{
    int sock = socket(AF_NETLINK, SOCK_RAW, protocol);
    if (sock < 0)
    {
        throw std::system_error(errno, std::generic_category(), "netlink open");
    }

    sockaddr_nl local{};
    local.nl_family = AF_NETLINK;
    int r = bind(sock, reinterpret_cast<sockaddr*>(&local), sizeof(local));
    if (r < 0)
    {
        close(sock);
        throw std::system_error(errno, std::generic_category(), "netlink bind");
    }

    return sock;
}

void performRequest(int protocol, void* data, size_t size,
                    const ReceiveCallback& cb)
{
    Descriptor sock(newRequestSocket(protocol));
    requestSend(sock(), data, size);
    receive(sock(), cb);
}

} // namespace detail

std::tuple<rtattr, std::string_view> extractRtAttr(std::string_view& data)
{
    auto hdr = stdplus::raw::copyFrom<rtattr>(data);
    if (hdr.rta_len < RTA_LENGTH(0))
    {
        throw std::runtime_error("Invalid rtattr length");
    }
    if (data.size() < hdr.rta_len)
    {
        throw std::runtime_error("Not enough data for rtattr");
    }
    auto attr = data.substr(RTA_LENGTH(0), hdr.rta_len - RTA_LENGTH(0));
    data.remove_prefix(RTA_ALIGN(hdr.rta_len));
    return {hdr, attr};
}

} // namespace netlink
} // namespace network
} // namespace phosphor
