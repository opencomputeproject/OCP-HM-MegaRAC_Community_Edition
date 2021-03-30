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
 * @class PGOODMonitor
 *
 * Monitors PGOOD and checks for errors on the power sequencer
 * if it doesn't come on in time.
 *
 * The run() function is designed to be called right after the
 * power sequencer device is told to kick off a power on.
 *
 * Future commits will analyze the power sequencer chip for errors
 * on a PGOOD fail.
 */
class PGOODMonitor : public DeviceMonitor
{
  public:
    PGOODMonitor() = delete;
    ~PGOODMonitor() = default;
    PGOODMonitor(const PGOODMonitor&) = delete;
    PGOODMonitor& operator=(const PGOODMonitor&) = delete;
    PGOODMonitor(PGOODMonitor&&) = delete;
    PGOODMonitor& operator=(PGOODMonitor&&) = delete;

    /**
     * Constructor
     *
     * @param[in] d - the device to monitor
     * @param[in] b - D-Bus bus object
     * @param[in] e - event object
     * @param[in] t - time to allow PGOOD to come up
     */
    PGOODMonitor(std::unique_ptr<witherspoon::power::Device>&& d,
                 sdbusplus::bus::bus& b, const sdeventplus::Event& e,
                 std::chrono::milliseconds& t) :
        DeviceMonitor(std::move(d), e, t),
        bus(b)
    {}

    /**
     * Analyzes the power sequencer for fails and then
     * notifies the event loop that it can exit.
     *
     * The timer callback.
     */
    void analyze() override;

    /**
     * Waits a specified amount of time for PGOOD to
     * come on, and if it fails to come on in that time
     * it will analyze the power sequencer for faults.
     *
     * It will exit after either PGOOD is asserted or
     * the device is analyzed for faults.
     *
     * @return - the return value from sd_event_loop()
     */
    int run() override;

  private:
    /**
     * Enables the properties changed signal callback
     * on the power object so we can tell when PGOOD
     * comes on.
     */
    void startListening();

    /**
     * The callback function for the properties changed
     * signal.
     */
    void propertyChanged();

    /**
     * Returns true if the system has been turned on
     * but PGOOD isn't up yet.
     */
    bool pgoodPending();

    /**
     * Used to break out of the event loop in run()
     */
    void exitEventLoop();

    /**
     * The D-Bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * The match object for the properties changed signal
     */
    std::unique_ptr<sdbusplus::bus::match_t> match;
};

} // namespace power
} // namespace witherspoon
