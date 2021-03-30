#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <sdeventplus/event.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/test/sdevent.hpp>
#include <type_traits>

namespace sdeventplus
{
namespace
{

using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;

class EventTest : public testing::Test
{
  protected:
    testing::StrictMock<test::SdEventMock> mock;
    sd_event* const expected_event = reinterpret_cast<sd_event*>(1234);
};

TEST_F(EventTest, NewEventRef)
{
    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
    Event event(expected_event, &mock);
    EXPECT_EQ(&mock, event.getSdEvent());
    EXPECT_EQ(expected_event, event.get());

    EXPECT_CALL(mock, sd_event_unref(expected_event)).WillOnce(Return(nullptr));
}

TEST_F(EventTest, NewEventNoRef)
{
    Event event(expected_event, std::false_type(), &mock);
    EXPECT_EQ(&mock, event.getSdEvent());
    EXPECT_EQ(expected_event, event.get());

    EXPECT_CALL(mock, sd_event_unref(expected_event)).WillOnce(Return(nullptr));
}

TEST_F(EventTest, CopyEventNoOwn)
{
    Event event(expected_event, std::false_type(), &mock);
    EXPECT_EQ(&mock, event.getSdEvent());
    EXPECT_EQ(expected_event, event.get());

    Event event_noown(event, sdeventplus::internal::NoOwn());
    EXPECT_EQ(&mock, event_noown.getSdEvent());
    EXPECT_EQ(expected_event, event_noown.get());

    EXPECT_CALL(mock, sd_event_unref(expected_event)).WillOnce(Return(nullptr));
}

TEST_F(EventTest, CopyEventNoOwnCopy)
{
    Event event(expected_event, std::false_type(), &mock);
    EXPECT_EQ(&mock, event.getSdEvent());
    EXPECT_EQ(expected_event, event.get());

    Event event_noown(event, sdeventplus::internal::NoOwn());
    EXPECT_EQ(&mock, event_noown.getSdEvent());
    EXPECT_EQ(expected_event, event_noown.get());

    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
    Event event2(event_noown);
    EXPECT_EQ(&mock, event2.getSdEvent());
    EXPECT_EQ(expected_event, event2.get());

    EXPECT_CALL(mock, sd_event_unref(expected_event))
        .WillOnce(Return(nullptr))
        .WillOnce(Return(nullptr));
}

TEST_F(EventTest, GetNewEvent)
{
    EXPECT_CALL(mock, sd_event_new(testing::_))
        .WillOnce(DoAll(SetArgPointee<0>(expected_event), Return(0)));
    Event event = Event::get_new(&mock);
    EXPECT_EQ(&mock, event.getSdEvent());
    EXPECT_EQ(expected_event, event.get());

    EXPECT_CALL(mock, sd_event_unref(expected_event)).WillOnce(Return(nullptr));
}

TEST_F(EventTest, GetNewEventFail)
{
    EXPECT_CALL(mock, sd_event_new(testing::_)).WillOnce(Return(-EINVAL));
    EXPECT_THROW(Event::get_new(&mock), SdEventError);
}

TEST_F(EventTest, GetDefaultEvent)
{
    EXPECT_CALL(mock, sd_event_default(testing::_))
        .WillOnce(DoAll(SetArgPointee<0>(expected_event), Return(0)));
    Event event = Event::get_default(&mock);
    EXPECT_EQ(&mock, event.getSdEvent());
    EXPECT_EQ(expected_event, event.get());

    EXPECT_CALL(mock, sd_event_unref(expected_event)).WillOnce(Return(nullptr));
}

TEST_F(EventTest, GetDefaultEventFail)
{
    EXPECT_CALL(mock, sd_event_default(testing::_)).WillOnce(Return(-EINVAL));
    EXPECT_THROW(Event::get_default(&mock), SdEventError);
}

class EventMethodTest : public EventTest
{
  protected:
    std::unique_ptr<Event> event;

    void SetUp()
    {
        event =
            std::make_unique<Event>(expected_event, std::false_type(), &mock);
    }

    void TearDown()
    {
        EXPECT_CALL(mock, sd_event_unref(expected_event))
            .WillOnce(Return(nullptr));
    }
};

TEST_F(EventMethodTest, PrepareSuccessNone)
{
    EXPECT_CALL(mock, sd_event_prepare(expected_event)).WillOnce(Return(0));
    EXPECT_EQ(0, event->prepare());
}

TEST_F(EventMethodTest, PrepareSuccessReady)
{
    const int events_ready = 10;
    EXPECT_CALL(mock, sd_event_prepare(expected_event))
        .WillOnce(Return(events_ready));
    EXPECT_EQ(events_ready, event->prepare());
}

TEST_F(EventMethodTest, PrepareInternalError)
{
    EXPECT_CALL(mock, sd_event_prepare(expected_event))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(event->prepare(), SdEventError);
}

TEST_F(EventMethodTest, WaitSuccessNone)
{
    const std::chrono::microseconds timeout{20};
    EXPECT_CALL(mock, sd_event_wait(expected_event, timeout.count()))
        .WillOnce(Return(0));
    EXPECT_EQ(0, event->wait(timeout));
}

TEST_F(EventMethodTest, WaitSuccessReady)
{
    const int events_ready = 10;
    EXPECT_CALL(mock, sd_event_wait(expected_event, static_cast<uint64_t>(-1)))
        .WillOnce(Return(events_ready));
    EXPECT_EQ(events_ready, event->wait(std::nullopt));
}

TEST_F(EventMethodTest, WaitInternalError)
{
    EXPECT_CALL(mock, sd_event_wait(expected_event, static_cast<uint64_t>(-1)))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(event->wait(std::nullopt), SdEventError);
}

TEST_F(EventMethodTest, DispatchInitial)
{
    EXPECT_CALL(mock, sd_event_dispatch(expected_event)).WillOnce(Return(0));
    EXPECT_EQ(0, event->dispatch());
}

TEST_F(EventMethodTest, DispatchDone)
{
    const int done_code = 10;
    EXPECT_CALL(mock, sd_event_dispatch(expected_event))
        .WillOnce(Return(done_code));
    EXPECT_EQ(done_code, event->dispatch());
}

TEST_F(EventMethodTest, DispatchInternalError)
{
    EXPECT_CALL(mock, sd_event_dispatch(expected_event))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(event->dispatch(), SdEventError);
}

TEST_F(EventMethodTest, RunSuccessNone)
{
    const std::chrono::microseconds timeout{20};
    EXPECT_CALL(mock, sd_event_run(expected_event, timeout.count()))
        .WillOnce(Return(0));
    EXPECT_EQ(0, event->run(timeout));
}

TEST_F(EventMethodTest, RunSuccessReady)
{
    const int events_ready = 10;
    EXPECT_CALL(mock, sd_event_run(expected_event, static_cast<uint64_t>(-1)))
        .WillOnce(Return(events_ready));
    EXPECT_EQ(events_ready, event->run(std::nullopt));
}

TEST_F(EventMethodTest, RunInternalError)
{
    EXPECT_CALL(mock, sd_event_run(expected_event, static_cast<uint64_t>(-1)))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(event->run(std::nullopt), SdEventError);
}

TEST_F(EventMethodTest, LoopSuccess)
{
    EXPECT_CALL(mock, sd_event_loop(expected_event)).WillOnce(Return(0));
    EXPECT_EQ(0, event->loop());
}

TEST_F(EventMethodTest, LoopUserError)
{
    const int user_error = 10;
    EXPECT_CALL(mock, sd_event_loop(expected_event))
        .WillOnce(Return(user_error));
    EXPECT_EQ(user_error, event->loop());
}

TEST_F(EventMethodTest, LoopInternalError)
{
    EXPECT_CALL(mock, sd_event_loop(expected_event)).WillOnce(Return(-EINVAL));
    EXPECT_THROW(event->loop(), SdEventError);
}

TEST_F(EventMethodTest, ExitSuccess)
{
    EXPECT_CALL(mock, sd_event_exit(expected_event, 0)).WillOnce(Return(2));
    event->exit(0);
    EXPECT_CALL(mock, sd_event_exit(expected_event, 0)).WillOnce(Return(0));
    event->exit(0);
    EXPECT_CALL(mock, sd_event_exit(expected_event, 10)).WillOnce(Return(0));
    event->exit(10);
}

TEST_F(EventMethodTest, ExitInternalError)
{
    EXPECT_CALL(mock, sd_event_exit(expected_event, 5))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(event->exit(5), SdEventError);
}

TEST_F(EventMethodTest, GetExitCodeSuccess)
{
    EXPECT_CALL(mock, sd_event_get_exit_code(expected_event, testing::_))
        .WillOnce(DoAll(SetArgPointee<1>(1), Return(0)));
    EXPECT_EQ(1, event->get_exit_code());

    EXPECT_CALL(mock, sd_event_get_exit_code(expected_event, testing::_))
        .WillOnce(DoAll(SetArgPointee<1>(0), Return(2)));
    EXPECT_EQ(0, event->get_exit_code());
}

TEST_F(EventMethodTest, GetExitCodeError)
{
    EXPECT_CALL(mock, sd_event_get_exit_code(expected_event, testing::_))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(event->get_exit_code(), SdEventError);
}

TEST_F(EventMethodTest, GetWatchdogSuccess)
{
    EXPECT_CALL(mock, sd_event_get_watchdog(expected_event))
        .WillOnce(Return(0));
    EXPECT_FALSE(event->get_watchdog());

    EXPECT_CALL(mock, sd_event_get_watchdog(expected_event))
        .WillOnce(Return(2));
    EXPECT_TRUE(event->get_watchdog());
}

TEST_F(EventMethodTest, GetWatchdogError)
{
    EXPECT_CALL(mock, sd_event_get_watchdog(expected_event))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(event->get_watchdog(), SdEventError);
}

TEST_F(EventMethodTest, SetWatchdogSuccess)
{
    // Disable
    EXPECT_CALL(mock, sd_event_set_watchdog(expected_event, false))
        .WillOnce(Return(0));
    EXPECT_FALSE(event->set_watchdog(false));

    // Enable but not supported
    EXPECT_CALL(mock, sd_event_set_watchdog(expected_event, true))
        .WillOnce(Return(0));
    EXPECT_FALSE(event->set_watchdog(true));

    // Enabled and supported
    EXPECT_CALL(mock, sd_event_set_watchdog(expected_event, true))
        .WillOnce(Return(2));
    EXPECT_TRUE(event->set_watchdog(true));
}

TEST_F(EventMethodTest, SetWatchdogError)
{
    EXPECT_CALL(mock, sd_event_set_watchdog(expected_event, 1))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(event->set_watchdog(1), SdEventError);
}

} // namespace
} // namespace sdeventplus
