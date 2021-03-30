#include "neighbor.hpp"
#include "util.hpp"

#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/ethernet.h>

#include <cstring>
#include <stdexcept>
#include <stdplus/raw.hpp>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace phosphor
{
namespace network
{
namespace detail
{

TEST(ParseNeighbor, NotNeighborType)
{
    nlmsghdr hdr{};
    hdr.nlmsg_type = RTM_NEWLINK;
    NeighborFilter filter;

    std::vector<NeighborInfo> neighbors;
    EXPECT_THROW(parseNeighbor(filter, hdr, "", neighbors), std::runtime_error);
    EXPECT_EQ(0, neighbors.size());
}

TEST(ParseNeighbor, SmallMsg)
{
    nlmsghdr hdr{};
    hdr.nlmsg_type = RTM_NEWNEIGH;
    std::string data = "1";
    NeighborFilter filter;

    std::vector<NeighborInfo> neighbors;
    EXPECT_THROW(parseNeighbor(filter, hdr, data, neighbors),
                 std::runtime_error);
    EXPECT_EQ(0, neighbors.size());
}

TEST(ParseNeighbor, NoAttrs)
{
    nlmsghdr hdr{};
    hdr.nlmsg_type = RTM_NEWNEIGH;
    ndmsg msg{};
    msg.ndm_ifindex = 1;
    msg.ndm_state = NUD_REACHABLE;
    std::string data;
    data.append(reinterpret_cast<char*>(&msg), sizeof(msg));
    NeighborFilter filter;

    std::vector<NeighborInfo> neighbors;
    EXPECT_THROW(parseNeighbor(filter, hdr, data, neighbors),
                 std::runtime_error);
    EXPECT_EQ(0, neighbors.size());
}

TEST(ParseNeighbor, NoAddress)
{
    nlmsghdr hdr{};
    hdr.nlmsg_type = RTM_NEWNEIGH;
    ndmsg msg{};
    msg.ndm_ifindex = 1;
    msg.ndm_state = NUD_REACHABLE;
    ether_addr mac = {{0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa}};
    rtattr lladdr{};
    lladdr.rta_len = RTA_LENGTH(sizeof(mac));
    lladdr.rta_type = NDA_LLADDR;
    char lladdrbuf[RTA_ALIGN(lladdr.rta_len)];
    std::memset(lladdrbuf, '\0', sizeof(lladdrbuf));
    std::memcpy(lladdrbuf, &lladdr, sizeof(lladdr));
    std::memcpy(RTA_DATA(lladdrbuf), &mac, sizeof(mac));
    std::string data;
    data.append(reinterpret_cast<char*>(&msg), sizeof(msg));
    data.append(reinterpret_cast<char*>(&lladdrbuf), sizeof(lladdrbuf));
    NeighborFilter filter;

    std::vector<NeighborInfo> neighbors;
    EXPECT_THROW(parseNeighbor(filter, hdr, data, neighbors),
                 std::runtime_error);
    EXPECT_EQ(0, neighbors.size());
}

TEST(ParseNeighbor, NoMAC)
{
    nlmsghdr hdr{};
    hdr.nlmsg_type = RTM_NEWNEIGH;
    ndmsg msg{};
    msg.ndm_family = AF_INET;
    msg.ndm_state = NUD_PERMANENT;
    msg.ndm_ifindex = 1;
    in_addr addr;
    ASSERT_EQ(1, inet_pton(msg.ndm_family, "192.168.10.1", &addr));
    rtattr dst{};
    dst.rta_len = RTA_LENGTH(sizeof(addr));
    dst.rta_type = NDA_DST;
    char dstbuf[RTA_ALIGN(dst.rta_len)];
    std::memset(dstbuf, '\0', sizeof(dstbuf));
    std::memcpy(dstbuf, &dst, sizeof(dst));
    std::memcpy(RTA_DATA(dstbuf), &addr, sizeof(addr));
    std::string data;
    data.append(reinterpret_cast<char*>(&msg), sizeof(msg));
    data.append(reinterpret_cast<char*>(&dstbuf), sizeof(dstbuf));
    NeighborFilter filter;

    std::vector<NeighborInfo> neighbors;
    parseNeighbor(filter, hdr, data, neighbors);
    EXPECT_EQ(1, neighbors.size());
    EXPECT_EQ(msg.ndm_ifindex, neighbors[0].interface);
    EXPECT_EQ(msg.ndm_state, neighbors[0].state);
    EXPECT_FALSE(neighbors[0].mac);
    EXPECT_TRUE(
        stdplus::raw::equal(addr, std::get<in_addr>(neighbors[0].address)));
}

TEST(ParseNeighbor, FilterInterface)
{
    nlmsghdr hdr{};
    hdr.nlmsg_type = RTM_NEWNEIGH;
    ndmsg msg{};
    msg.ndm_family = AF_INET;
    msg.ndm_state = NUD_PERMANENT;
    msg.ndm_ifindex = 2;
    in_addr addr;
    ASSERT_EQ(1, inet_pton(msg.ndm_family, "192.168.10.1", &addr));
    rtattr dst{};
    dst.rta_len = RTA_LENGTH(sizeof(addr));
    dst.rta_type = NDA_DST;
    char dstbuf[RTA_ALIGN(dst.rta_len)];
    std::memset(dstbuf, '\0', sizeof(dstbuf));
    std::memcpy(dstbuf, &dst, sizeof(dst));
    std::memcpy(RTA_DATA(dstbuf), &addr, sizeof(addr));
    std::string data;
    data.append(reinterpret_cast<char*>(&msg), sizeof(msg));
    data.append(reinterpret_cast<char*>(&dstbuf), sizeof(dstbuf));
    NeighborFilter filter;

    std::vector<NeighborInfo> neighbors;
    filter.interface = 1;
    parseNeighbor(filter, hdr, data, neighbors);
    EXPECT_EQ(0, neighbors.size());
    filter.interface = 2;
    parseNeighbor(filter, hdr, data, neighbors);
    EXPECT_EQ(1, neighbors.size());
    EXPECT_EQ(msg.ndm_ifindex, neighbors[0].interface);
    EXPECT_EQ(msg.ndm_state, neighbors[0].state);
    EXPECT_FALSE(neighbors[0].mac);
    EXPECT_TRUE(
        stdplus::raw::equal(addr, std::get<in_addr>(neighbors[0].address)));
}

TEST(ParseNeighbor, FilterState)
{
    nlmsghdr hdr{};
    hdr.nlmsg_type = RTM_NEWNEIGH;
    ndmsg msg{};
    msg.ndm_family = AF_INET;
    msg.ndm_state = NUD_PERMANENT;
    msg.ndm_ifindex = 2;
    in_addr addr;
    ASSERT_EQ(1, inet_pton(msg.ndm_family, "192.168.10.1", &addr));
    rtattr dst{};
    dst.rta_len = RTA_LENGTH(sizeof(addr));
    dst.rta_type = NDA_DST;
    char dstbuf[RTA_ALIGN(dst.rta_len)];
    std::memset(dstbuf, '\0', sizeof(dstbuf));
    std::memcpy(dstbuf, &dst, sizeof(dst));
    std::memcpy(RTA_DATA(dstbuf), &addr, sizeof(addr));
    std::string data;
    data.append(reinterpret_cast<char*>(&msg), sizeof(msg));
    data.append(reinterpret_cast<char*>(&dstbuf), sizeof(dstbuf));
    NeighborFilter filter;

    std::vector<NeighborInfo> neighbors;
    filter.state = NUD_NOARP;
    parseNeighbor(filter, hdr, data, neighbors);
    EXPECT_EQ(0, neighbors.size());
    filter.state = NUD_PERMANENT | NUD_NOARP;
    parseNeighbor(filter, hdr, data, neighbors);
    EXPECT_EQ(1, neighbors.size());
    EXPECT_EQ(msg.ndm_ifindex, neighbors[0].interface);
    EXPECT_EQ(msg.ndm_state, neighbors[0].state);
    EXPECT_FALSE(neighbors[0].mac);
    EXPECT_TRUE(
        stdplus::raw::equal(addr, std::get<in_addr>(neighbors[0].address)));
}

TEST(ParseNeighbor, Full)
{
    nlmsghdr hdr{};
    hdr.nlmsg_type = RTM_NEWNEIGH;
    ndmsg msg{};
    msg.ndm_family = AF_INET6;
    msg.ndm_state = NUD_NOARP;
    msg.ndm_ifindex = 1;
    ether_addr mac = {{0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa}};
    rtattr lladdr{};
    lladdr.rta_len = RTA_LENGTH(sizeof(mac));
    lladdr.rta_type = NDA_LLADDR;
    char lladdrbuf[RTA_ALIGN(lladdr.rta_len)];
    std::memset(lladdrbuf, '\0', sizeof(lladdrbuf));
    std::memcpy(lladdrbuf, &lladdr, sizeof(lladdr));
    std::memcpy(RTA_DATA(lladdrbuf), &mac, sizeof(mac));
    in6_addr addr;
    ASSERT_EQ(1, inet_pton(msg.ndm_family, "fd00::1", &addr));
    rtattr dst{};
    dst.rta_len = RTA_LENGTH(sizeof(addr));
    dst.rta_type = NDA_DST;
    char dstbuf[RTA_ALIGN(dst.rta_len)];
    std::memset(dstbuf, '\0', sizeof(dstbuf));
    std::memcpy(dstbuf, &dst, sizeof(dst));
    std::memcpy(RTA_DATA(dstbuf), &addr, sizeof(addr));
    std::string data;
    data.append(reinterpret_cast<char*>(&msg), sizeof(msg));
    data.append(reinterpret_cast<char*>(&lladdrbuf), sizeof(lladdrbuf));
    data.append(reinterpret_cast<char*>(&dstbuf), sizeof(dstbuf));
    NeighborFilter filter;

    std::vector<NeighborInfo> neighbors;
    parseNeighbor(filter, hdr, data, neighbors);
    EXPECT_EQ(1, neighbors.size());
    EXPECT_EQ(msg.ndm_ifindex, neighbors[0].interface);
    EXPECT_EQ(msg.ndm_state, neighbors[0].state);
    EXPECT_TRUE(neighbors[0].mac);
    EXPECT_TRUE(stdplus::raw::equal(mac, *neighbors[0].mac));
    EXPECT_TRUE(
        stdplus::raw::equal(addr, std::get<in6_addr>(neighbors[0].address)));
}

} // namespace detail
} // namespace network
} // namespace phosphor
