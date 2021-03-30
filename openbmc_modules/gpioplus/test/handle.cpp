#include <cerrno>
#include <cstdint>
#include <gmock/gmock.h>
#include <gpioplus/handle.hpp>
#include <gpioplus/test/sys.hpp>
#include <gtest/gtest.h>
#include <linux/gpio.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace gpioplus
{
namespace
{

using testing::DoAll;
using testing::Return;
using testing::SaveArgPointee;
using testing::SetArgPointee;

TEST(HandleFlags, HandleFlagsFromLineFlags)
{
    LineFlags line_flags(GPIOLINE_FLAG_KERNEL | GPIOLINE_FLAG_OPEN_DRAIN);
    HandleFlags handle_flags(line_flags);
    EXPECT_FALSE(handle_flags.output);
    EXPECT_FALSE(handle_flags.active_low);
    EXPECT_TRUE(handle_flags.open_drain);
    EXPECT_FALSE(handle_flags.open_source);
}

TEST(HandleFlags, HandleFlagsToInt)
{
    HandleFlags handle_flags;
    handle_flags.output = false;
    handle_flags.active_low = true;
    handle_flags.open_drain = false;
    handle_flags.open_source = false;
    EXPECT_EQ(GPIOHANDLE_REQUEST_INPUT | GPIOHANDLE_REQUEST_ACTIVE_LOW,
              handle_flags.toInt());

    handle_flags.output = true;
    handle_flags.active_low = false;
    handle_flags.open_drain = true;
    handle_flags.open_source = true;
    EXPECT_EQ(GPIOHANDLE_REQUEST_OUTPUT | GPIOHANDLE_REQUEST_OPEN_DRAIN |
                  GPIOHANDLE_REQUEST_OPEN_SOURCE,
              handle_flags.toInt());
}

class HandleTest : public testing::Test
{
  protected:
    const int chip_fd = 1234;
    const int handle_fd = 2345;
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

TEST_F(HandleTest, ConstructSuccess)
{
    const std::string label{"test"};
    std::vector<Handle::Line> lines;
    for (uint32_t i = 0; i < 7; ++i)
    {
        lines.push_back({i, 1});
    }

    struct gpiohandle_request req, ret;
    ret.fd = handle_fd;
    EXPECT_CALL(mock, gpio_get_linehandle(chip_fd, testing::_))
        .WillOnce(
            DoAll(SaveArgPointee<1>(&req), SetArgPointee<1>(ret), Return(0)));
    Handle handle(*chip, lines,
                  HandleFlags(LineFlags(GPIOLINE_FLAG_ACTIVE_LOW)),
                  label.c_str());

    EXPECT_EQ(handle_fd, *handle.getFd());
    EXPECT_EQ(GPIOHANDLE_REQUEST_INPUT | GPIOHANDLE_REQUEST_ACTIVE_LOW,
              req.flags);
    EXPECT_EQ(label, req.consumer_label);
    EXPECT_EQ(lines.size(), req.lines);
    for (uint32_t i = 0; i < lines.size(); ++i)
    {
        EXPECT_EQ(i, req.lineoffsets[i]);
        EXPECT_EQ(1, req.default_values[i]);
    }

    EXPECT_CALL(mock, close(handle_fd)).WillOnce(Return(0));
}

TEST_F(HandleTest, ConstructTooMany)
{
    const std::vector<Handle::Line> lines(GPIOHANDLES_MAX + 1);
    EXPECT_THROW(Handle(*chip, lines, HandleFlags(), "too_many"),
                 std::runtime_error);
}

TEST_F(HandleTest, ConstructLabelTooLong)
{
    const size_t large_size = sizeof(
        reinterpret_cast<struct gpiohandle_request*>(NULL)->consumer_label);
    EXPECT_THROW(Handle(*chip, {}, HandleFlags(), std::string(large_size, '1')),
                 std::invalid_argument);
}

TEST_F(HandleTest, ConstructError)
{
    const std::string label{"error"};
    std::vector<Handle::Line> lines;
    for (uint32_t i = 0; i < 5; ++i)
    {
        lines.push_back({i, 0});
    }

    struct gpiohandle_request req;
    EXPECT_CALL(mock, gpio_get_linehandle(chip_fd, testing::_))
        .WillOnce(DoAll(SaveArgPointee<1>(&req), Return(-EINVAL)));
    EXPECT_THROW(Handle(*chip, lines,
                        HandleFlags(LineFlags(GPIOLINE_FLAG_IS_OUT)),
                        label.c_str()),
                 std::runtime_error);

    EXPECT_EQ(GPIOHANDLE_REQUEST_OUTPUT, req.flags);
    EXPECT_EQ(label, req.consumer_label);
    EXPECT_EQ(lines.size(), req.lines);
    for (uint32_t i = 0; i < lines.size(); ++i)
    {
        EXPECT_EQ(i, req.lineoffsets[i]);
        EXPECT_EQ(0, req.default_values[i]);
    }
}

class HandleMethodTest : public HandleTest
{
  protected:
    std::unique_ptr<Handle> handle;
    const std::vector<Handle::Line> lines{{1, 1}, {4, 0}};

    void SetUp()
    {
        HandleTest::SetUp();
        struct gpiohandle_request ret;
        ret.fd = handle_fd;
        EXPECT_CALL(mock, gpio_get_linehandle(chip_fd, testing::_))
            .WillOnce(DoAll(SetArgPointee<1>(ret), Return(0)));
        handle = std::make_unique<Handle>(
            *chip, lines, HandleFlags(LineFlags(GPIOLINE_FLAG_IS_OUT)),
            "method");
    }

    void TearDown()
    {
        EXPECT_CALL(mock, close(handle_fd)).WillOnce(Return(0));
        handle.reset();
        HandleTest::TearDown();
    }
};

TEST_F(HandleMethodTest, GetValuesRet)
{
    const std::vector<uint8_t> expected{0, 0};
    struct gpiohandle_data ret;
    ret.values[0] = 0;
    ret.values[1] = 0;

    EXPECT_CALL(mock, gpiohandle_get_line_values(handle_fd, testing::_))
        .WillOnce(DoAll(SetArgPointee<1>(ret), Return(0)));
    EXPECT_EQ(expected, handle->getValues());
}

TEST_F(HandleMethodTest, GetValuesSuccess)
{
    const std::vector<uint8_t> expected{1, 1};
    struct gpiohandle_data ret;
    ret.values[0] = 1;
    ret.values[1] = 1;

    EXPECT_CALL(mock, gpiohandle_get_line_values(handle_fd, testing::_))
        .WillOnce(DoAll(SetArgPointee<1>(ret), Return(0)));
    std::vector<uint8_t> output;
    handle->getValues(output);
    EXPECT_EQ(expected, output);
}

TEST_F(HandleMethodTest, GetValuesFailure)
{
    EXPECT_CALL(mock, gpiohandle_get_line_values(handle_fd, testing::_))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(handle->getValues(), std::system_error);
}

TEST_F(HandleMethodTest, SetValuesSuccess)
{
    struct gpiohandle_data req;
    EXPECT_CALL(mock, gpiohandle_set_line_values(handle_fd, testing::_))
        .WillOnce(DoAll(SaveArgPointee<1>(&req), Return(0)));
    handle->setValues({0, 1});
    EXPECT_EQ(0, req.values[0]);
    EXPECT_EQ(1, req.values[1]);
}

TEST_F(HandleMethodTest, SetValuesInvalid)
{
    EXPECT_THROW(handle->setValues({1}), std::runtime_error);
    EXPECT_THROW(handle->setValues({1, 0, 1}), std::runtime_error);
}

TEST_F(HandleMethodTest, SetValuesFailure)
{
    struct gpiohandle_data req;
    EXPECT_CALL(mock, gpiohandle_set_line_values(handle_fd, testing::_))
        .WillOnce(DoAll(SaveArgPointee<1>(&req), Return(-EINVAL)));
    EXPECT_THROW(handle->setValues({1, 1}), std::system_error);
    EXPECT_EQ(1, req.values[0]);
    EXPECT_EQ(1, req.values[1]);
}

} // namespace
} // namespace gpioplus
