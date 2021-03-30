#include "env_mock.hpp"
#include "gpio_mock.hpp"
#include "hwmonio_mock.hpp"
#include "sensor.hpp"

#include <gpioplus/test/handle.hpp>
#include <memory>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace env
{

// Delegate all calls to getEnv() to the mock
const char* EnvImpl::get(const char* key) const
{
    return mockEnv.get(key);
}

EnvImpl env_impl;

} // namespace env

class SensorTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        gpioIntf = nullptr;
    }

    std::string temp = "temp";
    std::string five = "5";
};

using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Pair;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

TEST_F(SensorTest, BasicConstructorTest)
{
    /* Constructor test with nothing in an rcList or GPIO chip. */
    auto sensorKey = std::make_pair(temp, five);
    std::unique_ptr<hwmonio::HwmonIOInterface> hwmonio_mock =
        std::make_unique<hwmonio::HwmonIOMock>();
    std::string path = "/";

    /* Always calls GPIOCHIP and GPIO checks, returning empty string. */
    EXPECT_CALL(env::mockEnv, get(StrEq("GPIOCHIP_temp5")))
        .WillOnce(Return(""));
    EXPECT_CALL(env::mockEnv, get(StrEq("GPIO_temp5"))).WillOnce(Return(""));

    /* Always calls GAIN and OFFSET, can use ON_CALL instead of EXPECT_CALL */
    EXPECT_CALL(env::mockEnv, get(StrEq("GAIN_temp5"))).WillOnce(Return(""));
    EXPECT_CALL(env::mockEnv, get(StrEq("OFFSET_temp5"))).WillOnce(Return(""));
    EXPECT_CALL(env::mockEnv, get(StrEq("REMOVERCS_temp5")))
        .WillOnce(Return(""));

    auto sensor =
        std::make_unique<sensor::Sensor>(sensorKey, hwmonio_mock.get(), path);
    EXPECT_FALSE(sensor == nullptr);
}

TEST_F(SensorTest, SensorRequiresGpio)
{
    /* Constructor test with only the GPIO chip set. */

    StrictMock<GpioHandleMock> gMock;
    gpioIntf = &gMock;

    /* The following piece of code can probably be copied above once it's
     * working.
     */
    auto handleMock = std::make_unique<gpioplus::test::HandleMock>();

    auto sensorKey = std::make_pair(temp, five);
    std::unique_ptr<hwmonio::HwmonIOInterface> hwmonio_mock =
        std::make_unique<hwmonio::HwmonIOMock>();
    std::string path = "/";

    EXPECT_CALL(env::mockEnv, get(StrEq("GPIOCHIP_temp5")))
        .WillOnce(Return("chipA"));
    EXPECT_CALL(env::mockEnv, get(StrEq("GPIO_temp5"))).WillOnce(Return("5"));

    EXPECT_CALL(gMock, build(StrEq("chipA"), StrEq("5")))
        .WillOnce(Invoke([&](const std::string&, const std::string&) {
            return std::move(handleMock);
        }));

    /* Always calls GAIN and OFFSET, can use ON_CALL instead of EXPECT_CALL */
    EXPECT_CALL(env::mockEnv, get(StrEq("GAIN_temp5"))).WillOnce(Return(""));
    EXPECT_CALL(env::mockEnv, get(StrEq("OFFSET_temp5"))).WillOnce(Return(""));
    EXPECT_CALL(env::mockEnv, get(StrEq("REMOVERCS_temp5")))
        .WillOnce(Return(""));

    auto sensor =
        std::make_unique<sensor::Sensor>(sensorKey, hwmonio_mock.get(), path);
    EXPECT_FALSE(sensor == nullptr);
}

TEST_F(SensorTest, SensorHasGainAndOffsetAdjustValue)
{
    /* Construct a sensor that has a gain and offset, then verify they are used
     * when adjusting the value.
     */

    auto sensorKey = std::make_pair(temp, five);
    std::unique_ptr<hwmonio::HwmonIOInterface> hwmonio_mock =
        std::make_unique<hwmonio::HwmonIOMock>();
    std::string path = "/";

    /* Always calls GPIOCHIP_temp5 and GPIO checks, returning empty string. */
    EXPECT_CALL(env::mockEnv, get(StrEq("GPIOCHIP_temp5")))
        .WillOnce(Return(""));
    EXPECT_CALL(env::mockEnv, get(StrEq("GPIO_temp5"))).WillOnce(Return(""));

    EXPECT_CALL(env::mockEnv, get(StrEq("GAIN_temp5"))).WillOnce(Return("10"));
    EXPECT_CALL(env::mockEnv, get(StrEq("OFFSET_temp5")))
        .WillOnce(Return("15"));
    EXPECT_CALL(env::mockEnv, get(StrEq("REMOVERCS_temp5")))
        .WillOnce(Return(""));

    auto sensor =
        std::make_unique<sensor::Sensor>(sensorKey, hwmonio_mock.get(), path);
    EXPECT_FALSE(sensor == nullptr);

    double startingValue = 1.0;
    double resultValue = sensor->adjustValue(startingValue);
    EXPECT_DOUBLE_EQ(resultValue, 25.0);
}
