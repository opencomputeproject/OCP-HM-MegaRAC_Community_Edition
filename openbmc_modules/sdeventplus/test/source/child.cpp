#include <cerrno>
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <sdeventplus/event.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/source/child.hpp>
#include <sdeventplus/test/sdevent.hpp>
#include <sys/wait.h>
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

class ChildTest : public testing::Test
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

TEST_F(ChildTest, ConstructSuccess)
{
    const pid_t pid = 50;
    const int options = WEXITED;

    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
    sd_event_child_handler_t handler;
    EXPECT_CALL(mock, sd_event_add_child(expected_event, testing::_, pid,
                                         options, testing::_, nullptr))
        .WillOnce(DoAll(SetArgPointee<1>(expected_source), SaveArg<4>(&handler),
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
    const siginfo_t* return_si;
    Child::Callback callback = [&](Child&, const siginfo_t* si) {
        return_si = si;
        completions++;
    };
    Child child(*event, pid, options, std::move(callback));
    EXPECT_FALSE(callback);
    EXPECT_NE(&child, userdata);
    EXPECT_EQ(0, completions);

    const siginfo_t* expected_si = reinterpret_cast<siginfo_t*>(865);
    EXPECT_EQ(0, handler(nullptr, expected_si, userdata));
    EXPECT_EQ(1, completions);
    EXPECT_EQ(expected_si, return_si);

    child.set_callback(std::bind([]() {}));
    EXPECT_EQ(0, handler(nullptr, expected_si, userdata));
    EXPECT_EQ(1, completions);

    expect_destruct();
    destroy(userdata);
}

TEST_F(ChildTest, ConstructError)
{
    const pid_t pid = 50;
    const int options = WEXITED;

    EXPECT_CALL(mock, sd_event_add_child(expected_event, testing::_, pid,
                                         options, testing::_, nullptr))
        .WillOnce(Return(-EINVAL));
    int completions = 0;
    Child::Callback callback = [&completions](Child&, const siginfo_t*) {
        completions++;
    };
    EXPECT_THROW(Child(*event, pid, options, std::move(callback)),
                 SdEventError);
    EXPECT_TRUE(callback);
    EXPECT_EQ(0, completions);
}

class ChildMethodTest : public ChildTest
{
  protected:
    std::unique_ptr<Child> child;
    sd_event_destroy_t destroy;
    void* userdata;

    void SetUp()
    {
        const pid_t pid = 50;
        const int options = WEXITED;

        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_add_child(expected_event, testing::_, pid,
                                             options, testing::_, nullptr))
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
        child = std::make_unique<Child>(*event, pid, options,
                                        [](Child&, const siginfo_t*) {});
    }

    void TearDown()
    {
        expect_destruct();
        child.reset();
        destroy(userdata);
    }
};

TEST_F(ChildMethodTest, Copy)
{
    EXPECT_CALL(mock, sd_event_ref(expected_event))
        .WillOnce(Return(expected_event));
    EXPECT_CALL(mock, sd_event_source_ref(expected_source))
        .WillOnce(Return(expected_source));
    auto child2 = std::make_unique<Child>(*child);
    {
        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_source_ref(expected_source))
            .WillOnce(Return(expected_source));
        Child child3(*child);

        expect_destruct();
        EXPECT_CALL(mock, sd_event_ref(expected_event))
            .WillOnce(Return(expected_event));
        EXPECT_CALL(mock, sd_event_source_ref(expected_source))
            .WillOnce(Return(expected_source));
        *child2 = child3;

        expect_destruct();
    }

    // Delete the original child
    child2.swap(child);
    expect_destruct();
    child2.reset();

    // Make sure our new copy can still access data
    child->set_callback(nullptr);
}

TEST_F(ChildMethodTest, GetPidSuccess)
{
    const pid_t pid = 32;
    EXPECT_CALL(mock,
                sd_event_source_get_child_pid(expected_source, testing::_))
        .WillOnce(DoAll(SetArgPointee<1>(pid), Return(0)));
    EXPECT_EQ(pid, child->get_pid());
}

TEST_F(ChildMethodTest, GetPidError)
{
    EXPECT_CALL(mock,
                sd_event_source_get_child_pid(expected_source, testing::_))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(child->get_pid(), SdEventError);
}

} // namespace
} // namespace source
} // namespace sdeventplus
