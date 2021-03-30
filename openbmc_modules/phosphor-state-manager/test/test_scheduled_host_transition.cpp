#include "scheduled_host_transition.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <sdeventplus/event.hpp>
#include <xyz/openbmc_project/ScheduledTime/error.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace phosphor
{
namespace state
{
namespace manager
{

using namespace std::chrono;
using InvalidTimeError =
    sdbusplus::xyz::openbmc_project::ScheduledTime::Error::InvalidTime;
using HostTransition =
    sdbusplus::xyz::openbmc_project::State::server::ScheduledHostTransition;

class TestScheduledHostTransition : public testing::Test
{
  public:
    sdeventplus::Event event;
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus::bus mockedBus = sdbusplus::get_mocked_new(&sdbusMock);
    ScheduledHostTransition scheduledHostTransition;

    TestScheduledHostTransition() :
        event(sdeventplus::Event::get_default()),
        scheduledHostTransition(mockedBus, "", event)
    {
        // Empty
    }

    seconds getCurrentTime()
    {
        return scheduledHostTransition.getTime();
    }

    bool isTimerEnabled()
    {
        return scheduledHostTransition.timer.isEnabled();
    }

    void bmcTimeChange()
    {
        scheduledHostTransition.handleTimeUpdates();
    }
};

TEST_F(TestScheduledHostTransition, disableHostTransition)
{
    EXPECT_EQ(scheduledHostTransition.scheduledTime(0), 0);
    EXPECT_FALSE(isTimerEnabled());
}

TEST_F(TestScheduledHostTransition, invalidScheduledTime)
{
    // scheduled time is 1 min earlier than current time
    uint64_t schTime =
        static_cast<uint64_t>((getCurrentTime() - seconds(60)).count());
    EXPECT_THROW(scheduledHostTransition.scheduledTime(schTime),
                 InvalidTimeError);
}

TEST_F(TestScheduledHostTransition, validScheduledTime)
{
    // scheduled time is 1 min later than current time
    uint64_t schTime =
        static_cast<uint64_t>((getCurrentTime() + seconds(60)).count());
    EXPECT_EQ(scheduledHostTransition.scheduledTime(schTime), schTime);
    EXPECT_TRUE(isTimerEnabled());
}

TEST_F(TestScheduledHostTransition, hostTransitionStatus)
{
    // set requested transition to be on
    scheduledHostTransition.scheduledTransition(Transition::On);
    EXPECT_EQ(scheduledHostTransition.scheduledTransition(), Transition::On);
    // set requested transition to be off
    scheduledHostTransition.scheduledTransition(Transition::Off);
    EXPECT_EQ(scheduledHostTransition.scheduledTransition(), Transition::Off);
}

TEST_F(TestScheduledHostTransition, bmcTimeChangeWithDisabledHostTransition)
{
    // Disable host transition
    scheduledHostTransition.scheduledTime(0);
    bmcTimeChange();
    // Check timer
    EXPECT_FALSE(isTimerEnabled());
    // Check scheduled time
    EXPECT_EQ(scheduledHostTransition.HostTransition::scheduledTime(), 0);
}

TEST_F(TestScheduledHostTransition, bmcTimeChangeBackward)
{
    // Current time is earlier than scheduled time due to BMC time changing
    uint64_t schTime =
        static_cast<uint64_t>((getCurrentTime() + seconds(60)).count());
    // Set scheduled time, which is the same as bmc time is changed.
    // But can't use this method to write another case like
    // bmcTimeChangeForward, because set a scheduled time earlier than current
    // time will throw an error.
    scheduledHostTransition.scheduledTime(schTime);
    bmcTimeChange();
    // Check timer
    EXPECT_TRUE(isTimerEnabled());
}

} // namespace manager
} // namespace state
} // namespace phosphor
