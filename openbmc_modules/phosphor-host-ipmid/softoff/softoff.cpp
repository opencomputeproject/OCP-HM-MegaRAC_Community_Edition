/**
 * Copyright © 2016 IBM Corporation
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
#include "config.h"

#include "softoff.hpp"

#include <chrono>
#include <ipmid/utils.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Control/Host/server.hpp>
namespace phosphor
{
namespace ipmi
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Control::server;

void SoftPowerOff::sendHostShutDownCmd()
{
    auto ctrlHostPath =
        std::string{CONTROL_HOST_OBJ_MGR} + '/' + HOST_NAME + '0';
    auto host = ::ipmi::getService(this->bus, CONTROL_HOST_BUSNAME,
                                   ctrlHostPath.c_str());

    auto method = bus.new_method_call(host.c_str(), ctrlHostPath.c_str(),
                                      CONTROL_HOST_BUSNAME, "Execute");

    method.append(convertForMessage(Host::Command::SoftOff).c_str());

    auto reply = bus.call(method);
    if (reply.is_method_error())
    {
        log<level::ERR>("Error in call to control host Execute");
        // TODO openbmc/openbmc#851 - Once available, throw returned error
        throw std::runtime_error("Error in call to control host Execute");
    }

    return;
}

// Function called on host control signals
void SoftPowerOff::hostControlEvent(sdbusplus::message::message& msg)
{
    std::string cmdCompleted{};
    std::string cmdStatus{};

    msg.read(cmdCompleted, cmdStatus);

    log<level::DEBUG>("Host control signal values",
                      entry("COMMAND=%s", cmdCompleted.c_str()),
                      entry("STATUS=%s", cmdStatus.c_str()));

    if (Host::convertResultFromString(cmdStatus) == Host::Result::Success)
    {
        // Set our internal property indicating we got host attention
        sdbusplus::xyz::openbmc_project::Ipmi::Internal ::server::SoftPowerOff::
            responseReceived(HostResponse::SoftOffReceived);

        // Start timer for host shutdown
        using namespace std::chrono;
        auto time = duration_cast<microseconds>(
            seconds(IPMI_HOST_SHUTDOWN_COMPLETE_TIMEOUT_SECS));
        auto r = startTimer(time);
        if (r < 0)
        {
            log<level::ERR>("Failure to start Host shutdown wait timer",
                            entry("ERRNO=0x%X", -r));
        }
        else
        {
            log<level::INFO>(
                "Timer started waiting for host to shutdown",
                entry("TIMEOUT_IN_MSEC=%llu",
                      (duration_cast<milliseconds>(
                           seconds(IPMI_HOST_SHUTDOWN_COMPLETE_TIMEOUT_SECS)))
                          .count()));
        }
    }
    else
    {
        // An error on the initial attention is not considered an error, just
        // exit normally and allow remaining shutdown targets to run
        log<level::INFO>("Timeout on host attention, continue with power down");
        completed = true;
    }
    return;
}

// Starts a timer
int SoftPowerOff::startTimer(const std::chrono::microseconds& usec)
{
    return timer.start(usec);
}

// Host Response handler
auto SoftPowerOff::responseReceived(HostResponse response) -> HostResponse
{
    using namespace std::chrono;

    if (response == HostResponse::HostShutdown)
    {
        // Disable the timer since Host has quiesced and we are
        // done with soft power off part
        auto r = timer.stop();
        if (r < 0)
        {
            log<level::ERR>("Failure to STOP the timer",
                            entry("ERRNO=0x%X", -r));
        }

        // This marks the completion of soft power off sequence.
        completed = true;
    }

    return sdbusplus::xyz::openbmc_project::Ipmi::Internal ::server::
        SoftPowerOff::responseReceived(response);
}

} // namespace ipmi
} // namespace phosphor
