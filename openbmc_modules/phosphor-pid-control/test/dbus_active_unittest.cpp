#include "dbus/dbusactiveread.hpp"
#include "test/dbushelper_mock.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Invoke;
using ::testing::NotNull;

TEST(DbusActiveReadTest, BoringConstructorTest)
{
    // Verify we can construct it.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    DbusHelperMock helper;
    std::string path = "/asdf";
    std::string service = "asdfasdf.asdfasdf";

    DbusActiveRead ar(bus_mock, path, service, &helper);
}

TEST(DbusActiveReadTest, Read_VerifyCallsToDbusForValue)
{
    // Verify it calls to get the value from dbus when requested.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    DbusHelperMock helper;
    std::string path = "/asdf";
    std::string service = "asdfasdf.asdfasdf";

    DbusActiveRead ar(bus_mock, path, service, &helper);

    EXPECT_CALL(helper, getProperties(_, service, path, NotNull()))
        .WillOnce(
            Invoke([&](sdbusplus::bus::bus& bus, const std::string& service,
                       const std::string& path, struct SensorProperties* prop) {
                prop->scale = -3;
                prop->value = 10000;
                prop->unit = "x";
            }));

    ReadReturn r = ar.read();
    EXPECT_EQ(10, r.value);
}

// WARN: getProperties will raise an exception on failure
// Instead of just not updating the value.
