#include "manager.hpp"
#include "mocked_property_change_listener.hpp"
#include "types.hpp"

#include <sdbusplus/bus.hpp>

#include <gtest/gtest.h>

using ::testing::_;

namespace phosphor
{
namespace time
{

class TestManager : public testing::Test
{
  public:
    sdbusplus::bus::bus bus;
    Manager manager;
    MockPropertyChangeListner listener1;
    MockPropertyChangeListner listener2;

    TestManager() : bus(sdbusplus::bus::new_default()), manager(bus)
    {
        // Add two mocked listeners so that we can test
        // the behavior related to listeners
        manager.addListener(&listener1);
        manager.addListener(&listener2);
    }

    // Proxies for Manager's private members and functions
    Mode getTimeMode()
    {
        return manager.timeMode;
    }
    bool hostOn()
    {
        return manager.hostOn;
    }
    std::string getRequestedMode()
    {
        return manager.requestedMode;
    }
    void notifyPropertyChanged(const std::string& key, const std::string& value)
    {
        manager.onPropertyChanged(key, value);
    }
    void notifyOnHostState(bool hostOn)
    {
        manager.onHostState(hostOn);
    }
};

TEST_F(TestManager, DISABLED_empty)
{
    EXPECT_FALSE(hostOn());
    EXPECT_EQ("", getRequestedMode());

    // Default mode is MANUAL
    EXPECT_EQ(Mode::Manual, getTimeMode());
}

TEST_F(TestManager, DISABLED_hostStateChange)
{
    notifyOnHostState(true);
    EXPECT_TRUE(hostOn());
    notifyOnHostState(false);
    EXPECT_FALSE(hostOn());
}

TEST_F(TestManager, DISABLED_propertyChanged)
{
    // When host is off, property change will be notified to listeners
    EXPECT_FALSE(hostOn());

    // Check mocked listeners shall receive notifications on property changed
    EXPECT_CALL(listener1, onModeChanged(Mode::Manual)).Times(1);
    EXPECT_CALL(listener2, onModeChanged(Mode::Manual)).Times(1);

    notifyPropertyChanged(
        "TimeSyncMethod",
        "xyz.openbmc_project.Time.Synchronization.Method.Manual");

    EXPECT_EQ("", getRequestedMode());

    // When host is on, property changes are saved as requested ones
    notifyOnHostState(true);

    // Check mocked listeners shall not receive notifications
    EXPECT_CALL(listener1, onModeChanged(Mode::Manual)).Times(0);
    EXPECT_CALL(listener2, onModeChanged(Mode::Manual)).Times(0);

    notifyPropertyChanged(
        "TimeSyncMethod",
        "xyz.openbmc_project.Time.Synchronization.Method.NTP");

    EXPECT_EQ("xyz.openbmc_project.Time.Synchronization.Method.NTP",
              getRequestedMode());

    // When host becomes off, the requested mode shall be notified
    // to listeners, and be cleared
    EXPECT_CALL(listener1, onModeChanged(Mode::NTP)).Times(1);
    EXPECT_CALL(listener2, onModeChanged(Mode::NTP)).Times(1);

    notifyOnHostState(false);

    EXPECT_EQ("", getRequestedMode());

    // When host is on, and invalid property is changed,
    // verify the code asserts because it shall never occur
    notifyOnHostState(true);
    ASSERT_DEATH(notifyPropertyChanged("invalid property", "whatever"), "");
}

TEST_F(TestManager, DISABLED_propertyChangedAndChangedbackWhenHostOn)
{
    // Property is now MANUAL/HOST
    notifyPropertyChanged(
        "TimeSyncMethod",
        "xyz.openbmc_project.Time.Synchronization.Method.Manual");

    // Set host on
    notifyOnHostState(true);

    // Check mocked listeners shall not receive notifications
    EXPECT_CALL(listener1, onModeChanged(_)).Times(0);
    EXPECT_CALL(listener2, onModeChanged(_)).Times(0);

    notifyPropertyChanged(
        "TimeSyncMethod",
        "xyz.openbmc_project.Time.Synchronization.Method.NTP");

    // Saved as requested mode
    EXPECT_EQ("xyz.openbmc_project.Time.Synchronization.Method.NTP",
              getRequestedMode());

    // Property changed back to MANUAL/HOST
    notifyPropertyChanged(
        "TimeSyncMethod",
        "xyz.openbmc_project.Time.Synchronization.Method.Manual");

    // Requested mode shall be updated
    EXPECT_EQ("xyz.openbmc_project.Time.Synchronization.Method.Manual",
              getRequestedMode());

    // Because the latest mode is the same as when host is off,
    // The listeners shall not be notified, and requested mode
    // shall be cleared
    EXPECT_CALL(listener1, onModeChanged(_)).Times(0);
    EXPECT_CALL(listener2, onModeChanged(_)).Times(0);

    notifyOnHostState(false);

    EXPECT_EQ("", getRequestedMode());
}

// TODO: if gmock is ready, add case to test
// updateNtpSetting() and updateNetworkSetting()

} // namespace time
} // namespace phosphor
