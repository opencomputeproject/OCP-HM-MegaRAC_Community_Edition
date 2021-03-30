#include "sensors/manager.hpp"
#include "test/sensor_mock.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::IsNull;
using ::testing::Return;
using ::testing::StrEq;

TEST(SensorManagerTest, BoringConstructorTest)
{
    // Build a boring SensorManager.

    sdbusplus::SdBusMock sdbus_mock_passive, sdbus_mock_host;
    auto bus_mock_passive = sdbusplus::get_mocked_new(&sdbus_mock_passive);
    auto bus_mock_host = sdbusplus::get_mocked_new(&sdbus_mock_host);

    EXPECT_CALL(sdbus_mock_host,
                sd_bus_add_object_manager(
                    IsNull(), _, StrEq("/xyz/openbmc_project/extsensors")))
        .WillOnce(Return(0));

    SensorManager s(bus_mock_passive, bus_mock_host);
    // Success
}

TEST(SensorManagerTest, AddSensorInvalidTypeTest)
{
    // AddSensor doesn't validate the type of sensor you're adding, because
    // ultimately it doesn't care -- but if we decide to change that this
    // test will start failing :D

    sdbusplus::SdBusMock sdbus_mock_passive, sdbus_mock_host;
    auto bus_mock_passive = sdbusplus::get_mocked_new(&sdbus_mock_passive);
    auto bus_mock_host = sdbusplus::get_mocked_new(&sdbus_mock_host);

    EXPECT_CALL(sdbus_mock_host,
                sd_bus_add_object_manager(
                    IsNull(), _, StrEq("/xyz/openbmc_project/extsensors")))
        .WillOnce(Return(0));

    SensorManager s(bus_mock_passive, bus_mock_host);

    std::string name = "name";
    std::string type = "invalid";
    int64_t timeout = 1;
    std::unique_ptr<Sensor> sensor =
        std::make_unique<SensorMock>(name, timeout);
    Sensor* sensor_ptr = sensor.get();

    s.addSensor(type, name, std::move(sensor));
    EXPECT_EQ(s.getSensor(name), sensor_ptr);
}
