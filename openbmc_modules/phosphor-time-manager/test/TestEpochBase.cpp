#include "epoch_base.hpp"
#include "types.hpp"

#include <sdbusplus/bus.hpp>

#include <gtest/gtest.h>

namespace phosphor
{
namespace time
{

class TestEpochBase : public testing::Test
{
  public:
    sdbusplus::bus::bus bus;
    EpochBase epochBase;

    TestEpochBase() : bus(sdbusplus::bus::new_default()), epochBase(bus, "")
    {
        // Empty
    }

    Mode getMode()
    {
        return epochBase.timeMode;
    }
};

TEST_F(TestEpochBase, onModeChange)
{
    epochBase.onModeChanged(Mode::NTP);
    EXPECT_EQ(Mode::NTP, getMode());

    epochBase.onModeChanged(Mode::Manual);
    EXPECT_EQ(Mode::Manual, getMode());
}

} // namespace time
} // namespace phosphor
