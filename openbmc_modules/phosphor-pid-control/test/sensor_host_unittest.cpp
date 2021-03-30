#include "sensors/host.hpp"
#include "test/helpers.hpp"

#include <chrono>
#include <memory>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::IsNull;
using ::testing::Return;
using ::testing::StrEq;

TEST(HostSensorTest, BoringConstructorTest)
{
    // WARN: The host sensor is not presently meant to be created this way,
    // TODO: Can I move the constructor into private?
}

TEST(HostSensorTest, CreateHostTempSensorTest)
{
    // The normal case for this sensor is to be a temperature sensor, where
    // the value is treated as a margin sensor.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    std::string name = "fleeting0";
    int64_t timeout = 1;
    const char* objPath = "/asdf/asdf0";
    bool defer = false;
    std::string interface = "xyz.openbmc_project.Sensor.Value";

    std::vector<std::string> properties = {};
    double d;

    // The createTemp updates all the properties, however, only Scale is set
    // to non-default.
    SetupDbusObject(&sdbus_mock, defer, objPath, interface, properties, &d);

    // This is called during object destruction.
    EXPECT_CALL(sdbus_mock,
                sd_bus_emit_object_removed(IsNull(), StrEq(objPath)))
        .WillOnce(Return(0));

    std::unique_ptr<Sensor> s =
        HostSensor::createTemp(name, timeout, bus_mock, objPath, defer);
}

TEST(HostSensorTest, VerifyWriteThenReadMatches)
{
    // Verify that when value is updated, the information matches
    // what we expect when read back.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    std::string name = "fleeting0";
    int64_t timeout = 1;
    const char* objPath = "/asdf/asdf0";
    bool defer = false;
    std::string interface = "xyz.openbmc_project.Sensor.Value";

    std::vector<std::string> properties = {};
    double d;

    SetupDbusObject(&sdbus_mock, defer, objPath, interface, properties, &d);

    EXPECT_CALL(sdbus_mock,
                sd_bus_emit_object_removed(IsNull(), StrEq(objPath)))
        .WillOnce(Return(0));

    std::unique_ptr<Sensor> s =
        HostSensor::createTemp(name, timeout, bus_mock, objPath, defer);

    // Value is updated from dbus calls only (normally).
    HostSensor* hs = static_cast<HostSensor*>(s.get());
    double new_value = 2;

    ReadReturn r = hs->read();
    EXPECT_EQ(r.value, 0);

    EXPECT_CALL(sdbus_mock,
                sd_bus_emit_properties_changed_strv(
                    IsNull(), StrEq(objPath), StrEq(interface), NotNull()))
        .WillOnce(Invoke([=](sd_bus* bus, const char* path,
                             const char* interface, char** names) {
            EXPECT_STREQ("Value", names[0]);
            return 0;
        }));

    std::chrono::high_resolution_clock::time_point t1 =
        std::chrono::high_resolution_clock::now();

    hs->value(new_value);
    r = hs->read();
    EXPECT_EQ(r.value, new_value);

    auto duration =
        std::chrono::duration_cast<std::chrono::seconds>(t1 - r.updated)
            .count();

    // Verify it was updated within the last second.
    EXPECT_TRUE(duration < 1);
}
