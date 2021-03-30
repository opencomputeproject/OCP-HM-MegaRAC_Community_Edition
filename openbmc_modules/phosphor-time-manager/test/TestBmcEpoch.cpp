#include "config.h"

#include "bmc_epoch.hpp"
#include "types.hpp"

#include <memory>
#include <sdbusplus/bus.hpp>

#include <gtest/gtest.h>

namespace phosphor
{
namespace time
{

using namespace std::chrono;

class TestBmcEpoch : public testing::Test
{
  public:
    sdbusplus::bus::bus bus;
    sd_event* event;
    std::unique_ptr<BmcEpoch> bmcEpoch;

    TestBmcEpoch() : bus(sdbusplus::bus::new_default())
    {
        // BmcEpoch requires sd_event to init
        sd_event_default(&event);
        bus.attach_event(event, SD_EVENT_PRIORITY_NORMAL);
        bmcEpoch = std::make_unique<BmcEpoch>(bus, OBJPATH_BMC);
    }

    ~TestBmcEpoch()
    {
        bus.detach_event();
        sd_event_unref(event);
    }

    // Proxies for BmcEpoch's private members and functions
    Mode getTimeMode()
    {
        return bmcEpoch->timeMode;
    }
    void setTimeMode(Mode mode)
    {
        bmcEpoch->timeMode = mode;
    }
    void triggerTimeChange()
    {
        bmcEpoch->onTimeChange(nullptr, -1, 0, bmcEpoch.get());
    }
};

TEST_F(TestBmcEpoch, empty)
{
    // Default mode is MANUAL
    EXPECT_EQ(Mode::Manual, getTimeMode());
}

TEST_F(TestBmcEpoch, getElapsed)
{
    auto t1 = bmcEpoch->elapsed();
    EXPECT_NE(0, t1);
    auto t2 = bmcEpoch->elapsed();
    EXPECT_GE(t2, t1);
}

TEST_F(TestBmcEpoch, setElapsedOK)
{
    // TODO: setting time will call sd-bus functions and it will fail on host
    // if we have gmock for sdbusplus::bus, we can test setElapsed.
    // But for now we can not test it
}

} // namespace time
} // namespace phosphor
