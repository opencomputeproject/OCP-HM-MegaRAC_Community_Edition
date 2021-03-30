#pragma once

#include "pid/controller.hpp"

#include <gmock/gmock.h>

class ControllerMock : public PIDController
{
  public:
    virtual ~ControllerMock() = default;

    ControllerMock(const std::string& id, PIDZone* owner) :
        PIDController(id, owner)
    {
    }

    MOCK_METHOD0(inputProc, double());
    MOCK_METHOD0(setptProc, double());
    MOCK_METHOD1(outputProc, void(double));
};
