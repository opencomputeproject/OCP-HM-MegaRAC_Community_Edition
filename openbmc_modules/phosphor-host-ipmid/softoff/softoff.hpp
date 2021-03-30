#pragma once

#include "config.h"

#include <functional>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdbusplus/timer.hpp>
#include <xyz/openbmc_project/Control/Host/server.hpp>
#include <xyz/openbmc_project/Ipmi/Internal/SoftPowerOff/server.hpp>
namespace phosphor
{
namespace ipmi
{

namespace Base = sdbusplus::xyz::openbmc_project::Ipmi::Internal::server;
using namespace sdbusplus::xyz::openbmc_project::Control::server;

namespace sdbusRule = sdbusplus::bus::match::rules;

/** @class SoftPowerOff
 *  @brief Responsible for coordinating Host SoftPowerOff operation
 */
class SoftPowerOff
    : public sdbusplus::server::object::object<Base::SoftPowerOff>
{
  public:
    /** @brief Constructs SoftPowerOff object.
     *
     *  @param[in] bus       - system dbus handler
     *  @param[in] event     - sd_event handler
     *  @param[in] objPath   - The Dbus path hosting SoftPowerOff function
     */
    SoftPowerOff(sdbusplus::bus::bus& bus, sd_event* event,
                 const char* objPath) :
        sdbusplus::server::object::object<Base::SoftPowerOff>(bus, objPath,
                                                              false),
        bus(bus), timer(event),
        hostControlSignal(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("CommandComplete") +
                sdbusRule::path("/xyz/openbmc_project/control/host0") +
                sdbusRule::interface(CONTROL_HOST_BUSNAME) +
                sdbusRule::argN(0, convertForMessage(Host::Command::SoftOff)),
            std::bind(std::mem_fn(&SoftPowerOff::hostControlEvent), this,
                      std::placeholders::_1))
    {
        // Need to announce since we may get the response
        // very quickly on host shutdown command
        emit_object_added();

        // The whole purpose of this application is to send a host shutdown
        // command and watch for the soft power off to go through. We need
        // the interface added signal emitted before we send the shutdown
        // command just to attend to lightning fast response from host
        sendHostShutDownCmd();
    }

    /** @brief Tells if the objective of this application is completed */
    inline auto isCompleted()
    {
        return completed;
    }

    /** @brief Tells if the referenced timer is expired or not */
    inline auto isTimerExpired()
    {
        return timer.isExpired();
    }

    /** @brief overloaded property setter function
     *
     *  @param[in] value - One of SoftOffReceived / HostShutdown
     *
     *  @return Success or exception thrown
     */
    HostResponse responseReceived(HostResponse value) override;

    /** @brief Using the base class's getter method */
    using Base::SoftPowerOff::responseReceived;

    /** @brief Calls to start a timer
     *
     *  @param[in] usec - Time in microseconds
     *
     *  @return Success or exception thrown
     */
    int startTimer(const std::chrono::microseconds& usec);

  private:
    // Need this to send SMS_ATTN
    // TODO : Switch over to using mapper service in a different patch
    static constexpr auto HOST_IPMI_BUS = "org.openbmc.HostIpmi";
    static constexpr auto HOST_IPMI_OBJ = "/org/openbmc/HostIpmi/1";
    static constexpr auto HOST_IPMI_INTF = "org.openbmc.HostIpmi";

    /* @brief sdbusplus handle */
    sdbusplus::bus::bus& bus;

    /** @brief Reference to Timer object */
    Timer timer;

    /** @brief Marks the end of life of this application.
     *
     *  This is set to true if host gives appropriate responses
     *  for the sequence of commands.
     */
    bool completed = false;

    /** @brief Subscribe to host control signals
     *
     *  Protocol is to send the host power off request to the host
     *  control interface and then wait for a signal indicating pass/fail
     **/
    sdbusplus::bus::match_t hostControlSignal;

    /** @brief Sends host control command to tell host to shut down
     *
     *  After sending the command, wait for a signal indicating the status
     *  of the command.
     *
     *  After receiving the initial response, start a timer for 30 minutes
     *  to let host do a clean shutdown of partitions. When the response is
     *  received from the host, it indicates that BMC can do a power off.
     *  If BMC fails to get any response, then a hard power off would
     *  be forced.
     *
     *  @return - Does not return anything. Error will result in exception
     *            being thrown
     */
    void sendHostShutDownCmd();

    /** @brief Callback function on host control signals
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    void hostControlEvent(sdbusplus::message::message& msg);
};
} // namespace ipmi
} // namespace phosphor
