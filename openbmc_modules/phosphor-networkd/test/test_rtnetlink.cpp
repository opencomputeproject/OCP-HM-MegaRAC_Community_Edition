#include "mock_network_manager.hpp"
#include "mock_syscall.hpp"
#include "rtnetlink_server.hpp"
#include "types.hpp"

#include <linux/rtnetlink.h>
#include <net/if.h>

#include <chrono>
#include <functional>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <gtest/gtest.h>

namespace phosphor
{

namespace network
{
sdbusplus::bus::bus bus(sdbusplus::bus::new_default());
std::unique_ptr<MockManager> manager = nullptr;
extern std::unique_ptr<Timer> refreshObjectTimer;
extern std::unique_ptr<Timer> restartTimer;
EventPtr eventPtr = nullptr;

/** @brief refresh the network objects. */
void refreshObjects()
{
    if (manager)
    {
        manager->createChildObjects();
    }
}

void initializeTimers()
{
    refreshObjectTimer = std::make_unique<Timer>(
        sdeventplus::Event::get_default(), std::bind(refreshObjects));
}

class TestRtNetlink : public testing::Test
{

  public:
    std::string confDir;
    phosphor::Descriptor smartSock;

    TestRtNetlink()
    {
        manager =
            std::make_unique<MockManager>(bus, "/xyz/openbmc_test/bcd", "/tmp");
        sd_event* events;
        sd_event_default(&events);
        eventPtr.reset(events);
        events = nullptr;
        setConfDir();
        initializeTimers();
        createNetLinkSocket();
        bus.attach_event(eventPtr.get(), SD_EVENT_PRIORITY_NORMAL);
        rtnetlink::Server svr(eventPtr, smartSock);
    }

    ~TestRtNetlink()
    {
        if (confDir.empty())
        {
            fs::remove_all(confDir);
        }
    }

    void setConfDir()
    {
        char tmp[] = "/tmp/NetworkManager.XXXXXX";
        confDir = mkdtemp(tmp);
        manager->setConfDir(confDir);
    }

    void createNetLinkSocket()
    {
        // RtnetLink socket
        auto fd = socket(PF_NETLINK, SOCK_RAW | SOCK_NONBLOCK, NETLINK_ROUTE);
        smartSock.set(fd);
    }
};

TEST_F(TestRtNetlink, WithSingleInterface)
{
    using namespace std::chrono;
    mock_clear();
    // Adds the following ip in the getifaddrs list.
    mock_addIF("igb5", 6);
    mock_addIP("igb5", "127.0.0.1", "255.255.255.128", IFF_UP | IFF_RUNNING);
    constexpr auto BUFSIZE = 4096;
    std::array<char, BUFSIZE> msgBuf = {0};

    // point the header and the msg structure pointers into the buffer.
    auto nlMsg = reinterpret_cast<nlmsghdr*>(msgBuf.data());
    // Length of message
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(rtmsg));
    nlMsg->nlmsg_type = RTM_GETADDR;
    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    nlMsg->nlmsg_seq = 0;
    nlMsg->nlmsg_pid = getpid();

    EXPECT_EQ(false, manager->hasInterface("igb5"));
    // Send the request
    send(smartSock(), nlMsg, nlMsg->nlmsg_len, 0);

    int i = 3;
    while (i--)
    {
        // wait for timer to expire
        std::this_thread::sleep_for(std::chrono::milliseconds(refreshTimeout));
        sd_event_run(eventPtr.get(), 10);
    };

    EXPECT_EQ(true, manager->hasInterface("igb5"));
}

} // namespace network
} // namespace phosphor
