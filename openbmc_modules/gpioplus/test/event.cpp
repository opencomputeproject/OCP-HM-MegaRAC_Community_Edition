#include <cerrno>
#include <cstring>
#include <gmock/gmock.h>
#include <gpioplus/event.hpp>
#include <gpioplus/test/sys.hpp>
#include <gtest/gtest.h>
#include <linux/gpio.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>

namespace gpioplus
{
namespace
{

using testing::Assign;
using testing::DoAll;
using testing::Return;
using testing::SaveArgPointee;
using testing::SetArgPointee;
using testing::WithArg;

TEST(EventFlags, EventFlagsToInt)
{
    EventFlags event_flags;
    event_flags.rising_edge = true;
    event_flags.falling_edge = true;
    EXPECT_EQ(GPIOEVENT_REQUEST_RISING_EDGE | GPIOEVENT_REQUEST_FALLING_EDGE,
              event_flags.toInt());

    event_flags.rising_edge = false;
    event_flags.falling_edge = false;
    EXPECT_EQ(0, event_flags.toInt());
}

class EventTest : public testing::Test
{
  protected:
    const int chip_fd = 1234;
    const int event_fd = 2345;
    testing::StrictMock<test::SysMock> mock;
    std::unique_ptr<Chip> chip;

    void SetUp()
    {
        EXPECT_CALL(mock, open(testing::_, testing::_))
            .WillOnce(Return(chip_fd));
        chip = std::make_unique<Chip>(0, &mock);
    }

    void TearDown()
    {
        EXPECT_CALL(mock, close(chip_fd)).WillOnce(Return(0));
        chip.reset();
    }
};

TEST_F(EventTest, ConstructSuccess)
{
    const uint32_t line_offset = 3;
    const std::string label{"test"};
    HandleFlags handle_flags(LineFlags(0));
    EventFlags event_flags;
    event_flags.rising_edge = true;
    event_flags.falling_edge = false;

    struct gpioevent_request req, ret;
    ret.fd = event_fd;
    EXPECT_CALL(mock, gpio_get_lineevent(chip_fd, testing::_))
        .WillOnce(
            DoAll(SaveArgPointee<1>(&req), SetArgPointee<1>(ret), Return(0)));
    Event event(*chip, line_offset, handle_flags, event_flags, label.c_str());

    EXPECT_EQ(event_fd, *event.getFd());
    EXPECT_EQ(line_offset, req.lineoffset);
    EXPECT_EQ(GPIOHANDLE_REQUEST_INPUT, req.handleflags);
    EXPECT_EQ(GPIOEVENT_REQUEST_RISING_EDGE, req.eventflags);
    EXPECT_EQ(label, req.consumer_label);

    EXPECT_CALL(mock, close(event_fd)).WillOnce(Return(0));
}

TEST_F(EventTest, ConstructLabelTooLong)
{
    const size_t large_size = sizeof(
        reinterpret_cast<struct gpioevent_request*>(NULL)->consumer_label);
    EXPECT_THROW(Event(*chip, 0, HandleFlags(), EventFlags(),
                       std::string(large_size, '1')),
                 std::invalid_argument);
}

TEST_F(EventTest, ConstructFailure)
{
    const uint32_t line_offset = 3;
    const std::string label{"test"};
    HandleFlags handle_flags(LineFlags(0));
    EventFlags event_flags;
    event_flags.rising_edge = false;
    event_flags.falling_edge = false;

    struct gpioevent_request req;
    EXPECT_CALL(mock, gpio_get_lineevent(chip_fd, testing::_))
        .WillOnce(DoAll(SaveArgPointee<1>(&req), Return(-EINVAL)));
    EXPECT_THROW(
        Event(*chip, line_offset, handle_flags, event_flags, label.c_str()),
        std::system_error);

    EXPECT_EQ(line_offset, req.lineoffset);
    EXPECT_EQ(GPIOHANDLE_REQUEST_INPUT, req.handleflags);
    EXPECT_EQ(0, req.eventflags);
    EXPECT_EQ(label, req.consumer_label);
}

class EventMethodTest : public EventTest
{
  protected:
    std::unique_ptr<Event> event;

    void SetUp()
    {
        EventTest::SetUp();
        struct gpioevent_request ret;
        ret.fd = event_fd;
        EXPECT_CALL(mock, gpio_get_lineevent(chip_fd, testing::_))
            .WillOnce(DoAll(SetArgPointee<1>(ret), Return(0)));
        event = std::make_unique<Event>(*chip, 0, HandleFlags(LineFlags(0)),
                                        EventFlags(), "method");
    }

    void TearDown()
    {
        EXPECT_CALL(mock, close(event_fd)).WillOnce(Return(0));
        event.reset();
        EventTest::TearDown();
    }
};

ACTION_P(WriteStruct, data)
{
    memcpy(arg0, &data, sizeof(data));
}

TEST_F(EventMethodTest, ReadSuccess)
{
    struct gpioevent_data ret;
    ret.timestamp = 5;
    ret.id = 15;
    EXPECT_CALL(mock, read(event_fd, testing::_, sizeof(struct gpioevent_data)))
        .WillOnce(DoAll(WithArg<1>(WriteStruct(ret)), Return(sizeof(ret))));
    std::optional<Event::Data> data = event->read();
    EXPECT_TRUE(data);
    EXPECT_EQ(ret.timestamp, data->timestamp.count());
    EXPECT_EQ(ret.id, data->id);
}

TEST_F(EventMethodTest, ReadAgain)
{
    EXPECT_CALL(mock, read(event_fd, testing::_, sizeof(struct gpioevent_data)))
        .WillOnce(DoAll(Assign(&errno, EAGAIN), Return(-1)));
    EXPECT_EQ(std::nullopt, event->read());
}

TEST_F(EventMethodTest, ReadFailure)
{
    EXPECT_CALL(mock, read(event_fd, testing::_, sizeof(struct gpioevent_data)))
        .WillOnce(DoAll(Assign(&errno, EINVAL), Return(-1)));
    EXPECT_THROW(event->read(), std::system_error);
}

TEST_F(EventMethodTest, ReadTooSmall)
{
    EXPECT_CALL(mock, read(event_fd, testing::_, sizeof(struct gpioevent_data)))
        .WillOnce(Return(1));
    EXPECT_THROW(event->read(), std::runtime_error);
}

TEST_F(EventMethodTest, GetValueSuccess)
{
    struct gpiohandle_data data;
    data.values[0] = 1;
    EXPECT_CALL(mock, gpiohandle_get_line_values(event_fd, testing::_))
        .WillOnce(DoAll(SetArgPointee<1>(data), Return(0)));
    EXPECT_EQ(data.values[0], event->getValue());
}

TEST_F(EventMethodTest, GetValueFailure)
{
    EXPECT_CALL(mock, gpiohandle_get_line_values(event_fd, testing::_))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(event->getValue(), std::system_error);
}

} // namespace
} // namespace gpioplus
