#pragma once

#include <gpioplus/handle.hpp>
#include <memory>
#include <string>

namespace gpio
{

/**
 * Method called to validate inputs and create a GpioHandle.
 *
 * @param[in] gpiochip - gpiochip id as string, e.g. "0", or "1"
 * @param[in] line - gpio line offset as string.
 * @return A gpioplus::HandleInterface on success nullptr on failure.
 */
std::unique_ptr<gpioplus::HandleInterface>
    BuildGpioHandle(const std::string& gpiochip, const std::string& line);

} // namespace gpio
