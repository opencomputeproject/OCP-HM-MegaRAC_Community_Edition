#include "config.h"

#include "host-interface.hpp"

#include "systemintfcmds.hpp"

#include <functional>
#include <ipmid-host/cmd-utils.hpp>
#include <ipmid-host/cmd.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace host
{
namespace command
{

// When you see Base:: you know we're referencing our base class
namespace Base = sdbusplus::xyz::openbmc_project::Control::server;

// IPMI OEM command.
// https://github.com/openbmc/openbmc/issues/2082 for handling
// Non-OEM commands that need to send SMS_ATN
using OEMCmd = uint8_t;

// Map of IPMI OEM command to its equivalent interface command.
// This is needed when invoking the callback handler to indicate
// the status of the executed command.
static const std::map<OEMCmd, Host::Command> intfCommand = {
    {CMD_HEARTBEAT, Base::Host::Command::Heartbeat},
    {CMD_POWER, Base::Host::Command::SoftOff}};

// Map of Interface command to its corresponding IPMI OEM command.
// This is needed when pushing IPMI commands to command manager's
// queue. The same pair will be returned when IPMI asks us
// why a SMS_ATN was sent
static const std::map<Host::Command, IpmiCmdData> ipmiCommand = {
    {Base::Host::Command::Heartbeat, std::make_pair(CMD_HEARTBEAT, 0x00)},
    {Base::Host::Command::SoftOff, std::make_pair(CMD_POWER, SOFT_OFF)}};

// Called at user request
void Host::execute(Base::Host::Command command)
{
    using namespace phosphor::logging;

    log<level::DEBUG>(
        "Pushing cmd on to queue",
        entry("CONTROL_HOST_CMD=%s", convertForMessage(command).c_str()));

    auto cmd = std::make_tuple(ipmiCommand.at(command),
                               std::bind(&Host::commandStatusHandler, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2));

    ipmid_send_cmd_to_host(std::move(cmd));
}

// Called into by Command Manager
void Host::commandStatusHandler(IpmiCmdData cmd, bool status)
{
    // Need to convert <cmd> to the equivalent one mentioned in spec
    auto value = status ? Result::Success : Result::Failure;

    // Fire a signal
    this->commandComplete(intfCommand.at(std::get<0>(cmd)), value);
}

} // namespace command
} // namespace host
} // namespace phosphor
