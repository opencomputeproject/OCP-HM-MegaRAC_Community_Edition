#pragma once

#include "device.hpp"
#include "device_monitor.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdeventplus/event.hpp>

namespace witherspoon
{
namespace power
{

/**
 * @class RuntimeMonitor
 *
 * Monitors the power sequencer for faults at runtime
 *
 * Triggers the power sequencer fault check 2 different ways:
 *
 * 1) Listens for the PowerLost signal that indicates master
 *    PGOOD was dropped due to a fatal fault.  After the analysis,
 *    a power off will be issued so the sequencer will stop
 *    driving power to a faulted component.
 *
 * 2) Polls for faults, as some won't always drop PGOOD.
 *
 * The application this runs in will only run while PGOOD is
 * expected to be asserted, so any loss of PGOOD is considered
 * an error.
 */
class RuntimeMonitor : public DeviceMonitor
{
  public:
    RuntimeMonitor() = delete;
    ~RuntimeMonitor() = default;
    RuntimeMonitor(const RuntimeMonitor&) = delete;
    RuntimeMonitor& operator=(const RuntimeMonitor&) = delete;
    RuntimeMonitor(RuntimeMonitor&&) = delete;
    RuntimeMonitor& operator=(RuntimeMonitor&&) = delete;

    /**
     * Constructor
     *
     * @param[in] d - the device to monitor
     * @param[in] b - D-Bus bus object
     * @param[in] e - event object
     * @param[in] i - poll interval
     */
    RuntimeMonitor(std::unique_ptr<witherspoon::power::Device>&& d,
                   sdbusplus::bus::bus& b, const sdeventplus::Event& e,
                   std::chrono::milliseconds& i) :
        DeviceMonitor(std::move(d), e, i),
        bus(b), match(bus, getMatchString(),
                      std::bind(std::mem_fn(&RuntimeMonitor::onPowerLost), this,
                                std::placeholders::_1))
    {}

    /**
     * Clears faults and then runs DeviceMonitor::run to
     * call Device::analyze() on an ongoing interval.
     *
     * @return the return value from sd_event_loop()
     */
    int run() override;

  private:
    /**
     * The PowerLost signal handler.
     *
     * After doing an analysis, will issue a power off
     * as some device has a power fault and needs to be
     * properly shut down.
     *
     * @param[in] msg - D-Bus message for callback
     */
    void onPowerLost(sdbusplus::message::message& msg);

    /**
     * Returns the match string for the PowerLost signal
     */
    std::string getMatchString()
    {
        using namespace sdbusplus::bus::match::rules;

        std::string s = type::signal() + path("/org/openbmc/control/power0") +
                        interface("org.openbmc.control.Power") +
                        member("PowerLost");

        return s;
    }

    /**
     * The D-Bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * Match object for PowerLost signals
     */
    sdbusplus::bus::match_t match;
};

} // namespace power
} // namespace witherspoon
