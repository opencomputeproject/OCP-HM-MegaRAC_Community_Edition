#include <cerrno>
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <sdeventplus/event.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/test/sdevent.hpp>
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

class EventTest : public testing::Test
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

TEST_F(EventTest, DeferConstruct)
{
    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
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
    sd_event_handler_t handler;
    EXPECT_CALL(mock, sd_event_add_defer(expected_event, testing::_, testing::_,
                                         nullptr))
        .WillOnce(DoAll(SetArgPointee<1>(expected_source), SaveArg<2>(&handler),
                        Return(0)));
    int completions = 0;
    EventBase::Callback callback = [&completions](EventBase&) {
        completions++;
    };
    Defer defer(*event, std::move(callback));
    EXPECT_NE(&defer, userdata);
    EXPECT_FALSE(callback);
    EXPECT_EQ(0, completions);

    EXPECT_EQ(0, handler(nullptr, userdata));
    EXPECT_EQ(1, completions);

    defer.set_callback(std::bind([]() {}));
    EXPECT_EQ(0, handler(nullptr, userdata));
    EXPECT_EQ(1, completions);

    expect_destruct();
    destroy(userdata);
}

TEST_F(EventTest, PostConstruct)
{
    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
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
    sd_event_handler_t handler;
    EXPECT_CALL(mock, sd_event_add_post(expected_event, testing::_, testing::_,
                                        nullptr))
        .WillOnce(DoAll(SetArgPointee<1>(expected_source), SaveArg<2>(&handler),
                        Return(0)));
    int completions = 0;
    EventBase::Callback callback = [&completions](EventBase&) {
        completions++;
    };
    Post post(*event, std::move(callback));
    EXPECT_NE(&post, userdata);
    EXPECT_FALSE(callback);
    EXPECT_EQ(0, completions);

    EXPECT_EQ(0, handler(nullptr, userdata));
    EXPECT_EQ(1, completions);

    expect_destruct();
    destroy(userdata);
}

TEST_F(EventTest, ExitConstruct)
{
    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
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
    sd_event_handler_t handler;
    EXPECT_CALL(mock, sd_event_add_exit(expected_event, testing::_, testing::_,
                                        nullptr))
        .WillOnce(DoAll(SetArgPointee<1>(expected_source), SaveArg<2>(&handler),
                        Return(0)));
    int completions = 0;
    EventBase::Callback callback = [&completions](EventBase&) {
        completions++;
    };
    Exit exit(*event, std::move(callback));
    EXPECT_NE(&exit, userdata);
    EXPECT_FALSE(callback);
    EXPECT_EQ(0, completions);

    EXPECT_EQ(0, handler(nullptr, userdata));
    EXPECT_EQ(1, completions);

    expect_destruct();
    destroy(userdata);
}

TEST_F(EventTest, ConstructFailure)
{
    EXPECT_CALL(mock, sd_event_add_defer(expected_event, testing::_, testing::_,
                                         nullptr))
        .WillOnce(Return(-EINVAL));
    int completions = 0;
    EventBase::Callback callback = [&completions](EventBase&) {
        completions++;
    };
    EXPECT_THROW(Defer(*event, std::move(callback)), SdEventError);
    EXPECT_TRUE(callback);
    EXPECT_EQ(0, completions);
}

TEST_F(EventTest, CopyConstruct)
{
    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
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
    sd_event_handler_t handler;
    EXPECT_CALL(mock, sd_event_add_exit(expected_event, testing::_, testing::_,
                                        nullptr))
        .WillOnce(DoAll(SetArgPointee<1>(expected_source), SaveArg<2>(&handler),
                        Return(0)));
    auto exit = std::make_unique<Exit>(*event, [](EventBase&) {});

    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
    EXPECT_CALL(mock, sd_event_source_ref(expected_source))
        .WillOnce(Return(expected_source));
    auto exit2 = std::make_unique<Exit>(*exit);
    {
        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_source_ref(expected_source))
            .WillOnce(Return(expected_source));
        Exit exit3(*exit);

        expect_destruct();
        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_source_ref(expected_source))
            .WillOnce(Return(expected_source));
        *exit2 = exit3;

        expect_destruct();
    }

    // Delete the original exit
    expect_destruct();
    exit.reset();

    // Make sure our new copy can still access data
    exit2->set_callback(nullptr);
    expect_destruct();
    exit2.reset();
    destroy(userdata);
}

} // namespace
} // namespace source
} // namespace sdeventplus
