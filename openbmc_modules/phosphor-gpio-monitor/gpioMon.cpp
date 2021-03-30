/**
 * Copyright © 2019 Facebook
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

#include "gpioMon.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace gpio
{

/* systemd service to kick start a target. */
constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_ROOT = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

using namespace phosphor::logging;

void GpioMonitor::scheduleEventHandler()
{

    gpioEventDescriptor.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [this](const boost::system::error_code& ec) {
            if (ec)
            {
                std::string msg = gpioLineMsg + "event handler error" +
                                  std::string(ec.message());
                log<level::ERR>(msg.c_str());
                return;
            }
            gpioEventHandler();
        });
}

void GpioMonitor::gpioEventHandler()
{
    gpiod_line_event gpioLineEvent;

    if (gpiod_line_event_read_fd(gpioEventDescriptor.native_handle(),
                                 &gpioLineEvent) < 0)
    {
        log<level::ERR>("Failed to read gpioLineEvent from fd",
                        entry("GPIO_LINE=%s", gpioLineMsg.c_str()));
        return;
    }

    std::string logMessage =
        gpioLineMsg + (gpioLineEvent.event_type == GPIOD_LINE_EVENT_RISING_EDGE
                           ? " Asserted"
                           : " Deasserted");

    log<level::INFO>(logMessage.c_str());

    /* Execute the target if it is defined. */
    if (!target.empty())
    {
        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_ROOT,
                                          SYSTEMD_INTERFACE, "StartUnit");
        method.append(target);
        method.append("replace");

        bus.call_noreply(method);
    }

    /* if not required to continue monitoring then return */
    if (!continueAfterEvent)
    {
        return;
    }

    /* Schedule a wait event */
    scheduleEventHandler();
}

int GpioMonitor::requestGPIOEvents()
{

    /* Request an event to monitor for respected gpio line */
    if (gpiod_line_request(gpioLine, &gpioConfig, 0) < 0)
    {
        log<level::ERR>("Failed to request gpioLineEvent",
                        entry("GPIO_LINE=%s", gpioLineMsg.c_str()));
        return -1;
    }

    int gpioLineFd = gpiod_line_event_get_fd(gpioLine);
    if (gpioLineFd < 0)
    {
        log<level::ERR>("Failed to get fd for gpioLineEvent",
                        entry("GPIO_LINE=%s", gpioLineMsg.c_str()));
        return -1;
    }

    std::string logMsg = gpioLineMsg + " monitoring started";
    log<level::INFO>(logMsg.c_str());

    /* Assign line fd to descriptor for monitoring */
    gpioEventDescriptor.assign(gpioLineFd);

    /* Schedule a wait event */
    scheduleEventHandler();

    return 0;
}
} // namespace gpio
} // namespace phosphor
