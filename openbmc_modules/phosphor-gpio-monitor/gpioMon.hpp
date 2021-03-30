#pragma once

#include <gpiod.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

namespace phosphor
{
namespace gpio
{

/** @class GpioMonitor
 *  @brief Responsible for catching GPIO state change
 *  condition and starting systemd targets.
 */
class GpioMonitor
{
  public:
    GpioMonitor() = delete;
    ~GpioMonitor() = default;
    GpioMonitor(const GpioMonitor&) = delete;
    GpioMonitor& operator=(const GpioMonitor&) = delete;
    GpioMonitor(GpioMonitor&&) = delete;
    GpioMonitor& operator=(GpioMonitor&&) = delete;

    /** @brief Constructs GpioMonitor object.
     *
     *  @param[in] line        - GPIO line from libgpiod
     *  @param[in] config      - configuration of line with event
     *  @param[in] io          - io service
     *  @param[in] target      - systemd unit to be started on GPIO
     *                           value change
     *  @param[in] lineMsg     - GPIO line message to be used for log
     *  @param[in] continueRun - Whether to continue after event occur
     */
    GpioMonitor(gpiod_line* line, gpiod_line_request_config& config,
                boost::asio::io_service& io, const std::string& target,
                const std::string& lineMsg, bool continueRun) :
        gpioLine(line),
        gpioConfig(config), gpioEventDescriptor(io), target(target),
        gpioLineMsg(lineMsg), continueAfterEvent(continueRun)
    {
        requestGPIOEvents();
    };

  private:
    /** @brief GPIO line */
    gpiod_line* gpioLine;

    /** @brief GPIO line configuration */
    gpiod_line_request_config gpioConfig;

    /** @brief GPIO event descriptor */
    boost::asio::posix::stream_descriptor gpioEventDescriptor;

    /** @brief Systemd unit to be started when the condition is met */
    const std::string target;

    /** @brief GPIO line name message */
    std::string gpioLineMsg;

    /** @brief If the monitor should continue after event */
    bool continueAfterEvent;

    /** @brief register handler for gpio event
     *
     *  @return  - 0 on success and -1 otherwise
     */
    int requestGPIOEvents();

    /** @brief Schedule an event handler for GPIO event to trigger */
    void scheduleEventHandler();

    /** @brief Handle the GPIO event and starts configured target */
    void gpioEventHandler();
};

} // namespace gpio
} // namespace phosphor
