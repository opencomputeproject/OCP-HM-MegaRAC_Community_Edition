#pragma once

#include "pid/zone.hpp"

#include <string>

#include <gmock/gmock.h>

class ZoneMock : public ZoneInterface
{
  public:
    virtual ~ZoneMock() = default;

    MOCK_METHOD1(getCachedValue, double(const std::string&));
    MOCK_METHOD1(addSetPoint, void(double));
    MOCK_METHOD1(addRPMCeiling, void(double));
    MOCK_CONST_METHOD0(getMaxSetPointRequest, double());
    MOCK_CONST_METHOD0(getFailSafeMode, bool());
    MOCK_CONST_METHOD0(getFailSafePercent, double());
    MOCK_METHOD1(getSensor, Sensor*(const std::string&));
};
