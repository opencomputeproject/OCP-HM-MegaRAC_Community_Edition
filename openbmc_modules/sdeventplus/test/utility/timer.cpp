#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/test/sdevent.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <stdexcept>
#include <systemd/sd-event.h>

namespace sdeventplus
{
namespace utility
{
namespace
{

constexpr ClockId testClock = ClockId::Monotonic;

using std::chrono::microseconds;
using std::chrono::milliseconds;
using testing::DoAll;
using testing::Return;
using testing::ReturnPointee;
using testing::SaveArg;
using testing::SetArgPointee;
using TestTimer = Timer<testClock>;

ssize_t event_ref_times = 0;

ACTION(EventRef)
{
    event_ref_times++;
}

ACTION(EventUnref)
{
    ASSERT_LT(0, event_ref_times);
    event_ref_times--;
}

class TimerTest : public testing::Test
{
  protected:
    testing::StrictMock<test::SdEventMock> mock;
    sd_event* const expected_event = reinterpret_cast<sd_event*>(1234);
    sd_event_source* const expected_source =
        reinterpret_cast<sd_event_source*>(2345);
    sd_event_source* const expected_source2 =
        reinterpret_cast<sd_event_source*>(3456);
    const milliseconds interval{134};
    const milliseconds starting_time{10};
    const milliseconds starting_time2{30};
    sd_event_time_handler_t handler = nullptr;
    void* handler_userdata;
    sd_event_destroy_t handler_destroy;
    std::unique_ptr<Event> event;
    std::unique_ptr<TestTimer> timer;
    std::function<void()> callback;

    void expectNow(microseconds ret)
    {
        EXPECT_CALL(mock,
                    sd_event_now(expected_event,
                                 static_cast<clockid_t>(testClock), testing::_))
            .WillOnce(DoAll(SetArgPointee<2>(ret.count()), Return(0)));
    }

    void expectSetTime(microseconds time)
    {
        EXPECT_CALL(mock,
                    sd_event_source_set_time(expected_source, time.count()))
            .WillOnce(Return(0));
    }

    void expectSetEnabled(source::Enabled enabled)
    {
        EXPECT_CALL(mock, sd_event_source_set_enabled(
                              expected_source, static_cast<int>(enabled)))
            .WillOnce(Return(0));
    }

    void expectGetEnabled(source::Enabled enabled)
    {
        EXPECT_CALL(mock,
                    sd_event_source_get_enabled(expected_source, testing::_))
            .WillOnce(
                DoAll(SetArgPointee<1>(static_cast<int>(enabled)), Return(0)));
    }

    void resetTimer()
    {
        if (timer)
        {
            timer.reset();
            handler_destroy(handler_userdata);
        }
    }

    void expireTimer()
    {
        const milliseconds new_time(90);
        expectNow(new_time);
        expectSetTime(new_time + interval);
        EXPECT_EQ(0, handler(nullptr, 0, handler_userdata));
        EXPECT_TRUE(timer->hasExpired());
        EXPECT_EQ(interval, timer->getInterval());
    }

    void SetUp()
    {
        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillRepeatedly(DoAll(EventRef(), Return(expected_event)));
        EXPECT_CALL(mock, sd_event_unref(expected_event))
            .WillRepeatedly(DoAll(EventUnref(), Return(nullptr)));
        event = std::make_unique<Event>(expected_event, &mock);
        EXPECT_CALL(mock, sd_event_source_unref(expected_source))
            .WillRepeatedly(Return(nullptr));
        EXPECT_CALL(mock, sd_event_source_set_destroy_callback(expected_source,
                                                               testing::_))
            .WillRepeatedly(DoAll(SaveArg<1>(&handler_destroy), Return(0)));
        EXPECT_CALL(mock,
                    sd_event_source_set_userdata(expected_source, testing::_))
            .WillRepeatedly(
                DoAll(SaveArg<1>(&handler_userdata), Return(nullptr)));
        EXPECT_CALL(mock, sd_event_source_get_userdata(expected_source))
            .WillRepeatedly(ReturnPointee(&handler_userdata));

        // Having a callback proxy allows us to update the test callback
        // dynamically, without changing it inside the timer
        auto runCallback = [&](TestTimer&) {
            if (callback)
            {
                callback();
            }
        };
        expectNow(starting_time);
        EXPECT_CALL(mock, sd_event_add_time(
                              expected_event, testing::_,
                              static_cast<clockid_t>(testClock),
                              microseconds(starting_time + interval).count(),
                              1000, testing::_, nullptr))
            .WillOnce(DoAll(SetArgPointee<1>(expected_source),
                            SaveArg<5>(&handler), Return(0)));
        expectSetEnabled(source::Enabled::On);
        timer = std::make_unique<TestTimer>(*event, runCallback, interval);
        EXPECT_EQ(expected_event, timer->get_event().get());
    }

    void TearDown()
    {
        resetTimer();
        event.reset();
        EXPECT_EQ(0, event_ref_times);
    }
};

TEST_F(TimerTest, NoCallback)
{
    resetTimer();
    expectNow(starting_time);
    EXPECT_CALL(
        mock, sd_event_add_time(expected_event, testing::_,
                                static_cast<clockid_t>(testClock),
                                microseconds(starting_time + interval).count(),
                                1000, testing::_, nullptr))
        .WillOnce(DoAll(SetArgPointee<1>(expected_source), SaveArg<5>(&handler),
                        Return(0)));
    expectSetEnabled(source::Enabled::On);
    timer = std::make_unique<TestTimer>(*event, nullptr, interval);

    expectNow(starting_time);
    expectSetTime(starting_time + interval);
    EXPECT_EQ(0, handler(nullptr, 0, handler_userdata));
}

TEST_F(TimerTest, NoInterval)
{
    resetTimer();
    expectNow(starting_time);
    EXPECT_CALL(mock, sd_event_add_time(expected_event, testing::_,
                                        static_cast<clockid_t>(testClock),
                                        microseconds(starting_time).count(),
                                        1000, testing::_, nullptr))
        .WillOnce(DoAll(SetArgPointee<1>(expected_source), SaveArg<5>(&handler),
                        Return(0)));
    expectSetEnabled(source::Enabled::Off);
    timer = std::make_unique<TestTimer>(*event, nullptr);

    EXPECT_EQ(std::nullopt, timer->getInterval());
    EXPECT_THROW(timer->setEnabled(true), std::runtime_error);
}

TEST_F(TimerTest, NewTimer)
{
    EXPECT_FALSE(timer->hasExpired());
    EXPECT_EQ(interval, timer->getInterval());
}

TEST_F(TimerTest, IsEnabled)
{
    expectGetEnabled(source::Enabled::On);
    EXPECT_TRUE(timer->isEnabled());
    expectGetEnabled(source::Enabled::Off);
    EXPECT_FALSE(timer->isEnabled());
}

TEST_F(TimerTest, GetRemainingDisabled)
{
    expectGetEnabled(source::Enabled::Off);
    EXPECT_THROW(timer->getRemaining(), std::runtime_error);
}

TEST_F(TimerTest, GetRemainingNegative)
{
    milliseconds now(675), end(453);
    expectGetEnabled(source::Enabled::On);
    EXPECT_CALL(mock, sd_event_source_get_time(expected_source, testing::_))
        .WillOnce(
            DoAll(SetArgPointee<1>(microseconds(end).count()), Return(0)));
    expectNow(now);
    EXPECT_EQ(milliseconds(0), timer->getRemaining());
}

TEST_F(TimerTest, GetRemainingPositive)
{
    milliseconds now(453), end(675);
    expectGetEnabled(source::Enabled::On);
    EXPECT_CALL(mock, sd_event_source_get_time(expected_source, testing::_))
        .WillOnce(
            DoAll(SetArgPointee<1>(microseconds(end).count()), Return(0)));
    expectNow(now);
    EXPECT_EQ(end - now, timer->getRemaining());
}

TEST_F(TimerTest, SetEnabled)
{
    expectSetEnabled(source::Enabled::On);
    timer->setEnabled(true);
    EXPECT_FALSE(timer->hasExpired());
    // Value should always be passed through regardless of current state
    expectSetEnabled(source::Enabled::On);
    timer->setEnabled(true);
    EXPECT_FALSE(timer->hasExpired());

    expectSetEnabled(source::Enabled::Off);
    timer->setEnabled(false);
    EXPECT_FALSE(timer->hasExpired());
    // Value should always be passed through regardless of current state
    expectSetEnabled(source::Enabled::Off);
    timer->setEnabled(false);
    EXPECT_FALSE(timer->hasExpired());
}

TEST_F(TimerTest, SetEnabledUnsetTimer)
{
    // Force the timer to become unset
    expectSetEnabled(source::Enabled::Off);
    timer->restart(std::nullopt);

    // Setting an interval should not update the timer directly
    timer->setInterval(milliseconds(90));

    expectSetEnabled(source::Enabled::Off);
    timer->setEnabled(false);
    EXPECT_THROW(timer->setEnabled(true), std::runtime_error);
}

TEST_F(TimerTest, SetEnabledOneshot)
{
    // Timer effectively becomes oneshot if it gets initialized but has
    // the interval removed
    timer->setInterval(std::nullopt);

    expectSetEnabled(source::Enabled::Off);
    timer->setEnabled(false);
    expectSetEnabled(source::Enabled::On);
    timer->setEnabled(true);
}

TEST_F(TimerTest, SetRemaining)
{
    const milliseconds now(90), remaining(30);
    expectNow(now);
    expectSetTime(now + remaining);
    timer->setRemaining(remaining);
    EXPECT_EQ(interval, timer->getInterval());
    EXPECT_FALSE(timer->hasExpired());
}

TEST_F(TimerTest, ResetRemaining)
{
    const milliseconds now(90);
    expectNow(now);
    expectSetTime(now + interval);
    timer->resetRemaining();
    EXPECT_EQ(interval, timer->getInterval());
    EXPECT_FALSE(timer->hasExpired());
}

TEST_F(TimerTest, SetInterval)
{
    const milliseconds new_interval(40);
    timer->setInterval(new_interval);
    EXPECT_EQ(new_interval, timer->getInterval());
    EXPECT_FALSE(timer->hasExpired());
}

TEST_F(TimerTest, SetIntervalEmpty)
{
    timer->setInterval(std::nullopt);
    EXPECT_EQ(std::nullopt, timer->getInterval());
    EXPECT_FALSE(timer->hasExpired());
}

TEST_F(TimerTest, CallbackHappensLast)
{
    const milliseconds new_time(90);
    expectNow(new_time);
    expectSetTime(new_time + interval);
    callback = [&]() {
        EXPECT_TRUE(timer->hasExpired());
        expectSetEnabled(source::Enabled::On);
        timer->setEnabled(true);
        timer->clearExpired();
        timer->setInterval(std::nullopt);
    };
    EXPECT_EQ(0, handler(nullptr, 0, handler_userdata));
    EXPECT_FALSE(timer->hasExpired());
    EXPECT_EQ(std::nullopt, timer->getInterval());
    expectSetEnabled(source::Enabled::On);
    timer->setEnabled(true);
}

TEST_F(TimerTest, CallbackOneshot)
{
    // Make sure we try a one shot so we can test the callback
    // correctly
    timer->setInterval(std::nullopt);

    expectSetEnabled(source::Enabled::Off);
    callback = [&]() {
        EXPECT_TRUE(timer->hasExpired());
        EXPECT_THROW(timer->setEnabled(true), std::runtime_error);
        timer->setInterval(interval);
    };
    EXPECT_EQ(0, handler(nullptr, 0, handler_userdata));
    EXPECT_THROW(timer->setEnabled(true), std::runtime_error);
}

TEST_F(TimerTest, CallbackMove)
{
    size_t called = 0;
    callback = [&]() { ++called; };

    expectNow(starting_time2);
    sd_event_destroy_t local_destroy;
    EXPECT_CALL(mock, sd_event_source_set_destroy_callback(expected_source2,
                                                           testing::_))
        .WillOnce(DoAll(SaveArg<1>(&local_destroy), Return(0)));
    void* local_userdata;
    EXPECT_CALL(mock,
                sd_event_source_set_userdata(expected_source2, testing::_))
        .WillOnce(DoAll(SaveArg<1>(&local_userdata), Return(nullptr)));
    EXPECT_CALL(mock, sd_event_source_get_userdata(expected_source2))
        .WillRepeatedly(ReturnPointee(&local_userdata));
    EXPECT_CALL(mock, sd_event_add_time(expected_event, testing::_,
                                        static_cast<clockid_t>(testClock),
                                        microseconds(starting_time2).count(),
                                        1000, testing::_, nullptr))
        .WillOnce(DoAll(SetArgPointee<1>(expected_source2), Return(0)));
    EXPECT_CALL(mock, sd_event_source_unref(expected_source2))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(mock,
                sd_event_source_set_enabled(
                    expected_source2, static_cast<int>(source::Enabled::Off)))
        .WillOnce(Return(0));
    TestTimer local_timer(*event, nullptr);

    // Move assign
    local_timer = std::move(*timer);
    local_destroy(local_userdata);
    timer.reset();

    // Move construct
    timer = std::make_unique<TestTimer>(std::move(local_timer));

    // handler_userdata should have been updated and the callback should work
    const milliseconds new_time(90);
    expectNow(new_time);
    expectSetTime(new_time + interval);
    EXPECT_EQ(0, handler(nullptr, 0, handler_userdata));
    EXPECT_EQ(1, called);

    // update the callback and make sure it still works
    timer->set_callback(std::bind([]() {}));
    expectNow(new_time);
    expectSetTime(new_time + interval);
    EXPECT_EQ(0, handler(nullptr, 0, handler_userdata));
    EXPECT_EQ(1, called);
}

TEST_F(TimerTest, SetValuesExpiredTimer)
{
    const milliseconds new_time(90);
    expectNow(new_time);
    expectSetTime(new_time + interval);
    EXPECT_EQ(0, handler(nullptr, 0, handler_userdata));
    EXPECT_TRUE(timer->hasExpired());
    EXPECT_EQ(interval, timer->getInterval());

    // Timer should remain expired unless clearExpired() or reset()
    expectSetEnabled(source::Enabled::On);
    timer->setEnabled(true);
    EXPECT_TRUE(timer->hasExpired());
    expectNow(milliseconds(20));
    expectSetTime(milliseconds(50));
    timer->setRemaining(milliseconds(30));
    EXPECT_TRUE(timer->hasExpired());
    timer->setInterval(milliseconds(10));
    EXPECT_TRUE(timer->hasExpired());
    expectNow(milliseconds(20));
    expectSetTime(milliseconds(30));
    timer->resetRemaining();
    EXPECT_TRUE(timer->hasExpired());

    timer->clearExpired();
    EXPECT_FALSE(timer->hasExpired());
}

TEST_F(TimerTest, Restart)
{
    expireTimer();

    const milliseconds new_interval(471);
    expectNow(starting_time);
    expectSetTime(starting_time + new_interval);
    expectSetEnabled(source::Enabled::On);
    timer->restart(new_interval);
    EXPECT_FALSE(timer->hasExpired());
    EXPECT_EQ(new_interval, timer->getInterval());
    expectSetEnabled(source::Enabled::On);
    timer->setEnabled(true);
}

TEST_F(TimerTest, RestartEmpty)
{
    expireTimer();

    expectSetEnabled(source::Enabled::Off);
    timer->restart(std::nullopt);
    EXPECT_FALSE(timer->hasExpired());
    EXPECT_EQ(std::nullopt, timer->getInterval());
    EXPECT_THROW(timer->setEnabled(true), std::runtime_error);
}

TEST_F(TimerTest, RestartOnce)
{
    expireTimer();

    const milliseconds remaining(471);
    expectNow(starting_time);
    expectSetTime(starting_time + remaining);
    expectSetEnabled(source::Enabled::On);
    timer->restartOnce(remaining);
    EXPECT_FALSE(timer->hasExpired());
    EXPECT_EQ(std::nullopt, timer->getInterval());
    expectSetEnabled(source::Enabled::On);
    timer->setEnabled(true);
}

} // namespace
} // namespace utility
} // namespace sdeventplus
