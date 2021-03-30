#include "../fallback.hpp"

#include "../psensor.hpp"

#include <iostream>
#include <string>

#include <gtest/gtest.h>

int pstate = -1;

namespace phosphor
{
namespace fan
{
namespace presence
{
void setPresence(const Fan& fan, bool newState)
{
    if (newState)
    {
        pstate = 1;
    }
    else
    {
        pstate = 0;
    }
}

class TestSensor : public PresenceSensor
{
  public:
    bool start() override
    {
        ++started;
        return _present;
    }

    void stop() override
    {
        ++stopped;
    }

    bool present() override
    {
        return _present;
    }

    void fail() override
    {
        ++failed;
    }

    bool _present = false;
    size_t started = 0;
    size_t stopped = 0;
    size_t failed = 0;
};

} // namespace presence
} // namespace fan
} // namespace phosphor

using namespace phosphor::fan::presence;

TEST(FallbackTest, TestOne)
{
    // Validate a single sensor.
    // Validate on->off->on.
    pstate = -1;
    Fan fan{"/path", "name"};
    TestSensor ts;
    ts._present = true;
    std::vector<std::reference_wrapper<PresenceSensor>> sensors{ts};
    Fallback f{fan, sensors};

    f.monitor();
    ASSERT_EQ(pstate, 1);
    ASSERT_EQ(ts.failed, 0);
    ASSERT_EQ(ts.stopped, 0);
    ASSERT_EQ(ts.started, 1);

    f.stateChanged(false);
    ASSERT_EQ(pstate, 0);
    ASSERT_EQ(ts.failed, 0);
    ASSERT_EQ(ts.stopped, 0);
    ASSERT_EQ(ts.started, 1);

    f.stateChanged(true);
    ASSERT_EQ(pstate, 1);
    ASSERT_EQ(ts.failed, 0);
    ASSERT_EQ(ts.stopped, 0);
    ASSERT_EQ(ts.started, 1);
}

TEST(FallbackTest, TestTwoA)
{
    // Validate two sensors.
    // Validate both sensors on->off->on

    pstate = -1;
    Fan fan{"/path", "name"};
    TestSensor ts1, ts2;
    ts1._present = true;
    ts2._present = true;

    std::vector<std::reference_wrapper<PresenceSensor>> sensors{ts1, ts2};
    Fallback f{fan, sensors};

    f.monitor();
    ASSERT_EQ(pstate, 1);
    ASSERT_EQ(ts1.failed, 0);
    ASSERT_EQ(ts1.stopped, 0);
    ASSERT_EQ(ts1.started, 1);
    ASSERT_EQ(ts2.failed, 0);
    ASSERT_EQ(ts2.stopped, 0);
    ASSERT_EQ(ts2.started, 0);

    ts1._present = false;
    ts2._present = false;
    f.stateChanged(false);
    ASSERT_EQ(pstate, 0);
    ASSERT_EQ(ts1.failed, 0);
    ASSERT_EQ(ts1.stopped, 0);
    ASSERT_EQ(ts1.started, 1);
    ASSERT_EQ(ts2.failed, 0);
    ASSERT_EQ(ts2.stopped, 0);
    ASSERT_EQ(ts2.started, 0);

    ts1._present = true;
    ts2._present = true;
    f.stateChanged(true);
    ASSERT_EQ(pstate, 1);
    ASSERT_EQ(ts1.failed, 0);
    ASSERT_EQ(ts1.stopped, 0);
    ASSERT_EQ(ts1.started, 1);
    ASSERT_EQ(ts2.failed, 0);
    ASSERT_EQ(ts2.stopped, 0);
    ASSERT_EQ(ts2.started, 0);
}

TEST(FallbackTest, TestTwoB)
{
    // Validate two sensors.
    // Validate first sensor on->off.

    pstate = -1;
    Fan fan{"/path", "name"};
    TestSensor ts1, ts2;
    ts1._present = true;
    ts2._present = true;

    std::vector<std::reference_wrapper<PresenceSensor>> sensors{ts1, ts2};
    Fallback f{fan, sensors};

    f.monitor();
    ASSERT_EQ(pstate, 1);
    ts1._present = false;
    f.stateChanged(false);
    ASSERT_EQ(pstate, 1);
    ASSERT_EQ(ts1.failed, 1);
    ASSERT_EQ(ts1.stopped, 1);
    ASSERT_EQ(ts1.started, 1);
    ASSERT_EQ(ts2.failed, 0);
    ASSERT_EQ(ts2.stopped, 0);
    ASSERT_EQ(ts2.started, 1);

    // Flip the state of both sensors.
    ts1._present = true;
    ts2._present = false;
    f.stateChanged(false);
    ASSERT_EQ(pstate, 0);
    ASSERT_EQ(ts1.failed, 1);
    ASSERT_EQ(ts1.stopped, 1);
    ASSERT_EQ(ts1.started, 1);
    ASSERT_EQ(ts2.failed, 0);
    ASSERT_EQ(ts2.stopped, 0);
    ASSERT_EQ(ts2.started, 1);
}

TEST(FallbackTest, TestTwoC)
{
    // Validate two sensors.
    // Validate first in bad state.

    pstate = -1;
    Fan fan{"/path", "name"};
    TestSensor ts1, ts2;
    ts1._present = false;
    ts2._present = true;

    std::vector<std::reference_wrapper<PresenceSensor>> sensors{ts1, ts2};
    Fallback f{fan, sensors};

    f.monitor();
    ASSERT_EQ(pstate, 1);
    ASSERT_EQ(ts1.failed, 1);
    ASSERT_EQ(ts1.stopped, 0);
    ASSERT_EQ(ts1.started, 0);
    ASSERT_EQ(ts2.failed, 0);
    ASSERT_EQ(ts2.stopped, 0);
    ASSERT_EQ(ts2.started, 1);

    f.stateChanged(false);
    ASSERT_EQ(pstate, 0);
    ASSERT_EQ(ts1.failed, 1);
    ASSERT_EQ(ts1.stopped, 0);
    ASSERT_EQ(ts1.started, 0);
    ASSERT_EQ(ts2.failed, 0);
    ASSERT_EQ(ts2.stopped, 0);
    ASSERT_EQ(ts2.started, 1);
}

TEST(FallbackTest, TestTwoD)
{
    // Validate two sensors.
    // Validate both in bad state.

    pstate = -1;
    Fan fan{"/path", "name"};
    TestSensor ts1, ts2;
    ts1._present = false;
    ts2._present = false;

    std::vector<std::reference_wrapper<PresenceSensor>> sensors{ts1, ts2};
    Fallback f{fan, sensors};

    f.monitor();
    ASSERT_EQ(pstate, 0);
    ASSERT_EQ(ts1.failed, 0);
    ASSERT_EQ(ts1.stopped, 0);
    ASSERT_EQ(ts1.started, 1);
    ASSERT_EQ(ts2.failed, 0);
    ASSERT_EQ(ts2.stopped, 0);
    ASSERT_EQ(ts2.started, 0);
}
