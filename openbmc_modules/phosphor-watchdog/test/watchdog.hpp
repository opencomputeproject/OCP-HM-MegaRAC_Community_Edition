#include "../watchdog.hpp"

#include <chrono>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <gtest/gtest.h>

namespace phosphor
{
namespace watchdog
{

using namespace std::chrono;
using namespace std::chrono_literals;

constexpr auto TEST_MIN_INTERVAL = duration<uint64_t, std::deci>(2);

// Test Watchdog functionality
class WdogTest : public ::testing::Test
{
  public:
    // The unit time used to measure the timer
    // This should be large enough to accomodate drift
    using Quantum = duration<uint64_t, std::deci>;

    // Gets called as part of each TEST_F construction
    WdogTest() :
        event(sdeventplus::Event::get_default()),
        bus(sdbusplus::bus::new_default()),
        wdog(std::make_unique<Watchdog>(
            bus, TEST_PATH, event, Watchdog::ActionTargetMap(), std::nullopt,
            milliseconds(TEST_MIN_INTERVAL).count())),

        defaultInterval(Quantum(3))

    {
        wdog->interval(milliseconds(defaultInterval).count());
        // Initially the watchdog would be disabled
        EXPECT_FALSE(wdog->enabled());
    }

    // sdevent Event handle
    sdeventplus::Event event;

    // sdbusplus handle
    sdbusplus::bus::bus bus;

    // Watchdog object
    std::unique_ptr<Watchdog> wdog;

    // This is the default interval as given in Interface definition
    Quantum defaultInterval;

  protected:
    // Dummy name for object path
    // This is just to satisfy the constructor. Does not have
    // a need to check if the objects paths have been created.
    static constexpr auto TEST_PATH = "/test/path";

    // Returns how long it took for the current watchdog timer to be
    // disabled or have its timeRemaining reset.
    Quantum waitForWatchdog(Quantum timeLimit);
};

} // namespace watchdog
} // namespace phosphor
