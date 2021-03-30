#include <cerrno>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/test/sdevent.hpp>
#include <systemd/sd-event.h>
#include <type_traits>
#include <utility>

namespace sdeventplus
{
namespace
{

using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;

class ClockTest : public testing::Test
{
  protected:
    testing::StrictMock<test::SdEventMock> mock;
    sd_event* const expected_event = reinterpret_cast<sd_event*>(1234);
};

TEST_F(ClockTest, CopyEvent)
{
    Event event(expected_event, std::false_type(), &mock);

    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
    Clock<ClockId::RealTime> clock(event);
    EXPECT_CALL(mock, sd_event_now(expected_event, CLOCK_REALTIME, testing::_))
        .WillOnce(DoAll(SetArgPointee<2>(2000000), Return(0)));
    EXPECT_EQ(Clock<ClockId::RealTime>::time_point(std::chrono::seconds{2}),
              clock.now());

    EXPECT_CALL(mock, sd_event_unref(expected_event))
        .Times(2)
        .WillRepeatedly(Return(nullptr));
}

TEST_F(ClockTest, MoveEvent)
{
    Event event(expected_event, std::false_type(), &mock);

    Clock<ClockId::Monotonic> clock(std::move(event));
    EXPECT_CALL(mock, sd_event_now(expected_event, CLOCK_MONOTONIC, testing::_))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(clock.now(), SdEventError);

    EXPECT_CALL(mock, sd_event_unref(expected_event)).WillOnce(Return(nullptr));
}

} // namespace
} // namespace sdeventplus
