#include "config.h"

#include "host-cmd-manager.hpp"

#include "systemintfcmds.hpp"

#include <chrono>
#include <ipmid/utils.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/timer.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

namespace phosphor
{
namespace host
{
namespace command
{

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto HOST_STATE_PATH = "/xyz/openbmc_project/state/host0";
constexpr auto HOST_STATE_INTERFACE = "xyz.openbmc_project.State.Host";
constexpr auto HOST_TRANS_PROP = "RequestedHostTransition";

// For throwing exceptions
using namespace phosphor::logging;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

namespace sdbusRule = sdbusplus::bus::match::rules;

Manager::Manager(sdbusplus::bus::bus& bus) :
    bus(bus), timer(std::bind(&Manager::hostTimeout, this)),
    hostTransitionMatch(
        bus,
        sdbusRule::propertiesChanged(HOST_STATE_PATH, HOST_STATE_INTERFACE),
        std::bind(&Manager::clearQueueOnPowerOn, this, std::placeholders::_1))
{
    // Nothing to do here.
}

// Called as part of READ_MSG_DATA command
IpmiCmdData Manager::getNextCommand()
{
    // Stop the timer. Don't have to Err failure doing so.
    auto r = timer.stop();
    if (r < 0)
    {
        log<level::ERR>("Failure to STOP the timer",
                        entry("ERROR=%s", strerror(-r)));
    }

    if (this->workQueue.empty())
    {
        // Just return a heartbeat in this case.  A spurious SMS_ATN was
        // asserted for the host (probably from a previous boot).
        log<level::DEBUG>("Control Host work queue is empty!");

        return std::make_pair(CMD_HEARTBEAT, 0x00);
    }

    // Pop the processed entry off the queue
    auto command = this->workQueue.front();
    this->workQueue.pop();

    // IPMI command is the first element in pair
    auto ipmiCmdData = std::get<0>(command);

    // Now, call the user registered functions so that
    // implementation specific CommandComplete signals
    // can be sent. `true` indicating Success.
    std::get<CallBack>(command)(ipmiCmdData, true);

    // Check for another entry in the queue and kick it off
    this->checkQueueAndAlertHost();

    // Tuple of command and data
    return ipmiCmdData;
}

// Called when initial timer goes off post sending SMS_ATN
void Manager::hostTimeout()
{
    log<level::ERR>("Host control timeout hit!");

    clearQueue();
}

void Manager::clearQueue()
{
    // Dequeue all entries and send fail signal
    while (!this->workQueue.empty())
    {
        auto command = this->workQueue.front();
        this->workQueue.pop();

        // IPMI command is the first element in pair
        auto ipmiCmdData = std::get<0>(command);

        // Call the implementation specific Command Failure.
        // `false` indicating Failure
        std::get<CallBack>(command)(ipmiCmdData, false);
    }
}

// Called for alerting the host
void Manager::checkQueueAndAlertHost()
{
    if (this->workQueue.size() >= 1)
    {
        log<level::DEBUG>("Asserting SMS Attention");

        std::string IPMI_PATH("/org/openbmc/HostIpmi/1");
        std::string IPMI_INTERFACE("org.openbmc.HostIpmi");

        auto host = ::ipmi::getService(this->bus, IPMI_INTERFACE, IPMI_PATH);

        // Start the timer for this transaction
        auto time = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::seconds(IPMI_SMS_ATN_ACK_TIMEOUT_SECS));

        auto r = timer.start(time);
        if (r < 0)
        {
            log<level::ERR>("Error starting timer for control host");
            return;
        }

        auto method =
            this->bus.new_method_call(host.c_str(), IPMI_PATH.c_str(),
                                      IPMI_INTERFACE.c_str(), "setAttention");
        auto reply = this->bus.call(method);

        if (reply.is_method_error())
        {
            log<level::ERR>("Error in setting SMS attention");
            elog<InternalFailure>();
        }
        log<level::DEBUG>("SMS Attention asserted");
    }
}

// Called by specific implementations that provide commands
void Manager::execute(CommandHandler command)
{
    log<level::DEBUG>("Pushing cmd on to queue",
                      entry("COMMAND=%d", std::get<0>(command).first));

    this->workQueue.emplace(command);

    // Alert host if this is only command in queue otherwise host will
    // be notified of next message after processing the current one
    if (this->workQueue.size() == 1)
    {
        this->checkQueueAndAlertHost();
    }
    else
    {
        log<level::INFO>("Command in process, no attention");
    }

    return;
}

void Manager::clearQueueOnPowerOn(sdbusplus::message::message& msg)
{
    namespace server = sdbusplus::xyz::openbmc_project::State::server;

    ::ipmi::DbusInterface interface;
    ::ipmi::PropertyMap properties;

    msg.read(interface, properties);

    if (properties.find(HOST_TRANS_PROP) == properties.end())
    {
        return;
    }

    auto& requestedState =
        std::get<std::string>(properties.at(HOST_TRANS_PROP));

    if (server::Host::convertTransitionFromString(requestedState) ==
        server::Host::Transition::On)
    {
        clearQueue();
    }
}

} // namespace command
} // namespace host
} // namespace phosphor
