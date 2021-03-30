#include "util.hpp"

#include <arpa/inet.h>
#include <dlfcn.h>
#include <ifaddrs.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>
#include <queue>
#include <stdexcept>
#include <stdplus/raw.hpp>
#include <string>
#include <string_view>
#include <vector>

#define MAX_IFADDRS 5

int debugging = false;

/* Data for mocking getifaddrs */
struct ifaddr_storage
{
    struct ifaddrs ifaddr;
    struct sockaddr_storage addr;
    struct sockaddr_storage mask;
    struct sockaddr_storage bcast;
} mock_ifaddr_storage[MAX_IFADDRS];

struct ifaddrs* mock_ifaddrs = nullptr;

int ifaddr_count = 0;

/* Stub library functions */
void freeifaddrs(ifaddrs* /*ifp*/)
{
    return;
}

std::map<int, std::queue<std::string>> mock_rtnetlinks;

std::map<std::string, int> mock_if_nametoindex;
std::map<int, std::string> mock_if_indextoname;
std::map<std::string, ether_addr> mock_macs;

void mock_clear()
{
    mock_ifaddrs = nullptr;
    ifaddr_count = 0;
    mock_rtnetlinks.clear();
    mock_if_nametoindex.clear();
    mock_if_indextoname.clear();
    mock_macs.clear();
}

void mock_addIF(const std::string& name, int idx, const ether_addr& mac)
{
    if (idx == 0)
    {
        throw std::invalid_argument("Bad interface index");
    }

    mock_if_nametoindex[name] = idx;
    mock_if_indextoname[idx] = name;
    mock_macs[name] = mac;
}

void mock_addIP(const char* name, const char* addr, const char* mask,
                unsigned int flags)
{
    struct ifaddrs* ifaddr = &mock_ifaddr_storage[ifaddr_count].ifaddr;

    struct sockaddr_in* in =
        reinterpret_cast<sockaddr_in*>(&mock_ifaddr_storage[ifaddr_count].addr);
    struct sockaddr_in* mask_in =
        reinterpret_cast<sockaddr_in*>(&mock_ifaddr_storage[ifaddr_count].mask);

    in->sin_family = AF_INET;
    in->sin_port = 0;
    in->sin_addr.s_addr = inet_addr(addr);

    mask_in->sin_family = AF_INET;
    mask_in->sin_port = 0;
    mask_in->sin_addr.s_addr = inet_addr(mask);

    ifaddr->ifa_next = nullptr;
    ifaddr->ifa_name = const_cast<char*>(name);
    ifaddr->ifa_flags = flags;
    ifaddr->ifa_addr = reinterpret_cast<struct sockaddr*>(in);
    ifaddr->ifa_netmask = reinterpret_cast<struct sockaddr*>(mask_in);
    ifaddr->ifa_data = nullptr;

    if (ifaddr_count > 0)
        mock_ifaddr_storage[ifaddr_count - 1].ifaddr.ifa_next = ifaddr;
    ifaddr_count++;
    mock_ifaddrs = &mock_ifaddr_storage[0].ifaddr;
}

void validateMsgHdr(const struct msghdr* msg)
{
    if (msg->msg_namelen != sizeof(sockaddr_nl))
    {
        fprintf(stderr, "bad namelen: %u\n", msg->msg_namelen);
        abort();
    }
    const auto& from = *reinterpret_cast<sockaddr_nl*>(msg->msg_name);
    if (from.nl_family != AF_NETLINK)
    {
        fprintf(stderr, "recvmsg bad family data\n");
        abort();
    }
    if (msg->msg_iovlen != 1)
    {
        fprintf(stderr, "recvmsg unsupported iov configuration\n");
        abort();
    }
}

ssize_t sendmsg_link_dump(std::queue<std::string>& msgs, std::string_view in)
{
    const ssize_t ret = in.size();
    const auto& hdrin = stdplus::raw::copyFrom<nlmsghdr>(in);
    if (hdrin.nlmsg_type != RTM_GETLINK)
    {
        return 0;
    }

    for (const auto& [name, idx] : mock_if_nametoindex)
    {
        ifinfomsg info{};
        info.ifi_index = idx;
        nlmsghdr hdr{};
        hdr.nlmsg_len = NLMSG_LENGTH(sizeof(info));
        hdr.nlmsg_type = RTM_NEWLINK;
        hdr.nlmsg_flags = NLM_F_MULTI;
        auto& out = msgs.emplace(hdr.nlmsg_len, '\0');
        memcpy(out.data(), &hdr, sizeof(hdr));
        memcpy(NLMSG_DATA(out.data()), &info, sizeof(info));
    }

    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(0);
    hdr.nlmsg_type = NLMSG_DONE;
    hdr.nlmsg_flags = NLM_F_MULTI;
    auto& out = msgs.emplace(hdr.nlmsg_len, '\0');
    memcpy(out.data(), &hdr, sizeof(hdr));
    return ret;
}

ssize_t sendmsg_ack(std::queue<std::string>& msgs, std::string_view in)
{
    nlmsgerr ack{};
    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(sizeof(ack));
    hdr.nlmsg_type = NLMSG_ERROR;
    auto& out = msgs.emplace(hdr.nlmsg_len, '\0');
    memcpy(out.data(), &hdr, sizeof(hdr));
    memcpy(NLMSG_DATA(out.data()), &ack, sizeof(ack));
    return in.size();
}

extern "C" {

int getifaddrs(ifaddrs** ifap)
{
    *ifap = mock_ifaddrs;
    if (mock_ifaddrs == nullptr)
        return -1;
    return (0);
}

unsigned if_nametoindex(const char* ifname)
{
    auto it = mock_if_nametoindex.find(ifname);
    if (it == mock_if_nametoindex.end())
    {
        errno = ENXIO;
        return 0;
    }
    return it->second;
}

char* if_indextoname(unsigned ifindex, char* ifname)
{
    auto it = mock_if_indextoname.find(ifindex);
    if (it == mock_if_indextoname.end())
    {
        errno = ENXIO;
        return NULL;
    }
    return std::strcpy(ifname, it->second.c_str());
}

int ioctl(int fd, unsigned long int request, ...)
{
    va_list vl;
    va_start(vl, request);
    void* data = va_arg(vl, void*);
    va_end(vl);

    if (request == SIOCGIFHWADDR)
    {
        auto req = reinterpret_cast<ifreq*>(data);
        auto it = mock_macs.find(req->ifr_name);
        if (it == mock_macs.end())
        {
            errno = ENXIO;
            return -1;
        }
        std::memcpy(req->ifr_hwaddr.sa_data, &it->second, sizeof(it->second));
        return 0;
    }

    static auto real_ioctl =
        reinterpret_cast<decltype(&ioctl)>(dlsym(RTLD_NEXT, "ioctl"));
    return real_ioctl(fd, request, data);
}

int socket(int domain, int type, int protocol)
{
    static auto real_socket =
        reinterpret_cast<decltype(&socket)>(dlsym(RTLD_NEXT, "socket"));
    int fd = real_socket(domain, type, protocol);
    if (domain == AF_NETLINK && !(type & SOCK_RAW))
    {
        fprintf(stderr, "Netlink sockets must be RAW\n");
        abort();
    }
    if (domain == AF_NETLINK && protocol == NETLINK_ROUTE)
    {
        mock_rtnetlinks[fd] = {};
    }
    return fd;
}

int close(int fd)
{
    auto it = mock_rtnetlinks.find(fd);
    if (it != mock_rtnetlinks.end())
    {
        mock_rtnetlinks.erase(it);
    }

    static auto real_close =
        reinterpret_cast<decltype(&close)>(dlsym(RTLD_NEXT, "close"));
    return real_close(fd);
}

ssize_t sendmsg(int sockfd, const struct msghdr* msg, int flags)
{
    auto it = mock_rtnetlinks.find(sockfd);
    if (it == mock_rtnetlinks.end())
    {
        static auto real_sendmsg =
            reinterpret_cast<decltype(&sendmsg)>(dlsym(RTLD_NEXT, "sendmsg"));
        return real_sendmsg(sockfd, msg, flags);
    }
    auto& msgs = it->second;

    validateMsgHdr(msg);
    if (!msgs.empty())
    {
        fprintf(stderr, "Unread netlink responses\n");
        abort();
    }

    ssize_t ret;
    std::string_view iov(reinterpret_cast<char*>(msg->msg_iov[0].iov_base),
                         msg->msg_iov[0].iov_len);

    ret = sendmsg_link_dump(msgs, iov);
    if (ret != 0)
    {
        return ret;
    }

    ret = sendmsg_ack(msgs, iov);
    if (ret != 0)
    {
        return ret;
    }

    errno = ENOSYS;
    return -1;
}

ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags)
{
    auto it = mock_rtnetlinks.find(sockfd);
    if (it == mock_rtnetlinks.end())
    {
        static auto real_recvmsg =
            reinterpret_cast<decltype(&recvmsg)>(dlsym(RTLD_NEXT, "recvmsg"));
        return real_recvmsg(sockfd, msg, flags);
    }
    auto& msgs = it->second;

    validateMsgHdr(msg);
    constexpr size_t required_buf_size = 8192;
    if (msg->msg_iov[0].iov_len < required_buf_size)
    {
        fprintf(stderr, "recvmsg iov too short: %zu\n",
                msg->msg_iov[0].iov_len);
        abort();
    }
    if (msgs.empty())
    {
        fprintf(stderr, "No pending netlink responses\n");
        abort();
    }

    ssize_t ret = 0;
    auto data = reinterpret_cast<char*>(msg->msg_iov[0].iov_base);
    while (!msgs.empty())
    {
        const auto& msg = msgs.front();
        if (NLMSG_ALIGN(ret) + msg.size() > required_buf_size)
        {
            break;
        }
        ret = NLMSG_ALIGN(ret);
        memcpy(data + ret, msg.data(), msg.size());
        ret += msg.size();
        msgs.pop();
    }
    return ret;
}

} // extern "C"
