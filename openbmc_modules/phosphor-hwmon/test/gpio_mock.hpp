#pragma once

#include <gpioplus/handle.hpp>
#include <memory>
#include <string>

#include <gmock/gmock.h>

class GpioHandleInterface
{
  public:
    virtual ~GpioHandleInterface() = default;
    virtual std::unique_ptr<gpioplus::HandleInterface>
        build(const std::string& gpiochip, const std::string& line) const = 0;
};

class GpioHandleMock : public GpioHandleInterface
{
  public:
    virtual ~GpioHandleMock() = default;
    MOCK_CONST_METHOD2(build, std::unique_ptr<gpioplus::HandleInterface>(
                                  const std::string&, const std::string&));
};

// Set this before each test that hits a call to getEnv().
extern GpioHandleInterface* gpioIntf;
