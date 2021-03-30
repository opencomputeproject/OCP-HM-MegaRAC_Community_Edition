#pragma once

#include "evdev.hpp"

#include <linux/input.h>
#include <systemd/sd-event.h>
#include <unistd.h>

#include <sdbusplus/bus.hpp>
#include <string>

namespace phosphor
{
namespace gpio
{

/** @class Monitor
 *  @brief Responsible for catching GPIO state change
 *  condition and starting systemd targets.
 */
class Monitor : public Evdev
{
  public:
    Monitor() = delete;
    ~Monitor() = default;
    Monitor(const Monitor&) = delete;
    Monitor& operator=(const Monitor&) = delete;
    Monitor(Monitor&&) = delete;
    Monitor& operator=(Monitor&&) = delete;

    /** @brief Constructs Monitor object.
     *
     *  @param[in] path     - Path to gpio input device
     *  @param[in] key      - GPIO key to monitor
     *  @param[in] polarity - GPIO assertion polarity to look for
     *  @param[in] target   - systemd unit to be started on GPIO
     *                        value change
     *  @param[in] event    - sd_event handler
     *  @param[in] continueRun - Whether to continue after key pressed
     *  @param[in] handler  - IO callback handler. Defaults to one in this
     *                        class
     *  @param[in] useEvDev - Whether to use EvDev to retrieve events
     */
    Monitor(const std::string& path, decltype(input_event::code) key,
            decltype(input_event::value) polarity, const std::string& target,
            EventPtr& event, bool continueRun,
            sd_event_io_handler_t handler = Monitor::processEvents,
            bool useEvDev = true) :
        Evdev(path, key, event, handler, useEvDev),
        polarity(polarity), target(target),
        continueAfterKeyPress(continueRun){};

    /** @brief Callback handler when the FD has some activity on it
     *
     *  @param[in] es       - Populated event source
     *  @param[in] fd       - Associated File descriptor
     *  @param[in] revents  - Type of event
     *  @param[in] userData - User data that was passed during registration
     *
     *  @return             - 0 or positive number on success and negative
     *                        errno otherwise
     */
    static int processEvents(sd_event_source* es, int fd, uint32_t revents,
                             void* userData);

    /** @brief Returns the completion state of this handler */
    inline auto completed() const
    {
        return complete;
    }

  private:
    /** @brief GPIO key value that is of interest */
    decltype(input_event::value) polarity;

    /** @brief Systemd unit to be started when the condition is met */
    const std::string& target;

    /** @brief If the monitor should continue after key press */
    bool continueAfterKeyPress;

    /** @brief Completion indicator */
    bool complete = false;

    /** @brief Analyzes the GPIO event and starts configured target */
    void analyzeEvent();
};

} // namespace gpio
} // namespace phosphor
