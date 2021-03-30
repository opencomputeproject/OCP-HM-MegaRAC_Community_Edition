#include "gpio_mock.hpp"

#include <memory>
#include <string>

// Set this before each test that hits a call to getEnv().
GpioHandleInterface* gpioIntf;

namespace gpio
{

std::unique_ptr<gpioplus::HandleInterface>
    BuildGpioHandle(const std::string& gpiochip, const std::string& line)
{
    return (gpioIntf) ? gpioIntf->build(gpiochip, line) : nullptr;
}

} // namespace gpio
