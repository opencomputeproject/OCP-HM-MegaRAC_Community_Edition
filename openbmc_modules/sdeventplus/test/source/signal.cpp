#include <cerrno>
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <sdeventplus/event.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/source/signal.hpp>
#include <sdeventplus/test/sdevent.hpp>
#include <signal.h>
#include <systemd/sd-event.h>
#include <type_traits>
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

class SignalTest : public testing::Test
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

    void expect_destruct()
    {
        EXPECT_CALL(mock, sd_event_source_unref(expected_source))
            .WillOnce(Return(nullptr));
        EXPECT_CALL(mock, sd_event_unref(expected_event))
            .WillOnce(Return(nullptr));
    }
};

TEST_F(SignalTest, ConstructSuccess)
{
    const int sig = SIGALRM;

    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
    sd_event_signal_handler_t handler;
    EXPECT_CALL(mock, sd_event_add_signal(expected_event, testing::_, sig,
                                          testing::_, nullptr))
        .WillOnce(DoAll(SetArgPointee<1>(expected_source), SaveArg<3>(&handler),
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
    int completions = 0;
    const struct signalfd_siginfo* return_si;
    Signal::Callback callback = [&](Signal&,
                                    const struct signalfd_siginfo* si) {
        return_si = si;
        completions++;
    };
    Signal signal(*event, sig, std::move(callback));
    EXPECT_FALSE(callback);
    EXPECT_NE(&signal, userdata);
    EXPECT_EQ(0, completions);

    const struct signalfd_siginfo* expected_si =
        reinterpret_cast<struct signalfd_siginfo*>(865);
    EXPECT_EQ(0, handler(nullptr, expected_si, userdata));
    EXPECT_EQ(1, completions);
    EXPECT_EQ(expected_si, return_si);

    signal.set_callback(std::bind([]() {}));
    EXPECT_EQ(0, handler(nullptr, expected_si, userdata));
    EXPECT_EQ(1, completions);

    expect_destruct();
    destroy(userdata);
}

TEST_F(SignalTest, ConstructError)
{
    const int sig = SIGALRM;

    EXPECT_CALL(mock, sd_event_add_signal(expected_event, testing::_, sig,
                                          testing::_, nullptr))
        .WillOnce(Return(-EINVAL));
    int completions = 0;
    Signal::Callback callback = [&completions](Signal&,
                                               const struct signalfd_siginfo*) {
        completions++;
    };
    EXPECT_THROW(Signal(*event, sig, std::move(callback)), SdEventError);
    EXPECT_TRUE(callback);
    EXPECT_EQ(0, completions);
}

class SignalMethodTest : public SignalTest
{
  protected:
    std::unique_ptr<Signal> signal;
    sd_event_destroy_t destroy;
    void* userdata;

    void SetUp()
    {
        const int sig = SIGINT;

        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_add_signal(expected_event, testing::_, sig,
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
        signal = std::make_unique<Signal>(
            *event, sig, [](Signal&, const struct signalfd_siginfo*) {});
    }

    void TearDown()
    {
        expect_destruct();
        signal.reset();
        destroy(userdata);
    }
};

TEST_F(SignalMethodTest, Copy)
{
    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
    EXPECT_CALL(mock, sd_event_source_ref(expected_source))
        .WillOnce(Return(expected_source));
    auto signal2 = std::make_unique<Signal>(*signal);
    {
        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_source_ref(expected_source))
            .WillOnce(Return(expected_source));
        Signal signal3(*signal);

        expect_destruct();
        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_source_ref(expected_source))
            .WillOnce(Return(expected_source));
        *signal2 = signal3;

        expect_destruct();
    }

    // Delete the original signal
    signal2.swap(signal);
    expect_destruct();
    signal2.reset();

    // Make sure our new copy can still access data
    signal->set_callback(nullptr);
}

TEST_F(SignalMethodTest, GetSignalSuccess)
{
    const int sig = SIGTERM;
    EXPECT_CALL(mock, sd_event_source_get_signal(expected_source))
        .WillOnce(Return(sig));
    EXPECT_EQ(sig, signal->get_signal());
}

TEST_F(SignalMethodTest, GetSignalError)
{
    EXPECT_CALL(mock, sd_event_source_get_signal(expected_source))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(signal->get_signal(), SdEventError);
}

} // namespace
} // namespace source
} // namespace sdeventplus
