#include "filesystem_mock.hpp"
#include "hwmonio.hpp"

#include <chrono>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace hwmonio
{
namespace
{

using ::testing::_;
using ::testing::Return;

class HwmonIOTest : public ::testing::Test
{
  protected:
    HwmonIOTest() : _hwmonio(_path, &_mock)
    {
    }

    const int64_t _value = 12;
    const std::string _path = "abcd";
    const std::string _type = "fan";
    const std::string _id = "a";
    const std::string _sensor = "1";
    const size_t _retries = 1;
    const std::chrono::milliseconds _delay = std::chrono::milliseconds{10};

    FileSystemMock _mock;
    HwmonIO _hwmonio;
};

TEST_F(HwmonIOTest, ReadReturnsValue)
{
    EXPECT_CALL(_mock, read(_)).WillOnce(Return(_value));
    EXPECT_THAT(_hwmonio.read(_type, _id, _sensor, _retries, _delay), _value);
}

int64_t SetErrnoExcept(const std::string&)
{
    errno = ETIMEDOUT;
    throw std::runtime_error("bad times");
}

TEST_F(HwmonIOTest, ReadExceptsRetryable)
{
    EXPECT_CALL(_mock, read(_))
        .WillOnce(&SetErrnoExcept)
        .WillOnce(Return(_value));
    EXPECT_THAT(_hwmonio.read(_type, _id, _sensor, _retries, _delay), _value);
}

} // namespace
} // namespace hwmonio
