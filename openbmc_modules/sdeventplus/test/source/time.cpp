#include <cerrno>
#include <chrono>
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/source/time.hpp>
#include <sdeventplus/test/sdevent.hpp>
#include <systemd/sd-event.h>
#include <time.h>
#include <utility>

namespace sdeventplus
{
namespace source
{
namespace
{

using testing::DoAll;
using testing::Return;
using testing::ReturnPointee;
using testing::SaveArg;
using testing::SetArgPointee;

using UniqueEvent = std::unique_ptr<Event, std::function<void(Event*)>>;

class TimeTest : public testing::Test
{
  protected:
    testing::StrictMock<test::SdEventMock> mock;
    sd_event_source* const expected_source =
        reinterpret_cast<sd_event_source*>(1234);
    sd_event* const expected_event = reinterpret_cast<sd_event*>(2345);
    UniqueEvent event = make_event(expected_event);

    UniqueEvent make_event(sd_event* event)
    {
        auto deleter = [this, event](Event* e) {
            EXPECT_CALL(this->mock, sd_event_unref(event))
                .WillOnce(Return(nullptr));
            delete e;
        };
        return UniqueEvent(new Event(event, std::false_type(), &mock), deleter);
    }

    void expect_time_destroy(sd_event* event, sd_event_source* source)
    {
        EXPECT_CALL(mock, sd_event_source_unref(source))
            .WillOnce(Return(nullptr));
        EXPECT_CALL(mock, sd_event_unref(event)).WillOnce(Return(nullptr));
    }
};

TEST_F(TimeTest, ConstructSuccess)
{
    constexpr ClockId id = ClockId::RealTime;
    const Time<id>::TimePoint expected_time(std::chrono::seconds{2});
    const Time<id>::Accuracy expected_accuracy(std::chrono::milliseconds{50});
    Time<id>::TimePoint saved_time;
    Time<id>::Callback callback = [&saved_time](Time<id>&,
                                                Time<id>::TimePoint time) {
        saved_time = time;
    };

    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
    sd_event_time_handler_t handler;
    EXPECT_CALL(mock,
                sd_event_add_time(expected_event, testing::_, CLOCK_REALTIME,
                                  2000000, 50000, testing::_, nullptr))
        .WillOnce(DoAll(SetArgPointee<1>(expected_source), SaveArg<5>(&handler),
                        Return(0)));
    sd_event_destroy_t destroy;
    void* userdata;
    {
        testing::InSequence seq;
        EXPECT_CALL(mock, sd_event_source_set_destroy_callback(expected_source,
                                                               testing::_))
            .WillOnce(DoAll(SaveArg<1>(&destroy), Return(0)));
        EXPECT_CALL(mock,
                    sd_event_source_set_userdata(expected_source, testing::_))
            .WillOnce(DoAll(SaveArg<1>(&userdata), Return(nullptr)));
        EXPECT_CALL(mock, sd_event_source_get_userdata(expected_source))
            .WillRepeatedly(ReturnPointee(&userdata));
    }
    Time<id> time(*event, expected_time, expected_accuracy,
                  std::move(callback));
    EXPECT_FALSE(callback);
    EXPECT_NE(&time, userdata);
    EXPECT_EQ(expected_event, time.get_event().get());
    EXPECT_EQ(expected_source, time.get());

    EXPECT_EQ(0, handler(nullptr, 2000100, userdata));
    EXPECT_EQ(Time<id>::TimePoint(std::chrono::microseconds(2000100)),
              saved_time);

    time.set_callback(std::bind([]() {}));
    EXPECT_EQ(0, handler(nullptr, 0, userdata));
    EXPECT_EQ(Time<id>::TimePoint(std::chrono::microseconds(2000100)),
              saved_time);

    expect_time_destroy(expected_event, expected_source);
    destroy(userdata);
}

TEST_F(TimeTest, ConstructError)
{
    constexpr ClockId id = ClockId::Monotonic;
    const Time<id>::TimePoint expected_time(std::chrono::seconds{2});
    const Time<id>::Accuracy expected_accuracy(std::chrono::milliseconds{50});
    Time<id>::Callback callback = [](Time<id>&, Time<id>::TimePoint) {};

    EXPECT_CALL(mock,
                sd_event_add_time(expected_event, testing::_, CLOCK_MONOTONIC,
                                  2000000, 50000, testing::_, nullptr))
        .WillOnce(Return(-ENOSYS));
    EXPECT_THROW(
        Time<id>(*event, expected_time, expected_accuracy, std::move(callback)),
        SdEventError);
    EXPECT_TRUE(callback);
}

class TimeMethodTest : public TimeTest
{
  protected:
    static constexpr ClockId id = ClockId::BootTime;
    std::unique_ptr<Time<id>> time;
    sd_event_destroy_t destroy;
    void* userdata;

    void SetUp()
    {
        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_add_time(expected_event, testing::_,
                                            CLOCK_BOOTTIME, 2000000, 50000,
                                            testing::_, nullptr))
            .WillOnce(DoAll(SetArgPointee<1>(expected_source), Return(0)));
        {
            testing::InSequence seq;
            EXPECT_CALL(mock, sd_event_source_set_destroy_callback(
                                  expected_source, testing::_))
                .WillOnce(DoAll(SaveArg<1>(&destroy), Return(0)));
            EXPECT_CALL(
                mock, sd_event_source_set_userdata(expected_source, testing::_))
                .WillOnce(DoAll(SaveArg<1>(&userdata), Return(nullptr)));
            EXPECT_CALL(mock, sd_event_source_get_userdata(expected_source))
                .WillRepeatedly(ReturnPointee(&userdata));
        }
        time = std::make_unique<Time<id>>(
            *event, Time<id>::TimePoint(std::chrono::seconds{2}),
            std::chrono::milliseconds{50},
            [](Time<id>&, Time<id>::TimePoint) {});
    }

    void TearDown()
    {
        expect_time_destroy(expected_event, expected_source);
        time.reset();
        destroy(userdata);
    }
};

TEST_F(TimeMethodTest, Copy)
{
    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
    EXPECT_CALL(mock, sd_event_source_ref(expected_source))
        .WillOnce(Return(expected_source));
    auto time2 = std::make_unique<Time<id>>(*time);
    {
        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_source_ref(expected_source))
            .WillOnce(Return(expected_source));
        Time<id> time3(*time);

        expect_time_destroy(expected_event, expected_source);
        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_source_ref(expected_source))
            .WillOnce(Return(expected_source));
        *time2 = time3;

        expect_time_destroy(expected_event, expected_source);
    }

    // Delete the original time
    time2.swap(time);
    expect_time_destroy(expected_event, expected_source);
    time2.reset();

    // Make sure our new copy can still access data
    time->set_callback(nullptr);
}

TEST_F(TimeMethodTest, SetTimeSuccess)
{
    EXPECT_CALL(mock, sd_event_source_set_time(expected_source, 1000000))
        .WillOnce(Return(0));
    time->set_time(Time<id>::TimePoint(std::chrono::seconds{1}));
}

TEST_F(TimeMethodTest, SetTimeError)
{
    EXPECT_CALL(mock, sd_event_source_set_time(expected_source, 1000000))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(time->set_time(Time<id>::TimePoint(std::chrono::seconds{1})),
                 SdEventError);
}

TEST_F(TimeMethodTest, GetTimeSuccess)
{
    EXPECT_CALL(mock, sd_event_source_get_time(expected_source, testing::_))
        .WillOnce(DoAll(SetArgPointee<1>(10), Return(0)));
    EXPECT_EQ(Time<id>::TimePoint(std::chrono::microseconds{10}),
              time->get_time());
}

TEST_F(TimeMethodTest, GetTimeError)
{
    EXPECT_CALL(mock, sd_event_source_get_time(expected_source, testing::_))
        .WillOnce(Return(-ENOSYS));
    EXPECT_THROW(time->get_time(), SdEventError);
}

TEST_F(TimeMethodTest, SetAccuracySuccess)
{
    EXPECT_CALL(mock,
                sd_event_source_set_time_accuracy(expected_source, 5000000))
        .WillOnce(Return(0));
    time->set_accuracy(std::chrono::seconds{5});
}

TEST_F(TimeMethodTest, SetAccuracyError)
{
    EXPECT_CALL(mock,
                sd_event_source_set_time_accuracy(expected_source, 5000000))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(time->set_accuracy(std::chrono::seconds{5}), SdEventError);
}

TEST_F(TimeMethodTest, GetAccuracySuccess)
{
    EXPECT_CALL(mock,
                sd_event_source_get_time_accuracy(expected_source, testing::_))
        .WillOnce(DoAll(SetArgPointee<1>(1000), Return(0)));
    EXPECT_EQ(std::chrono::milliseconds{1}, time->get_accuracy());
}

TEST_F(TimeMethodTest, GetAccuracyError)
{
    EXPECT_CALL(mock,
                sd_event_source_get_time_accuracy(expected_source, testing::_))
        .WillOnce(Return(-ENOSYS));
    EXPECT_THROW(time->get_accuracy(), SdEventError);
}

} // namespace
} // namespace source
} // namespace sdeventplus
