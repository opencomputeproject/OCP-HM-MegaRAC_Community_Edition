#include "build/buildjson.hpp"
#include "errors/exception.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(ConfigurationVerificationTest, VerifyHappy)
{
    /* Verify a happy configuration throws no exceptions. */
    auto j2 = R"(
      {
        "sensors": [{
          "name": "fan1",
          "type": "fan",
          "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1"
        }],
        "zones": [{
          "id": 1,
          "minThermalOutput": 3000.0,
          "failsafePercent": 75.0,
          "pids": [{
            "name": "fan1-5",
            "type": "fan",
            "inputs": ["fan1", "fan5"],
            "setpoint": 90.0,
            "pid": {
              "samplePeriod": 0.1,
              "proportionalCoeff": 0.0,
              "integralCoeff": 0.0,
              "feedFwdOffsetCoeff": 0.0,
              "feedFwdGainCoeff": 0.010,
              "integralLimit_min": 0.0,
              "integralLimit_max": 0.0,
              "outLim_min": 30.0,
              "outLim_max": 100.0,
              "slewNeg": 0.0,
              "slewPos": 0.0
            }
          }]
        }]
      }
    )"_json;

    validateJson(j2);
}

TEST(ConfigurationVerificationTest, VerifyNoSensorKey)
{
    /* Verify the sensors key must be present. */
    auto j2 = R"(
      {
        "zones": [{
          "id": 1,
          "minThermalOutput": 3000.0,
          "failsafePercent": 75.0,
          "pids": [{
            "name": "fan1-5",
            "type": "fan",
            "inputs": ["fan1", "fan5"],
            "setpoint": 90.0,
            "pid": {
              "samplePeriod": 0.1,
              "proportionalCoeff": 0.0,
              "integralCoeff": 0.0,
              "feedFwdOffsetCoeff": 0.0,
              "feedFwdGainCoeff": 0.010,
              "integralLimit_min": 0.0,
              "integralLimit_max": 0.0,
              "outLim_min": 30.0,
              "outLim_max": 100.0,
              "slewNeg": 0.0,
              "slewPos": 0.0
            }
          }]
        }]
      }
    )"_json;

    EXPECT_THROW(validateJson(j2), ConfigurationException);
}

TEST(ConfigurationVerificationTest, VerifyNoZoneKey)
{
    /* Verify the zones key must be present. */
    auto j2 = R"(
      {
        "sensors": [{
          "name": "fan1",
          "type": "fan",
          "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1"
        }]
      }
    )"_json;

    EXPECT_THROW(validateJson(j2), ConfigurationException);
}

TEST(ConfigurationVerificationTest, VerifyNoSensor)
{
    /* Verify that there needs to be at least one sensor in the sensors key. */
    auto j2 = R"(
      {
        "sensors": [],
        "zones": [{
          "id": 1,
          "minThermalOutput": 3000.0,
          "failsafePercent": 75.0,
          "pids": [{
            "name": "fan1-5",
            "type": "fan",
            "inputs": ["fan1", "fan5"],
            "setpoint": 90.0,
            "pid": {
              "samplePeriod": 0.1,
              "proportionalCoeff": 0.0,
              "integralCoeff": 0.0,
              "feedFwdOffsetCoeff": 0.0,
              "feedFwdGainCoeff": 0.010,
              "integralLimit_min": 0.0,
              "integralLimit_max": 0.0,
              "outLim_min": 30.0,
              "outLim_max": 100.0,
              "slewNeg": 0.0,
              "slewPos": 0.0
            }
          }]
        }]
      }
    )"_json;

    EXPECT_THROW(validateJson(j2), ConfigurationException);
}

TEST(ConfigurationVerificationTest, VerifyNoPidInZone)
{
    /* Verify that there needs to be at least one PID in the zone. */
    auto j2 = R"(
      {
        "sensors": [{
          "name": "fan1",
          "type": "fan",
          "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1"
        }],
        "zones": [{
          "id": 1,
          "minThermalOutput": 3000.0,
          "failsafePercent": 75.0,
          "pids": []
        }]
      }
    )"_json;

    EXPECT_THROW(validateJson(j2), ConfigurationException);
}
