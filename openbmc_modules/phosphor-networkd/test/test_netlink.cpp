#include "mock_syscall.hpp"
#include "netlink.hpp"
#include "util.hpp"

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <cstring>
#include <stdexcept>
#include <stdplus/raw.hpp>
#include <string_view>

#include <gtest/gtest.h>

namespace phosphor
{
namespace network
{
namespace netlink
{
namespace detail
{

TEST(ExtractMsgs, TooSmall)
{
    const char buf[] = {'1'};
    static_assert(sizeof(buf) < sizeof(nlmsghdr));
    std::string_view data(buf, sizeof(buf));

    size_t cbCalls = 0;
    auto cb = [&](const nlmsghdr&, std::string_view) { cbCalls++; };
    bool done = true;
    EXPECT_THROW(processMsg(data, done, cb), std::runtime_error);
    EXPECT_EQ(1, data.size());
    EXPECT_EQ(0, cbCalls);
    EXPECT_TRUE(done);
}

TEST(ExtractMsgs, SmallAttrLen)
{
    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(0) - 1;
    std::string_view data(reinterpret_cast<char*>(&hdr), NLMSG_SPACE(0));

    size_t cbCalls = 0;
    auto cb = [&](const nlmsghdr&, std::string_view) { cbCalls++; };
    bool done = true;
    EXPECT_THROW(processMsg(data, done, cb), std::runtime_error);
    EXPECT_EQ(NLMSG_SPACE(0), data.size());
    EXPECT_EQ(0, cbCalls);
    EXPECT_TRUE(done);
}

TEST(ExtractMsgs, LargeAttrLen)
{
    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(0) + 1;
    std::string_view data(reinterpret_cast<char*>(&hdr), NLMSG_SPACE(0));

    size_t cbCalls = 0;
    auto cb = [&](const nlmsghdr&, std::string_view) { cbCalls++; };
    bool done = true;
    EXPECT_THROW(processMsg(data, done, cb), std::runtime_error);
    EXPECT_EQ(NLMSG_SPACE(0), data.size());
    EXPECT_EQ(0, cbCalls);
    EXPECT_TRUE(done);
}

TEST(ExtractMsgs, NoopMsg)
{
    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(0);
    hdr.nlmsg_type = NLMSG_NOOP;
    std::string_view data(reinterpret_cast<char*>(&hdr), NLMSG_SPACE(0));

    size_t cbCalls = 0;
    auto cb = [&](const nlmsghdr&, std::string_view) { cbCalls++; };
    bool done = true;
    processMsg(data, done, cb);
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(0, cbCalls);
    EXPECT_TRUE(done);
}

TEST(ExtractMsgs, AckMsg)
{
    nlmsgerr ack{};
    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(sizeof(ack));
    hdr.nlmsg_type = NLMSG_ERROR;
    char buf[NLMSG_ALIGN(hdr.nlmsg_len)];
    std::memcpy(buf, &hdr, sizeof(hdr));
    std::memcpy(NLMSG_DATA(buf), &ack, sizeof(ack));
    std::string_view data(reinterpret_cast<char*>(&buf), sizeof(buf));

    size_t cbCalls = 0;
    auto cb = [&](const nlmsghdr&, std::string_view) { cbCalls++; };
    bool done = true;
    processMsg(data, done, cb);
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(0, cbCalls);
    EXPECT_TRUE(done);
}

TEST(ExtractMsgs, ErrMsg)
{
    nlmsgerr err{};
    err.error = EINVAL;
    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(sizeof(err));
    hdr.nlmsg_type = NLMSG_ERROR;
    char buf[NLMSG_ALIGN(hdr.nlmsg_len)];
    std::memcpy(buf, &hdr, sizeof(hdr));
    std::memcpy(NLMSG_DATA(buf), &err, sizeof(err));
    std::string_view data(reinterpret_cast<char*>(&buf), sizeof(buf));

    size_t cbCalls = 0;
    nlmsghdr hdrOut;
    std::string_view dataOut;
    auto cb = [&](const nlmsghdr& hdr, std::string_view data) {
        hdrOut = hdr;
        dataOut = data;
        cbCalls++;
    };
    bool done = true;
    processMsg(data, done, cb);
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(1, cbCalls);
    EXPECT_TRUE(stdplus::raw::equal(hdr, hdrOut));
    EXPECT_TRUE(
        stdplus::raw::equal(err, stdplus::raw::extract<nlmsgerr>(dataOut)));
    EXPECT_EQ(0, dataOut.size());
    EXPECT_TRUE(done);
}

TEST(ExtractMsgs, DoneNoMulti)
{
    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(0);
    hdr.nlmsg_type = NLMSG_DONE;
    std::string_view data(reinterpret_cast<char*>(&hdr), NLMSG_SPACE(0));

    size_t cbCalls = 0;
    auto cb = [&](const nlmsghdr&, std::string_view) { cbCalls++; };
    bool done = true;
    EXPECT_THROW(processMsg(data, done, cb), std::runtime_error);
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(0, cbCalls);
    EXPECT_TRUE(done);
}

TEST(ExtractMsg, TwoMultiMsgs)
{
    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(0);
    hdr.nlmsg_type = RTM_NEWLINK;
    hdr.nlmsg_flags = NLM_F_MULTI;
    std::string buf;
    buf.append(reinterpret_cast<char*>(&hdr), NLMSG_SPACE(0));
    buf.append(reinterpret_cast<char*>(&hdr), NLMSG_SPACE(0));

    std::string_view data = buf;
    size_t cbCalls = 0;
    auto cb = [&](const nlmsghdr&, std::string_view) { cbCalls++; };
    bool done = true;
    processMsg(data, done, cb);
    EXPECT_EQ(NLMSG_SPACE(0), data.size());
    EXPECT_EQ(1, cbCalls);
    EXPECT_FALSE(done);

    processMsg(data, done, cb);
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(2, cbCalls);
    EXPECT_FALSE(done);
}

TEST(ExtractMsgs, MultiMsgValid)
{
    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(0);
    hdr.nlmsg_type = RTM_NEWLINK;
    hdr.nlmsg_flags = NLM_F_MULTI;
    std::string_view data(reinterpret_cast<char*>(&hdr), NLMSG_SPACE(0));

    size_t cbCalls = 0;
    auto cb = [&](const nlmsghdr&, std::string_view) { cbCalls++; };
    bool done = true;
    processMsg(data, done, cb);
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(1, cbCalls);
    EXPECT_FALSE(done);

    hdr.nlmsg_type = NLMSG_DONE;
    hdr.nlmsg_flags = 0;
    data = std::string_view(reinterpret_cast<char*>(&hdr), NLMSG_SPACE(0));
    processMsg(data, done, cb);
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(1, cbCalls);
    EXPECT_TRUE(done);
}

TEST(ExtractMsgs, MultiMsgInvalid)
{
    nlmsghdr hdr{};
    hdr.nlmsg_len = NLMSG_LENGTH(0);
    hdr.nlmsg_type = RTM_NEWLINK;
    hdr.nlmsg_flags = NLM_F_MULTI;
    std::string_view data(reinterpret_cast<char*>(&hdr), NLMSG_SPACE(0));

    size_t cbCalls = 0;
    auto cb = [&](const nlmsghdr&, std::string_view) { cbCalls++; };
    bool done = true;
    processMsg(data, done, cb);
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(1, cbCalls);
    EXPECT_FALSE(done);

    hdr.nlmsg_flags = 0;
    data = std::string_view(reinterpret_cast<char*>(&hdr), NLMSG_SPACE(0));
    EXPECT_THROW(processMsg(data, done, cb), std::runtime_error);
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(1, cbCalls);
    EXPECT_FALSE(done);
}

} // namespace detail

TEST(ExtractRtAttr, TooSmall)
{
    const char buf[] = {'1'};
    static_assert(sizeof(buf) < sizeof(rtattr));
    std::string_view data(buf, sizeof(buf));

    EXPECT_THROW(extractRtAttr(data), std::runtime_error);
    EXPECT_EQ(1, data.size());
}

TEST(ExtractRtAttr, SmallAttrLen)
{
    rtattr rta{};
    rta.rta_len = RTA_LENGTH(0) - 1;
    std::string_view data(reinterpret_cast<char*>(&rta), RTA_SPACE(0));

    EXPECT_THROW(extractRtAttr(data), std::runtime_error);
    EXPECT_EQ(RTA_SPACE(0), data.size());
}

TEST(ExtractRtAttr, LargeAttrLen)
{
    rtattr rta{};
    rta.rta_len = RTA_LENGTH(0) + 1;
    std::string_view data(reinterpret_cast<char*>(&rta), RTA_SPACE(0));

    EXPECT_THROW(extractRtAttr(data), std::runtime_error);
    EXPECT_EQ(RTA_SPACE(0), data.size());
}

TEST(ExtractRtAttr, NoData)
{
    rtattr rta{};
    rta.rta_len = RTA_LENGTH(0);
    std::string_view data(reinterpret_cast<char*>(&rta), RTA_SPACE(0));

    auto [hdr, attr] = extractRtAttr(data);
    EXPECT_EQ(0, data.size());
    EXPECT_EQ(0, attr.size());
    EXPECT_EQ(0, std::memcmp(&rta, &hdr, sizeof(rta)));
}

TEST(ExtractRtAttr, SomeData)
{
    const char attrbuf[] = "abcd";
    const char nextbuf[] = "efgh";
    rtattr rta{};
    rta.rta_len = RTA_LENGTH(sizeof(attrbuf));

    char buf[RTA_SPACE(sizeof(attrbuf)) + sizeof(nextbuf)];
    memcpy(buf, &rta, sizeof(rta));
    memcpy(RTA_DATA(buf), &attrbuf, sizeof(attrbuf));
    memcpy(buf + RTA_SPACE(sizeof(attrbuf)), &nextbuf, sizeof(nextbuf));
    std::string_view data(buf, sizeof(buf));

    auto [hdr, attr] = extractRtAttr(data);
    EXPECT_EQ(0, memcmp(&rta, &hdr, sizeof(rta)));
    EXPECT_EQ(sizeof(attrbuf), attr.size());
    EXPECT_EQ(0, memcmp(&attrbuf, attr.data(), sizeof(attrbuf)));
    EXPECT_EQ(sizeof(nextbuf), data.size());
    EXPECT_EQ(0, memcmp(&nextbuf, data.data(), sizeof(nextbuf)));
}

class PerformRequest : public testing::Test
{
  public:
    void doLinkDump(size_t ifs)
    {
        mock_clear();
        for (size_t i = 0; i < ifs; ++i)
        {
            mock_addIF("eth" + std::to_string(i), 1 + i);
        }

        size_t cbCalls = 0;
        auto cb = [&](const nlmsghdr&, std::string_view) { cbCalls++; };
        ifinfomsg msg{};
        netlink::performRequest(NETLINK_ROUTE, RTM_GETLINK, NLM_F_DUMP, msg,
                                cb);
        EXPECT_EQ(ifs, cbCalls);
    }
};

TEST_F(PerformRequest, NoResponse)
{
    doLinkDump(0);
}

TEST_F(PerformRequest, SingleResponse)
{
    doLinkDump(1);
}

TEST_F(PerformRequest, MultiResponse)
{
    doLinkDump(3);
}

TEST_F(PerformRequest, MultiMsg)
{
    doLinkDump(1000);
}

} // namespace netlink
} // namespace network
} // namespace phosphor
