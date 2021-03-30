#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gpioplus/chip.hpp>
#include <gpioplus/test/sys.hpp>
#include <gtest/gtest.h>
#include <linux/gpio.h>
#include <memory>
#include <system_error>

namespace gpioplus
{
namespace
{

using testing::DoAll;
using testing::Return;
using testing::SaveArgPointee;
using testing::SetArgPointee;
using testing::StrEq;

TEST(LineFlagsTest, LineFlagsFromFlags)
{
    LineFlags line_flags(GPIOLINE_FLAG_KERNEL | GPIOLINE_FLAG_OPEN_DRAIN |
                         GPIOLINE_FLAG_OPEN_SOURCE);
    EXPECT_TRUE(line_flags.kernel);
    EXPECT_FALSE(line_flags.output);
    EXPECT_FALSE(line_flags.active_low);
    EXPECT_TRUE(line_flags.open_drain);
    EXPECT_TRUE(line_flags.open_source);

    line_flags = GPIOLINE_FLAG_IS_OUT | GPIOLINE_FLAG_ACTIVE_LOW;
    EXPECT_FALSE(line_flags.kernel);
    EXPECT_TRUE(line_flags.output);
    EXPECT_TRUE(line_flags.active_low);
    EXPECT_FALSE(line_flags.open_drain);
    EXPECT_FALSE(line_flags.open_source);
}

class ChipMethodTest : public testing::Test
{
  protected:
    const int expected_fd = 1234;
    testing::StrictMock<test::SysMock> mock;
    std::unique_ptr<Chip> chip;

    void SetUp()
    {
        const int chip_id = 1;
        const char* path = "/dev/gpiochip1";

        EXPECT_CALL(mock, open(StrEq(path), O_RDONLY | O_CLOEXEC))
            .WillOnce(Return(expected_fd));
        chip = std::make_unique<Chip>(chip_id, &mock);
    }

    void TearDown()
    {
        EXPECT_CALL(mock, close(expected_fd)).WillOnce(Return(0));
        chip.reset();
    }
};

TEST_F(ChipMethodTest, Basic)
{
    EXPECT_EQ(expected_fd, *chip->getFd());
    EXPECT_EQ(&mock, chip->getFd().getSys());
}

TEST_F(ChipMethodTest, GetChipInfoSuccess)
{
    const ChipInfo expected_info{"name", "label", 31};
    struct gpiochip_info info;
    ASSERT_LE(expected_info.name.size(), sizeof(info.name));
    strcpy(info.name, expected_info.name.c_str());
    ASSERT_LE(expected_info.label.size(), sizeof(info.label));
    strcpy(info.label, expected_info.label.c_str());
    info.lines = expected_info.lines;

    EXPECT_CALL(mock, gpio_get_chipinfo(expected_fd, testing::_))
        .WillOnce(DoAll(SetArgPointee<1>(info), Return(0)));
    ChipInfo retrieved_info = chip->getChipInfo();
    EXPECT_EQ(expected_info.name, retrieved_info.name);
    EXPECT_EQ(expected_info.label, retrieved_info.label);
    EXPECT_EQ(expected_info.lines, retrieved_info.lines);
}

TEST_F(ChipMethodTest, GetChipInfoFail)
{
    EXPECT_CALL(mock, gpio_get_chipinfo(expected_fd, testing::_))
        .WillOnce(Return(-EINVAL));
    EXPECT_THROW(chip->getChipInfo(), std::system_error);
}

TEST_F(ChipMethodTest, GetLineInfoSuccess)
{
    const uint32_t line = 176;
    const LineInfo expected_info{GPIOLINE_FLAG_ACTIVE_LOW, "name", "consumer"};
    struct gpioline_info info, req;
    info.flags = GPIOLINE_FLAG_ACTIVE_LOW;
    ASSERT_LE(expected_info.name.size(), sizeof(info.name));
    strcpy(info.name, expected_info.name.c_str());
    ASSERT_LE(expected_info.consumer.size(), sizeof(info.consumer));
    strcpy(info.consumer, expected_info.consumer.c_str());

    EXPECT_CALL(mock, gpio_get_lineinfo(expected_fd, testing::_))
        .WillOnce(
            DoAll(SaveArgPointee<1>(&req), SetArgPointee<1>(info), Return(0)));
    LineInfo retrieved_info = chip->getLineInfo(line);
    EXPECT_EQ(line, req.line_offset);
    EXPECT_FALSE(retrieved_info.flags.kernel);
    EXPECT_FALSE(retrieved_info.flags.output);
    EXPECT_TRUE(retrieved_info.flags.active_low);
    EXPECT_FALSE(retrieved_info.flags.open_drain);
    EXPECT_FALSE(retrieved_info.flags.open_source);
    EXPECT_EQ(expected_info.name, retrieved_info.name);
    EXPECT_EQ(expected_info.consumer, retrieved_info.consumer);
}

TEST_F(ChipMethodTest, GetLineInfoFail)
{
    const uint32_t line = 143;

    struct gpioline_info info;
    EXPECT_CALL(mock, gpio_get_lineinfo(expected_fd, testing::_))
        .WillOnce(DoAll(SaveArgPointee<1>(&info), Return(-EINVAL)));
    EXPECT_THROW(chip->getLineInfo(line), std::system_error);
    EXPECT_EQ(line, info.line_offset);
}

} // namespace
} // namespace gpioplus
