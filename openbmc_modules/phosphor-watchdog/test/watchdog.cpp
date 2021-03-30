#include "watchdog.hpp"

#include <memory>
#include <thread>
#include <utility>

namespace phosphor
{
namespace watchdog
{

WdogTest::Quantum WdogTest::waitForWatchdog(Quantum timeLimit)
{
    auto previousTimeRemaining = wdog->timeRemaining();
    auto ret = Quantum(0);
    while (ret < timeLimit && previousTimeRemaining >= wdog->timeRemaining() &&
           wdog->timerEnabled())
    {
        previousTimeRemaining = wdog->timeRemaining();

        constexpr auto sleepTime = Quantum(1);
        if (event.run(sleepTime) == 0)
        {
            ret += sleepTime;
        }
    }

    return ret;
}

/** @brief Make sure that watchdog is started and not enabled */
TEST_F(WdogTest, createWdogAndDontEnable)
{
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());

    // We should be able to configure persistent properties
    // while disabled
    auto newAction = Watchdog::Action::PowerOff;
    EXPECT_EQ(newAction, wdog->expireAction(newAction));
    auto newIntervalMs = milliseconds(defaultInterval * 2).count();
    EXPECT_EQ(newIntervalMs, wdog->interval(newIntervalMs));

    EXPECT_EQ(newAction, wdog->expireAction());
    EXPECT_EQ(newIntervalMs, wdog->interval());

    // We won't be able to configure timeRemaining
    EXPECT_EQ(0, wdog->timeRemaining(1000));
    EXPECT_EQ(0, wdog->timeRemaining());

    // Timer should not have become enabled
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());
}

/** @brief Make sure that watchdog is started and enabled */
TEST_F(WdogTest, createWdogAndEnable)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    // Get the configured interval
    auto remaining = milliseconds(wdog->timeRemaining());

    // Its possible that we are off by few msecs depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= defaultInterval - Quantum(1)) &&
                (remaining <= defaultInterval));

    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Later, disable watchdog
 */
TEST_F(WdogTest, createWdogAndEnableThenDisable)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Disable and then verify
    EXPECT_FALSE(wdog->enabled(false));
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait for 5 quantums and make sure that the remaining
 *         time shows 5 fewer quantums.
 */
TEST_F(WdogTest, enableWdogAndWait5Quantums)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Sleep for 5 quantums
    auto sleepTime = Quantum(2);
    ASSERT_LT(sleepTime, defaultInterval);
    std::this_thread::sleep_for(sleepTime);

    // Get the remaining time again and expectation is that we get fewer
    auto remaining = milliseconds(wdog->timeRemaining());
    auto expected = defaultInterval - sleepTime;

    // Its possible that we are off by few msecs depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= expected - Quantum(1)) &&
                (remaining <= expected));
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait 1 quantum and then reset the timer to 5 quantums
 *         and then expect the watchdog to expire in 5 quantums
 */
TEST_F(WdogTest, enableWdogAndResetTo5Quantums)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Sleep for 1 second
    std::this_thread::sleep_for(Quantum(1));

    // Timer should still be running unexpired
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    // Next timer will expire in 5 quantums from now.
    auto expireTime = Quantum(5);
    auto expireTimeMs = milliseconds(expireTime).count();
    EXPECT_EQ(expireTimeMs, wdog->timeRemaining(expireTimeMs));

    // Waiting for expiration
    EXPECT_EQ(expireTime - Quantum(1), waitForWatchdog(expireTime));
    EXPECT_TRUE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());
}

/** @brief Make sure the Interval can be updated directly.
 */
TEST_F(WdogTest, verifyIntervalUpdateReceived)
{
    auto expireTime = Quantum(5);
    auto expireTimeMs = milliseconds(expireTime).count();
    EXPECT_EQ(expireTimeMs, wdog->interval(expireTimeMs));

    // Expect an update in the Interval
    EXPECT_EQ(expireTimeMs, wdog->interval());
}

/** @brief Make sure the Interval can be updated while the timer is running.
 */
TEST_F(WdogTest, verifyIntervalUpdateRunning)
{
    const auto oldInterval = milliseconds(wdog->interval());
    const auto newInterval = 5s;

    EXPECT_TRUE(wdog->enabled(true));
    auto remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(oldInterval, remaining);
    EXPECT_LE(oldInterval - Quantum(1), remaining);
    EXPECT_EQ(newInterval,
              milliseconds(wdog->interval(milliseconds(newInterval).count())));

    // Expect only the interval to update
    remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(oldInterval, remaining);
    EXPECT_LE(oldInterval - Quantum(1), remaining);
    EXPECT_EQ(newInterval, milliseconds(wdog->interval()));

    // Expect reset to use the new interval
    wdog->resetTimeRemaining(false);
    remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(newInterval, remaining);
    EXPECT_LE(newInterval - Quantum(1), remaining);
}

/** @brief Make sure that watchdog is started and enabled.
 *         Wait default interval quantums and make sure that wdog has died
 */
TEST_F(WdogTest, enableWdogAndWaitTillEnd)
{
    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Waiting default expiration
    EXPECT_EQ(defaultInterval - Quantum(1), waitForWatchdog(defaultInterval));

    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_TRUE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());
}

/** @brief Make sure the watchdog is started and enabled with a fallback
 *         Wait through the initial trip and ensure the fallback is observed
 *         Make sure that fallback runs to completion and ensure the watchdog
 *         is disabled
 */
TEST_F(WdogTest, enableWdogWithFallbackTillEnd)
{
    auto primaryInterval = Quantum(5);
    auto primaryIntervalMs = milliseconds(primaryInterval).count();
    auto fallbackInterval = primaryInterval * 2;
    auto fallbackIntervalMs = milliseconds(fallbackInterval).count();

    // We need to make a wdog with the right fallback options
    // The interval is set to be noticeably different from the default
    // so we can always tell the difference
    Watchdog::Fallback fallback;
    fallback.action = Watchdog::Action::PowerOff;
    fallback.interval = static_cast<uint64_t>(fallbackIntervalMs);
    fallback.always = false;
    wdog = std::make_unique<Watchdog>(bus, TEST_PATH, event,
                                      Watchdog::ActionTargetMap(),
                                      std::move(fallback));
    EXPECT_EQ(primaryInterval, milliseconds(wdog->interval(primaryIntervalMs)));
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());

    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Waiting default expiration
    EXPECT_EQ(primaryInterval - Quantum(1), waitForWatchdog(primaryInterval));

    // We should now have entered the fallback once the primary expires
    EXPECT_FALSE(wdog->enabled());
    auto remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    // We should still be ticking in fallback when setting action or interval
    auto newInterval = primaryInterval - Quantum(1);
    auto newIntervalMs = milliseconds(newInterval).count();
    EXPECT_EQ(newInterval, milliseconds(wdog->interval(newIntervalMs)));
    EXPECT_EQ(Watchdog::Action::None,
              wdog->expireAction(Watchdog::Action::None));

    EXPECT_FALSE(wdog->enabled());
    EXPECT_GE(remaining, milliseconds(wdog->timeRemaining()));
    EXPECT_LT(primaryInterval, milliseconds(wdog->timeRemaining()));
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    // Test that setting the timeRemaining always resets the timer to the
    // fallback interval
    EXPECT_EQ(fallback.interval, wdog->timeRemaining(primaryInterval.count()));
    EXPECT_FALSE(wdog->enabled());

    remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LE(fallbackInterval - Quantum(1), remaining);
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    // Waiting fallback expiration
    EXPECT_EQ(fallbackInterval - Quantum(1), waitForWatchdog(fallbackInterval));

    // We should now have disabled the watchdog after the fallback expires
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_TRUE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());

    // Make sure enabling the watchdog again works
    EXPECT_TRUE(wdog->enabled(true));

    // We should have re-entered the primary
    EXPECT_TRUE(wdog->enabled());
    EXPECT_GE(primaryInterval, milliseconds(wdog->timeRemaining()));
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());
}

/** @brief Make sure the watchdog is started and enabled with a fallback
 *         Wait through the initial trip and ensure the fallback is observed
 *         Make sure that we can re-enable the watchdog during fallback
 */
TEST_F(WdogTest, enableWdogWithFallbackReEnable)
{
    auto primaryInterval = Quantum(5);
    auto primaryIntervalMs = milliseconds(primaryInterval).count();
    auto fallbackInterval = primaryInterval * 2;
    auto fallbackIntervalMs = milliseconds(fallbackInterval).count();

    // We need to make a wdog with the right fallback options
    // The interval is set to be noticeably different from the default
    // so we can always tell the difference
    Watchdog::Fallback fallback;
    fallback.action = Watchdog::Action::PowerOff;
    fallback.interval = static_cast<uint64_t>(fallbackIntervalMs);
    fallback.always = false;
    wdog = std::make_unique<Watchdog>(bus, TEST_PATH, event,
                                      Watchdog::ActionTargetMap(),
                                      std::move(fallback));
    EXPECT_EQ(primaryInterval, milliseconds(wdog->interval(primaryIntervalMs)));
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());

    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Waiting default expiration
    EXPECT_EQ(primaryInterval - Quantum(1), waitForWatchdog(primaryInterval));

    // We should now have entered the fallback once the primary expires
    EXPECT_FALSE(wdog->enabled());
    auto remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    EXPECT_TRUE(wdog->enabled(true));

    // We should have re-entered the primary
    EXPECT_TRUE(wdog->enabled());
    EXPECT_GE(primaryInterval, milliseconds(wdog->timeRemaining()));
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());
}

/** @brief Make sure the watchdog is started and enabled with a fallback
 *         Wait through the initial trip and ensure the fallback is observed
 *         Make sure that changing the primary interval and calling reset timer
 *         will enable the primary watchdog with primary interval.
 */
TEST_F(WdogTest, enableWdogWithFallbackResetTimerEnable)
{
    auto primaryInterval = Quantum(5);
    auto primaryIntervalMs = milliseconds(primaryInterval).count();
    auto fallbackInterval = primaryInterval * 2;
    auto fallbackIntervalMs = milliseconds(fallbackInterval).count();
    auto newInterval = fallbackInterval * 2;
    auto newIntervalMs = milliseconds(newInterval).count();

    // We need to make a wdog with the right fallback options
    // The interval is set to be noticeably different from the default
    // so we can always tell the difference
    Watchdog::Fallback fallback;
    fallback.action = Watchdog::Action::PowerOff;
    fallback.interval = static_cast<uint64_t>(fallbackIntervalMs);
    fallback.always = false;
    wdog = std::make_unique<Watchdog>(bus, TEST_PATH, event,
                                      Watchdog::ActionTargetMap(),
                                      std::move(fallback));
    EXPECT_EQ(primaryInterval, milliseconds(wdog->interval(primaryIntervalMs)));
    EXPECT_FALSE(wdog->enabled());
    EXPECT_EQ(0, wdog->timeRemaining());
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_FALSE(wdog->timerEnabled());

    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));

    // Waiting default expiration
    EXPECT_EQ(primaryInterval - Quantum(1), waitForWatchdog(primaryInterval));

    // We should now have entered the fallback once the primary expires
    EXPECT_FALSE(wdog->enabled());
    auto remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    // Setting the interval should take effect once resetTimer re-enables wdog
    EXPECT_EQ(newIntervalMs, wdog->interval(newIntervalMs));
    wdog->resetTimeRemaining(true);

    // We should have re-entered the primary
    EXPECT_TRUE(wdog->enabled());
    remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(newInterval, remaining);
    EXPECT_LE(newInterval - Quantum(1), remaining);
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());
}

/** @brief Make sure the watchdog is started and with a fallback without
 *         sending an enable
 *         Then enable the watchdog
 *         Wait through the initial trip and ensure the fallback is observed
 *         Make sure that fallback runs to completion and ensure the watchdog
 *         is in the fallback state again
 */
TEST_F(WdogTest, enableWdogWithFallbackAlways)
{
    auto primaryInterval = Quantum(5);
    auto primaryIntervalMs = milliseconds(primaryInterval).count();
    auto fallbackInterval = primaryInterval * 2;
    auto fallbackIntervalMs = milliseconds(fallbackInterval).count();

    // We need to make a wdog with the right fallback options
    // The interval is set to be noticeably different from the default
    // so we can always tell the difference
    Watchdog::Fallback fallback;
    fallback.action = Watchdog::Action::PowerOff;
    fallback.interval = static_cast<uint64_t>(fallbackIntervalMs);
    fallback.always = true;
    wdog = std::make_unique<Watchdog>(bus, TEST_PATH, event,
                                      Watchdog::ActionTargetMap(), fallback,
                                      milliseconds(TEST_MIN_INTERVAL).count());

    // Make sure defualt interval is biggger than min interval
    EXPECT_LT(milliseconds((TEST_MIN_INTERVAL).count()),
              milliseconds(wdog->interval()));

    EXPECT_EQ(primaryInterval, milliseconds(wdog->interval(primaryIntervalMs)));
    EXPECT_FALSE(wdog->enabled());
    auto remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));
    EXPECT_GE(primaryInterval, milliseconds(wdog->timeRemaining()));

    // Waiting default expiration
    EXPECT_EQ(primaryInterval - Quantum(1), waitForWatchdog(primaryInterval));

    // We should now have entered the fallback once the primary expires
    EXPECT_FALSE(wdog->enabled());
    remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    // Waiting fallback expiration
    EXPECT_EQ(fallbackInterval - Quantum(1), waitForWatchdog(fallbackInterval));

    // We should now enter the fallback again
    EXPECT_FALSE(wdog->enabled());
    remaining = milliseconds(wdog->timeRemaining());
    EXPECT_GE(fallbackInterval, remaining);
    EXPECT_LT(primaryInterval, remaining);
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());
}

/** @brief Test minimal interval
 *  The minimal interval was set 2 seconds
 *  Test that when setting interval to 1s , it is still returning 2s
 */
TEST_F(WdogTest, verifyMinIntervalSetting)
{
    auto newInterval = Quantum(1);
    auto newIntervalMs = milliseconds(newInterval).count();
    auto minIntervalMs = milliseconds(TEST_MIN_INTERVAL).count();

    // Check first that the current interval is greater than minInterval
    EXPECT_LT(minIntervalMs, wdog->interval());
    // Check that the interval was not set to smaller value than minInterval
    EXPECT_EQ(minIntervalMs, wdog->interval(newIntervalMs));
    // Check that the interval was not set to smaller value than minInterval
    EXPECT_EQ(minIntervalMs, wdog->interval());
}

/** @brief Test minimal interval
 *  Initiate default Watchdog in order to get the default
 *  interval.
 *  Initiate watchdog with minInterval greater than default
 *  interval, and make sure the default interval was set to the
 *  minInterval.
 */
TEST_F(WdogTest, verifyConstructorMinIntervalSetting)
{
    // Initiate default Watchdog and get the default interval value.
    wdog = std::make_unique<Watchdog>(bus, TEST_PATH, event);
    auto defaultIntervalMs = wdog->interval();
    auto defaultInterval = milliseconds(defaultIntervalMs);
    auto minInterval = defaultInterval + Quantum(30);
    auto minIntervalMs = milliseconds(minInterval).count();

    // We initiate a new Watchdog with min interval greater than the default
    // intrval
    wdog = std::make_unique<Watchdog>(bus, TEST_PATH, event,
                                      Watchdog::ActionTargetMap(), std::nullopt,
                                      minIntervalMs);
    // Check that the interval was set to the minInterval
    EXPECT_EQ(minIntervalMs, wdog->interval());

    // Enable and then verify
    EXPECT_TRUE(wdog->enabled(true));
    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());

    // Set remaining time shorter than minInterval will actually set it to
    // minInterval
    auto remaining = milliseconds(wdog->timeRemaining(defaultIntervalMs));

    // Its possible that we are off by few msecs depending on
    // how we get scheduled. So checking a range here.
    EXPECT_TRUE((remaining >= minInterval - Quantum(1)) &&
                (remaining <= minInterval));

    EXPECT_FALSE(wdog->timerExpired());
    EXPECT_TRUE(wdog->timerEnabled());
}

} // namespace watchdog
} // namespace phosphor
