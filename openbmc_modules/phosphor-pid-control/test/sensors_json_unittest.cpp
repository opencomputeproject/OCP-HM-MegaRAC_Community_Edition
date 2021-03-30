#include "sensors/buildjson.hpp"
#include "sensors/sensor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(SensorsFromJson, emptyJsonNoSensors)
{
    // If the json has no sensors, the map is empty.

    auto j2 = R"(
      {
        "sensors": []
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_TRUE(output.empty());
}

TEST(SensorsFromJson, oneFanSensor)
{
    // If the json has one sensor, it's in the map.

    auto j2 = R"(
      {
        "sensors": [{
            "name": "fan1",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1",
            "writePath": "/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/pwm1",
            "min": 0,
            "max": 255
        }]
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_EQ(1, output.size());
    EXPECT_EQ(output["fan1"].type, "fan");
    EXPECT_EQ(output["fan1"].readPath,
              "/xyz/openbmc_project/sensors/fan_tach/fan1");
    EXPECT_EQ(output["fan1"].writePath,
              "/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/"
              "hwmon/**/pwm1");
    EXPECT_EQ(output["fan1"].min, 0);
    EXPECT_EQ(output["fan1"].max, 255);
    EXPECT_EQ(output["fan1"].timeout,
              Sensor::getDefaultTimeout(output["fan1"].type));
    EXPECT_EQ(output["fan1"].ignoreDbusMinMax, false);
}

TEST(SensorsFromJson, IgnoreDbusSensor)
{
    auto j2 = R"(
      {
        "sensors": [{
            "name": "fan1",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1",
            "ignoreDbusMinMax": true
        }]
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_EQ(1, output.size());
    EXPECT_EQ(output["fan1"].type, "fan");
    EXPECT_EQ(output["fan1"].readPath,
              "/xyz/openbmc_project/sensors/fan_tach/fan1");
    EXPECT_EQ(output["fan1"].writePath, "");
    EXPECT_EQ(output["fan1"].min, 0);
    EXPECT_EQ(output["fan1"].max, 0);
    EXPECT_EQ(output["fan1"].timeout,
              Sensor::getDefaultTimeout(output["fan1"].type));
    EXPECT_EQ(output["fan1"].ignoreDbusMinMax, true);
}

TEST(SensorsFromJson, validateOptionalFields)
{
    // The writePath, min, max, timeout, and ignoreDbusMinMax fields are
    // optional.

    auto j2 = R"(
      {
        "sensors": [{
            "name": "fan1",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1"
        }]
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_EQ(1, output.size());
    EXPECT_EQ(output["fan1"].type, "fan");
    EXPECT_EQ(output["fan1"].readPath,
              "/xyz/openbmc_project/sensors/fan_tach/fan1");
    EXPECT_EQ(output["fan1"].writePath, "");
    EXPECT_EQ(output["fan1"].min, 0);
    EXPECT_EQ(output["fan1"].max, 0);
    EXPECT_EQ(output["fan1"].timeout,
              Sensor::getDefaultTimeout(output["fan1"].type));
    EXPECT_EQ(output["fan1"].ignoreDbusMinMax, false);
}

TEST(SensorsFromJson, twoSensors)
{
    // Same as one sensor, but two.
    // If a configuration has two sensors with the same name the information
    // last is the information used.

    auto j2 = R"(
      {
        "sensors": [{
            "name": "fan1",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1"
        }, {
            "name": "fan2",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1"
        }]
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_EQ(2, output.size());
}
