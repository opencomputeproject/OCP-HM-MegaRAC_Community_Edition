/**
 * Copyright © 2017 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "gpio.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <cassert>

namespace witherspoon
{
namespace gpio
{

using namespace phosphor::logging;

using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

Value GPIO::read()
{
    assert(direction == Direction::input);

    requestLine();

    gpiohandle_data data{};

    auto rc = ioctl(lineFD(), GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);

    if (rc < 0)
    {
        auto e = errno;
        log<level::ERR>("Failed GET_LINE_VALUES ioctl", entry("ERRNO=%d", e));
        elog<InternalFailure>();
    }

    return (data.values[0] == 0) ? Value::low : Value::high;
}

void GPIO::set(Value value)
{
    assert(direction == Direction::output);

    requestLine(value);

    gpiohandle_data data{};
    data.values[0] = static_cast<gpioValue_t>(value);

    auto rc = ioctl(lineFD(), GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    if (rc == -1)
    {
        auto e = errno;
        log<level::ERR>("Failed SET_LINE_VALUES ioctl", entry("ERRNO=%d", e));
        elog<InternalFailure>();
    }
}

void GPIO::requestLine(Value defaultValue)
{
    // Only need to do this once
    if (lineFD)
    {
        return;
    }

    power::util::FileDescriptor fd{open(device.c_str(), 0)};
    if (fd() == -1)
    {
        auto e = errno;
        log<level::ERR>("Failed opening GPIO device",
                        entry("DEVICE=%s", device.c_str()),
                        entry("ERRNO=%d", e));
        elog<InternalFailure>();
    }

    // Make an ioctl call to request the GPIO line, which will
    // return the descriptor to use to access it.
    gpiohandle_request request{};
    strncpy(request.consumer_label, "witherspoon-pfault-analysis",
            sizeof(request.consumer_label));

    request.flags = (direction == Direction::input) ? GPIOHANDLE_REQUEST_INPUT
                                                    : GPIOHANDLE_REQUEST_OUTPUT;

    request.lineoffsets[0] = gpio;
    request.lines = 1;

    if (direction == Direction::output)
    {
        request.default_values[0] = static_cast<gpioValue_t>(defaultValue);
    }

    auto rc = ioctl(fd(), GPIO_GET_LINEHANDLE_IOCTL, &request);
    if (rc == -1)
    {
        auto e = errno;
        log<level::ERR>("Failed GET_LINEHANDLE ioctl", entry("GPIO=%d", gpio),
                        entry("ERRNO=%d", e));
        elog<InternalFailure>();
    }

    lineFD.set(request.fd);
}

} // namespace gpio
} // namespace witherspoon
