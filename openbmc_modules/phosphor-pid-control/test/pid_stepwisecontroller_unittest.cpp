#include "pid/controller.hpp"
#include "pid/ec/stepwise.hpp"
#include "pid/stepwisecontroller.hpp"
#include "test/zone_mock.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::StrEq;

TEST(StepwiseControllerTest, HysteresisTestPositive)
{
    // Verifies positive hysteresis works as expected

    ZoneMock z;

    std::vector<std::string> inputs = {"test"};
    ec::StepwiseInfo initial;
    initial.negativeHysteresis = 3.0;
    initial.positiveHysteresis = 2.0;
    initial.reading[0] = 20.0;
    initial.reading[1] = 30.0;
    initial.reading[2] = std::numeric_limits<double>::quiet_NaN();
    initial.output[0] = 40.0;
    initial.output[1] = 60.0;
    initial.isCeiling = false;

    std::unique_ptr<Controller> p =
        StepwiseController::createStepwiseController(&z, "foo", inputs,
                                                     initial);

    EXPECT_CALL(z, getCachedValue(StrEq("test")))
        .Times(3)
        .WillOnce(Return(29.0))  // return 40
        .WillOnce(Return(31.0))  // return 40
        .WillOnce(Return(32.0)); // return 60

    EXPECT_CALL(z, addSetPoint(40.0)).Times(2);
    EXPECT_CALL(z, addSetPoint(60.0)).Times(1);

    for (int ii = 0; ii < 3; ii++)
    {
        p->process();
    }
}

TEST(StepwiseControllerTest, HysteresisTestNegative)
{
    // Verifies negative hysteresis works as expected

    ZoneMock z;

    std::vector<std::string> inputs = {"test"};
    ec::StepwiseInfo initial;
    initial.negativeHysteresis = 3.0;
    initial.positiveHysteresis = 2.0;
    initial.reading[0] = 20.0;
    initial.reading[1] = 30.0;
    initial.reading[2] = std::numeric_limits<double>::quiet_NaN();
    initial.output[0] = 40.0;
    initial.output[1] = 60.0;
    initial.isCeiling = false;

    std::unique_ptr<Controller> p =
        StepwiseController::createStepwiseController(&z, "foo", inputs,
                                                     initial);

    EXPECT_CALL(z, getCachedValue(StrEq("test")))
        .Times(3)
        .WillOnce(Return(30.0))  // return 60
        .WillOnce(Return(27.0))  // return 60
        .WillOnce(Return(26.0)); // return 40

    EXPECT_CALL(z, addSetPoint(40.0)).Times(1);
    EXPECT_CALL(z, addSetPoint(60.0)).Times(2);

    for (int ii = 0; ii < 3; ii++)
    {
        p->process();
    }
}
